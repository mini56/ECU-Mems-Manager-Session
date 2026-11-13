#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#include <cstdio>
#include <string>
#pragma comment(lib, "winhttp.lib")
struct Handle {
    HINTERNET value=nullptr;
    explicit Handle(HINTERNET p=nullptr):value(p){}
    ~Handle(){if(value)WinHttpCloseHandle(value);}
};
int main(){
    Handle session(WinHttpOpen(L"ECU-MEMS-Manager-SESSION/141",
         WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,
         WINHTTP_NO_PROXY_BYPASS,0));
    if(!session.value){std::fprintf(stderr,"WINHTTP_OPEN_ERROR=%lu\n",GetLastError());return 1;}
    WinHttpSetTimeouts(session.value,10000,10000,10000,30000);
    Handle connection(WinHttpConnect(session.value,L"raw.githubusercontent.com",443,0));
    if(!connection.value){std::fprintf(stderr,"WINHTTP_CONNECT_ERROR=%lu\n",GetLastError());return 2;}
    Handle request(WinHttpOpenRequest(connection.value,L"GET",
        L"/mini56/ECU-Mems-Manager-Session-MAJ/main/manifest.json",nullptr,
        WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE));
    if(!request.value){std::fprintf(stderr,"WINHTTP_REQUEST_ERROR=%lu\n",GetLastError());return 3;}
    if(!WinHttpSendRequest(request.value,WINHTTP_NO_ADDITIONAL_HEADERS,0,
       WINHTTP_NO_REQUEST_DATA,0,0,0) || !WinHttpReceiveResponse(request.value,nullptr)){
        std::fprintf(stderr,"WINHTTP_SEND_OR_RESPONSE_ERROR=%lu\n",GetLastError());return 4;}
    DWORD code=0,len=sizeof(code);
    if(!WinHttpQueryHeaders(request.value,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,
                         WINHTTP_HEADER_NAME_BY_INDEX,&code,&len,WINHTTP_NO_HEADER_INDEX)){
        std::fprintf(stderr,"WINHTTP_STATUS_ERROR=%lu\n",GetLastError());return 5;}
    std::string body;
    for(;;){
        DWORD available=0;
        if(!WinHttpQueryDataAvailable(request.value,&available)){std::fprintf(stderr,"WINHTTP_READ_AVAILABLE_ERROR=%lu\n",GetLastError());return 6;}
        if(!available)break;
        std::string block;
        block.resize(available);
        DWORD got=0;
        if(!WinHttpReadData(request.value,&block[0],available,&got)){std::fprintf(stderr,"WINHTTP_READ_ERROR=%lu\n",GetLastError());return 7;}
        if(!got)break;
        body.append(block.data(),got);
    }
    std::fprintf(stderr,"NATIVE_HTTPS_STATUS=%lu\nNATIVE_HTTPS_BYTES=%zu\n",code,body.size());
    if(code!=200 || body.find("\"documentary_bases\"")==std::string::npos)return 8;
    std::fprintf(stderr,"NATIVE_WINDOWS_HTTPS_MANIFEST_PASS\n");
    return 0;
}
#else
int main(){return 0;}
#endif
