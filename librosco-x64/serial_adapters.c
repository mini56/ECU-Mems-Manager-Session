/* Native read-only COM discovery, based on Session 205 serialadapterdetector.h.
 * SetupAPI Ports class + QueryDosDevice fallback for virtual serial aliases.
 * No CreateFile, serial writes, driver installation or USB configuration.
 */
#include "serial_adapters.h"
#include <setupapi.h>
#include <cfgmgr32.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

static const GUID ports_class={0x4d36e978,0xe325,0x11ce,{0xbf,0xc1,0x08,0x00,0x2b,0xe1,0x03,0x18}};
typedef struct adapter_list {
    mems_manager_serial_adapter *items;
    size_t count,capacity;
} adapter_list;

static char ascii_upper(char c){return c>='a'&&c<='z'?(char)(c-'a'+'A'):c;}
static bool contains_ci(const char *s,const char *needle)
{
    size_t i,j,n=strlen(needle);
    if(!s)return false;
    for(i=0;s[i];++i){
        for(j=0;j<n && s[i+j] && ascii_upper(s[i+j])==needle[j];++j){}
        if(j==n)return true;
    }
    return false;
}
static bool labelled(const char *m,const char *d,const char *s)
{return contains_ci(m,s)||contains_ci(d,s);}
mems_manager_adapter_family mems_manager_detect_adapter_family(
    uint16_t vid,bool has_vid,uint16_t pid,bool has_pid,
    const char *manufacturer,const char *description,bool *prefer_kline)
{
    if(prefer_kline)*prefer_kline=has_vid&&has_pid&&vid==0x1A86u&&pid==0x7523u;
    if((has_vid&&vid==0x0403u)||labelled(manufacturer,description,"FTDI")||labelled(manufacturer,description,"FT232"))
        return MEMS_MANAGER_ADAPTER_FTDI;
    if((has_vid&&vid==0x067Bu)||labelled(manufacturer,description,"PROLIFIC")||labelled(manufacturer,description,"PL2303"))
        return MEMS_MANAGER_ADAPTER_PROLIFIC;
    if((has_vid&&vid==0x1A86u)||labelled(manufacturer,description,"CH340")||labelled(manufacturer,description,"CH341")||labelled(manufacturer,description,"WCH"))
        return MEMS_MANAGER_ADAPTER_WCH;
    if((has_vid&&vid==0x10C4u)||labelled(manufacturer,description,"CP210")||labelled(manufacturer,description,"SILICON LABS"))
        return MEMS_MANAGER_ADAPTER_CP210X;
    return has_vid?MEMS_MANAGER_ADAPTER_USB_OTHER:MEMS_MANAGER_ADAPTER_SERIAL;
}
static bool normalize(const char *in,char port[16])
{
    unsigned long n=0;
    if(!in)return false;
    while(*in==' '||*in=='\t')++in;
    if(strncmp(in,"\\\\.\\",4)==0)in+=4;
    if(strlen(in)<4||ascii_upper(in[0])!='C'||ascii_upper(in[1])!='O'||ascii_upper(in[2])!='M')return false;
    in+=3;
    if(*in<'0'||*in>'9')return false;
    while(*in>='0'&&*in<='9'){
        if(n>6553u)return false;
        n=n*10u+(unsigned)(*in++-'0');
        if(n>65535u)return false;
    }
    while(*in==' '||*in=='\t')++in;
    if(*in||!n)return false;
    snprintf(port,16,"COM%lu",n);
    return true;
}
static void utf8(const wchar_t *w,char *dst,size_t capacity)
{
    size_t n;
    if(!capacity)return;
    dst[0]=0;
    if(!w)return;
    n=wcslen(w);
    while(n){
        /* Do not split a surrogate pair when truncating. */
        if(w[n-1]>=0xD800 && w[n-1]<=0xDBFF){--n;continue;}
        int got=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,w,(int)n,dst,(int)capacity-1,NULL,NULL);
        if(got>0){dst[got]=0;return;}
        --n;
    }
}
static int hex_value(wchar_t c)
{
    if(c>=L'0'&&c<=L'9')return (int)(c-L'0');
    if(c>=L'A'&&c<=L'F')return (int)(c-L'A'+10);
    if(c>=L'a'&&c<=L'f')return (int)(c-L'a'+10);
    return -1;
}
static bool id_token(const wchar_t *s,const wchar_t *token,uint16_t *value)
{
    size_t i,j,n=wcslen(s);
    for(i=0;i+8<=n;++i){
        unsigned v=0;
        if(_wcsnicmp(s+i,token,4)!=0)continue;
        for(j=0;j<4;++j){
            int h=hex_value(s[i+4+j]);
            if(h<0)break;
            v=(v<<4)|(unsigned)h;
        }
        if(j==4 && (i+8==n||hex_value(s[i+8])<0)){*value=(uint16_t)v;return true;}
    }
    return false;
}
static void absorb_ids(mems_manager_serial_adapter *a,const wchar_t *id)
{
    uint16_t vid=0,pid=0;
    bool v=id_token(id,L"VID_",&vid),p=id_token(id,L"PID_",&pid);
    /* Keep IDs from the same identifier rather than combining unrelated parents. */
    if(v&&(!a->has_vendor_id||(!a->has_product_id&&a->vendor_id==vid&&p))){
        a->vendor_id=vid;a->has_vendor_id=1;
        if(p){a->product_id=pid;a->has_product_id=1;}
    }
}
static void classify(mems_manager_serial_adapter *a)
{
    bool hint=false;
    a->family=a->detected_by_system?(uint32_t)mems_manager_detect_adapter_family(
        a->vendor_id,a->has_vendor_id!=0,a->product_id,a->has_product_id!=0,
        a->manufacturer,a->description,&hint):MEMS_MANAGER_ADAPTER_CONFIGURED;
    a->prefers_kline_first=(uint8_t)hint;
}
static bool append(adapter_list *l,const mems_manager_serial_adapter *a)
{
    size_t i;
    for(i=0;i<l->count;++i)if(strcmp(l->items[i].port,a->port)==0){
        /* SetupAPI metadata takes precedence over a generic DOS alias. */
        if(!l->items[i].description[0]&&a->description[0])l->items[i]=*a;
        return true;
    }
    if(l->count==l->capacity){
        size_t next=l->capacity?l->capacity*2:16;
        mems_manager_serial_adapter *tmp;
        if(next<l->capacity||next>SIZE_MAX/sizeof(*tmp))return false;
        tmp=(mems_manager_serial_adapter*)realloc(l->items,next*sizeof(*tmp));
        if(!tmp)return false;
        l->items=tmp;l->capacity=next;
    }
    l->items[l->count++]=*a;return true;
}
static bool make_adapter(const char *port,bool detected,mems_manager_serial_adapter *a)
{
    memset(a,0,sizeof(*a));
    if(!normalize(port,a->port))return false;
    snprintf(a->device_path,sizeof(a->device_path),"\\\\.\\%s",a->port);
    a->detected_by_system=(uint8_t)detected;classify(a);return true;
}
static bool property(HDEVINFO set,SP_DEVINFO_DATA *dev,DWORD which,wchar_t *buf,DWORD chars,DWORD *type)
{
    memset(buf,0,chars*sizeof(*buf));
    if(!SetupDiGetDeviceRegistryPropertyW(set,dev,which,type,(PBYTE)buf,(chars-2)*sizeof(*buf),NULL))
        return false;
    return true;
}
static bool enumerate_setup(adapter_list *l,bool *success)
{
    HDEVINFO set=SetupDiGetClassDevsW(&ports_class,NULL,NULL,DIGCF_PRESENT);
    DWORD i;
    *success=false;
    if(set==INVALID_HANDLE_VALUE)return true; /* DOS fallback still available. */
    for(i=0;;++i){
        SP_DEVINFO_DATA dev;
        HKEY key;
        DWORD type=0,bytes;
        wchar_t port_w[64],buf[2048];
        char port[64];
        mems_manager_serial_adapter a;
        memset(&dev,0,sizeof(dev));dev.cbSize=sizeof(dev);
        if(!SetupDiEnumDeviceInfo(set,i,&dev)){
            *success=GetLastError()==ERROR_NO_MORE_ITEMS;
            break;
        }
        key=SetupDiOpenDevRegKey(set,&dev,DICS_FLAG_GLOBAL,0,DIREG_DEV,KEY_QUERY_VALUE);
        if(key==INVALID_HANDLE_VALUE)continue;
        memset(port_w,0,sizeof(port_w));bytes=sizeof(port_w)-sizeof(wchar_t);
        LONG rc=RegQueryValueExW(key,L"PortName",NULL,&type,(LPBYTE)port_w,&bytes);
        RegCloseKey(key);
        if(rc!=ERROR_SUCCESS||type!=REG_SZ)continue;
        utf8(port_w,port,sizeof(port));
        if(!make_adapter(port,true,&a))continue;
        if(property(set,&dev,SPDRP_FRIENDLYNAME,buf,2048,&type)&&type==REG_SZ)
            utf8(buf,a.description,sizeof(a.description));
        if(!a.description[0]&&property(set,&dev,SPDRP_DEVICEDESC,buf,2048,&type)&&type==REG_SZ)
            utf8(buf,a.description,sizeof(a.description));
        if(property(set,&dev,SPDRP_MFG,buf,2048,&type)&&type==REG_SZ)
            utf8(buf,a.manufacturer,sizeof(a.manufacturer));
        if(property(set,&dev,SPDRP_HARDWAREID,buf,2048,&type)&&type==REG_MULTI_SZ){
            const wchar_t *p=buf;
            while(*p){absorb_ids(&a,p);p+=wcslen(p)+1;}
        }
        /* FTDI bus children may expose VID/PID only in the device ancestry. */
        DEVINST node=dev.DevInst;
        for(unsigned depth=0;depth<6&&!a.has_product_id;++depth){
            if(CM_Get_Device_IDW(node,buf,2048,0)==CR_SUCCESS){
                buf[2047]=0;absorb_ids(&a,buf);
            }
            /* Stop after the device and immediate bus parents; don't assign a hub ID. */
            if(depth>=1 || (!a.has_vendor_id && !labelled(a.manufacturer,a.description,"FTDI") &&
                !labelled(a.manufacturer,a.description,"FT232")) ||
                CM_Get_Parent(&node,node,0)!=CR_SUCCESS)break;
        }
        classify(&a);
        if(!append(l,&a)){SetupDiDestroyDeviceInfoList(set);return false;}
    }
    SetupDiDestroyDeviceInfoList(set);return true;
}
static bool enumerate_dos(adapter_list *l,bool *success)
{
    DWORD capacity=4096,got=0;
    wchar_t *buf=NULL;
    *success=false;
    for(;;){
        wchar_t *tmp=(wchar_t*)realloc(buf,(size_t)capacity*sizeof(*tmp));
        if(!tmp){free(buf);return false;}buf=tmp;
        got=QueryDosDeviceW(NULL,buf,capacity);
        if(got)break;
        if(GetLastError()!=ERROR_INSUFFICIENT_BUFFER||capacity>=1048576u){free(buf);return true;}
        capacity*=2;
    }
    for(const wchar_t *p=buf;p<buf+got&&*p;p+=wcslen(p)+1){
        char port[64];mems_manager_serial_adapter a;
        if(wcslen(p)>=sizeof(port))continue;
        utf8(p,port,sizeof(port));
        if(make_adapter(port,true,&a)&&!append(l,&a)){free(buf);return false;}
    }
    free(buf);*success=true;return true;
}
static int port_compare(const void *p,const void *q)
{
    const mems_manager_serial_adapter *a=(const mems_manager_serial_adapter*)p,*b=(const mems_manager_serial_adapter*)q;
    long x=strtol(a->port+3,NULL,10),y=strtol(b->port+3,NULL,10);
    return x<y?-1:x>y?1:0;
}
static bool order(adapter_list *l,const char *preferred)
{
    char port[16];size_t i;mems_manager_serial_adapter a;
    if(l->count>1)qsort(l->items,l->count,sizeof(*l->items),port_compare);
    if(!normalize(preferred,port))return true;
    for(i=0;i<l->count;++i)if(strcmp(l->items[i].port,port)==0)break;
    if(i==l->count){
        if(!make_adapter(port,false,&a)||!append(l,&a))return false;
    }
    a=l->items[i];
    if(i)memmove(l->items+1,l->items,i*sizeof(*l->items));
    l->items[0]=a;return true;
}
static bool snapshot(adapter_list *l,const char *preferred)
{
    bool setup=false,dos=false;
    return enumerate_setup(l,&setup)&&enumerate_dos(l,&dos)&&(setup||dos)&&order(l,preferred);
}
bool mems_manager_list_serial_adapters(const char *preferred_device,
    mems_manager_serial_adapter *out,size_t capacity,size_t *total)
{
    adapter_list l={0};size_t n;
    if(!total||(!out&&capacity))return false;
    *total=0;
    if(!snapshot(&l,preferred_device)){free(l.items);return false;}
    n=l.count<capacity?l.count:capacity;
    if(n)memcpy(out,l.items,n*sizeof(*out));
    *total=l.count;free(l.items);return true;
}
bool mems_manager_get_adapter_details(const char *device,mems_manager_serial_adapter *out)
{
    adapter_list l={0};char port[16];bool found=false;
    if(!out||!normalize(device,port))return false;
    if(snapshot(&l,NULL))
        for(size_t i=0;i<l.count;++i)if(strcmp(port,l.items[i].port)==0){
            *out=l.items[i];found=true;break;
        }
    free(l.items);return found;
}
