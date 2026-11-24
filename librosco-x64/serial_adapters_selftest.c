/* Tests production implementation, pure metadata + real read-only OS enumeration.
 * No serial port is opened, no hardware is required. */
#include "serial_adapters.c"
static int failures;
#define CHECK(x) do{if(!(x)){fprintf(stderr,"SERIAL_ADAPTER_TEST_FAIL %d %s\n",__LINE__,#x);++failures;}}while(0)
int main(void)
{
    bool hint;char port[16];mems_manager_serial_adapter a,b;
    adapter_list l={0};size_t total=0;
    CHECK(normalize(" \\\\.\\com010 ",port)&&strcmp(port,"COM10")==0);
    CHECK(!normalize("COM0",port));CHECK(!normalize("COM65536",port));
    CHECK(!normalize("COM4x",port));CHECK(!normalize("LPT1",port));
    CHECK(!normalize("COM",port));CHECK(!normalize(NULL,port));
    CHECK(mems_manager_detect_adapter_family(0x0403,true,0x6001,true,NULL,NULL,&hint)==MEMS_MANAGER_ADAPTER_FTDI&&!hint);
    CHECK(mems_manager_detect_adapter_family(0x1A86,true,0x7523,true,NULL,NULL,&hint)==MEMS_MANAGER_ADAPTER_WCH&&hint);
    CHECK(mems_manager_detect_adapter_family(0x1A86,true,0x5523,true,NULL,NULL,&hint)==MEMS_MANAGER_ADAPTER_WCH&&!hint);
    CHECK(mems_manager_detect_adapter_family(0,false,0,false,"prolific",NULL,&hint)==MEMS_MANAGER_ADAPTER_PROLIFIC&&!hint);
    CHECK(mems_manager_detect_adapter_family(0,false,0,false,NULL,"CP2102",&hint)==MEMS_MANAGER_ADAPTER_CP210X);
    CHECK(mems_manager_detect_adapter_family(0x9999,true,0,false,NULL,NULL,NULL)==MEMS_MANAGER_ADAPTER_USB_OTHER);
    CHECK(mems_manager_detect_adapter_family(0,false,0,false,NULL,NULL,NULL)==MEMS_MANAGER_ADAPTER_SERIAL);
    CHECK(make_adapter("COM12",true,&a));CHECK(make_adapter("COM2",true,&b));
    absorb_ids(&a,L"FTDIBUS\\VID_0403+PID_6001+ABC");
    CHECK(a.has_vendor_id&&a.has_product_id&&a.vendor_id==0x0403&&a.product_id==0x6001);
    classify(&a);CHECK(a.family==MEMS_MANAGER_ADAPTER_FTDI);
    CHECK(append(&l,&a)&&append(&l,&b)&&append(&l,&a)&&l.count==2);
    CHECK(order(&l,NULL)&&strcmp(l.items[0].port,"COM2")==0);
    CHECK(order(&l,"\\\\.\\COM12")&&strcmp(l.items[0].port,"COM12")==0);
    CHECK(order(&l,"COM99")&&l.count==3&&!l.items[0].detected_by_system&&l.items[0].family==MEMS_MANAGER_ADAPTER_CONFIGURED);
    free(l.items);
    CHECK(!mems_manager_list_serial_adapters(NULL,NULL,1,&total));
    CHECK(!mems_manager_get_adapter_details("invalid",&a));
    CHECK(mems_manager_list_serial_adapters("COM65535",NULL,0,&total)&&total>=1);
    CHECK(mems_manager_list_serial_adapters("COM65535",&a,1,&total)&&total>=1);
    CHECK(strcmp(a.port,"COM65535")==0&&strcmp(a.device_path,"\\\\.\\COM65535")==0);
    if(!a.detected_by_system)CHECK(!mems_manager_get_adapter_details("COM65535",&b));
    if(failures)return 1;
    puts("SERIAL_ADAPTER_TEST_PASS: normalization, USB classification, preference, deduplication and read-only Windows enumeration");
    return 0;
}
