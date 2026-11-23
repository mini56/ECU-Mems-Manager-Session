#ifndef ECU_COMMUNICATION_TRACE_IO_H
#define ECU_COMMUNICATION_TRACE_IO_H
/* Win32 raw-byte recorder; never modifies serial commands or replies. */
#include <stdint.h>
#include <stddef.h>
#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
static inline void ecu_trace_bytes(const char *direction,const uint8_t *bytes,size_t size)
{
    if(!bytes||!size)return;
    wchar_t path[4096];
    DWORD n=GetEnvironmentVariableW(L"ECU_MEMS_COMM_TRACE_FILE",path,4096);
    if(!n||n>=4096)return;
    HANDLE f=CreateFileW(path,FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                         NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(f==INVALID_HANDLE_VALUE)return;
    SYSTEMTIME t;GetLocalTime(&t);
    for(size_t offset=0;offset<size;offset+=96){
        size_t k=size-offset>96?96:size-offset;
        char line[420];
        int used=snprintf(line,sizeof(line),"%04u-%02u-%02u %02u:%02u:%02u.%03u | %s |",
            (unsigned)t.wYear,(unsigned)t.wMonth,(unsigned)t.wDay,(unsigned)t.wHour,
            (unsigned)t.wMinute,(unsigned)t.wSecond,(unsigned)t.wMilliseconds,direction);
        if(used<0||used>=400)break;
        for(size_t i=0;i<k;++i)
            used+=snprintf(line+used,sizeof(line)-(size_t)used," %02X",(unsigned)bytes[offset+i]);
        line[used++]='\r';line[used++]='\n';
        DWORD written=0;WriteFile(f,line,(DWORD)used,&written,NULL);
    }
    CloseHandle(f);
}
#ifdef __cplusplus
static inline BOOL ecu_trace_read_file(HANDLE h,LPVOID bytes,DWORD qty,LPDWORD got,LPOVERLAPPED ov)
{
    BOOL ok=::ReadFile(h,bytes,qty,got,ov);
    if(ok&&bytes&&got&&*got)ecu_trace_bytes("RX",(const uint8_t*)bytes,*got);
    return ok;
}
static inline BOOL ecu_trace_write_file(HANDLE h,LPCVOID bytes,DWORD qty,LPDWORD got,LPOVERLAPPED ov)
{
    BOOL ok=::WriteFile(h,bytes,qty,got,ov);
    if(bytes&&got&&*got)ecu_trace_bytes("TX",(const uint8_t*)bytes,*got);
    return ok;
}
#endif
#else
static inline void ecu_trace_bytes(const char *dir,const uint8_t *data,size_t n)
{(void)dir;(void)data;(void)n;}
#endif
#endif
