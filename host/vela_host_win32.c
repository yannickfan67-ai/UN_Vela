#ifndef _WIN32
#error "vela_host_win32.c is for Windows hosts"
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include <string.h>
#include "vela_host.h"

static void text_copy(char*d,size_t cap,const char*s){if(!d||!cap)return;size_t i=0;while(s&&s[i]&&i+1<cap){d[i]=s[i];i++;}d[i]=0;}
static uint64_t win_time_ms(void*context){(void)context;return (uint64_t)GetTickCount64();}
static void win_log(void*context,uint32_t level,const char*message){(void)context;fprintf(stderr,"UN_Vela[%u]: %s\n",(unsigned)level,message?message:"");}
static int utf8_to_wide(const char*s,wchar_t*out,int cap){if(!s||!out||cap<2)return 0;int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s,-1,out,cap);return n>0;}
static void status_error(char*status,size_t cap,const char*prefix){char b[96];DWORD e=GetLastError();_snprintf_s(b,sizeof(b),_TRUNCATE,"%s (%lu)",prefix,(unsigned long)e);text_copy(status,cap,b);}
static int win_fetch(const char*url,uint8_t*data,size_t cap,size_t*len,char*type,size_t type_cap,char*status,size_t status_cap){
    if(len)*len=0;if(type&&type_cap)type[0]=0;if(status&&status_cap)status[0]=0;if(!url||!data||!cap)return 0;
    wchar_t wurl[1024];if(!utf8_to_wide(url,wurl,(int)(sizeof(wurl)/sizeof(wurl[0])))){text_copy(status,status_cap,"Invalid UTF-8 URL");return 0;}
    URL_COMPONENTS uc;memset(&uc,0,sizeof(uc));uc.dwStructSize=sizeof(uc);uc.dwSchemeLength=(DWORD)-1;uc.dwHostNameLength=(DWORD)-1;uc.dwUrlPathLength=(DWORD)-1;uc.dwExtraInfoLength=(DWORD)-1;
    if(!WinHttpCrackUrl(wurl,0,0,&uc)){status_error(status,status_cap,"WinHttpCrackUrl failed");return 0;}
    if(uc.nScheme!=INTERNET_SCHEME_HTTP&&uc.nScheme!=INTERNET_SCHEME_HTTPS){text_copy(status,status_cap,"Unsupported URL scheme");return 0;}
    wchar_t host[256];if(uc.dwHostNameLength==0||uc.dwHostNameLength>=sizeof(host)/sizeof(host[0])){text_copy(status,status_cap,"Host too long");return 0;}memcpy(host,uc.lpszHostName,uc.dwHostNameLength*sizeof(wchar_t));host[uc.dwHostNameLength]=0;
    wchar_t path[1024];size_t po=0;if(uc.dwUrlPathLength){if(uc.dwUrlPathLength>=sizeof(path)/sizeof(path[0]))return 0;memcpy(path,uc.lpszUrlPath,uc.dwUrlPathLength*sizeof(wchar_t));po=uc.dwUrlPathLength;}else path[po++]=L'/';if(uc.dwExtraInfoLength&&po+uc.dwExtraInfoLength+1<sizeof(path)/sizeof(path[0])){memcpy(path+po,uc.lpszExtraInfo,uc.dwExtraInfoLength*sizeof(wchar_t));po+=uc.dwExtraInfoLength;}path[po]=0;
    HINTERNET session=WinHttpOpen(L"UN_Vela/0.3",WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);if(!session){status_error(status,status_cap,"WinHttpOpen failed");return 0;}WinHttpSetTimeouts(session,5000,5000,12000,12000);
    HINTERNET connect=WinHttpConnect(session,host,uc.nPort,0);if(!connect){status_error(status,status_cap,"WinHttpConnect failed");WinHttpCloseHandle(session);return 0;}
    DWORD flags=uc.nScheme==INTERNET_SCHEME_HTTPS?WINHTTP_FLAG_SECURE:0;const wchar_t*accept[]={L"text/html",L"text/plain",L"image/*",L"*/*",NULL};HINTERNET req=WinHttpOpenRequest(connect,L"GET",path,NULL,WINHTTP_NO_REFERER,accept,flags);if(!req){status_error(status,status_cap,"WinHttpOpenRequest failed");WinHttpCloseHandle(connect);WinHttpCloseHandle(session);return 0;}
    BOOL ok=WinHttpSendRequest(req,WINHTTP_NO_ADDITIONAL_HEADERS,0,WINHTTP_NO_REQUEST_DATA,0,0,0)&&WinHttpReceiveResponse(req,NULL);if(!ok){status_error(status,status_cap,"HTTPS request failed");WinHttpCloseHandle(req);WinHttpCloseHandle(connect);WinHttpCloseHandle(session);return 0;}
    DWORD code=0,code_len=sizeof(code);WinHttpQueryHeaders(req,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&code,&code_len,WINHTTP_NO_HEADER_INDEX);
    wchar_t ctype[160];DWORD ctlen=sizeof(ctype);if(WinHttpQueryHeaders(req,WINHTTP_QUERY_CONTENT_TYPE,WINHTTP_HEADER_NAME_BY_INDEX,ctype,&ctlen,WINHTTP_NO_HEADER_INDEX)&&type&&type_cap){int n=WideCharToMultiByte(CP_UTF8,0,ctype,-1,type,(int)type_cap,NULL,NULL);if(!n)type[0]=0;}
    size_t used=0;for(;;){DWORD avail=0;if(!WinHttpQueryDataAvailable(req,&avail)){ok=FALSE;break;}if(!avail)break;if((size_t)avail>cap-used){text_copy(status,status_cap,"Response too large");ok=FALSE;break;}DWORD got=0;if(!WinHttpReadData(req,data+used,avail,&got)){ok=FALSE;break;}used+=got;if(got==0)break;}
    if(len)*len=used;if(status&&status_cap&&ok){char b[64];_snprintf_s(b,sizeof(b),_TRUNCATE,"HTTP %lu",(unsigned long)code);text_copy(status,status_cap,b);}if(!ok&&status&&status_cap&&!status[0])status_error(status,status_cap,"WinHTTP read failed");
    WinHttpCloseHandle(req);WinHttpCloseHandle(connect);WinHttpCloseHandle(session);return ok&&code>=200&&code<400;
}
static int win_resource_get(void*context,const char*url,uint8_t*data,size_t cap,size_t*len,char*type,size_t type_cap,char*status,size_t status_cap){(void)context;return win_fetch(url,data,cap,len,type,type_cap,status,status_cap);}
static int win_http_get(void*context,const char*url,char*body,size_t cap,char*status,size_t status_cap){size_t n=0;char type[96];if(!body||cap<2)return 0;if(!win_resource_get(context,url,(uint8_t*)body,cap-1,&n,type,sizeof(type),status,status_cap))return 0;body[n]=0;return 1;}
int vela_hosted_init(VelaHostedCarrier*carrier){if(!carrier)return 0;memset(carrier,0,sizeof(*carrier));carrier->ops.abi_version=VELA_PLATFORM_ABI_VERSION;carrier->ops.struct_size=(uint32_t)sizeof(carrier->ops);carrier->ops.capabilities=VELA_PLATFORM_CAP_HTTP|VELA_PLATFORM_CAP_TLS|VELA_PLATFORM_CAP_RESOURCES|VELA_PLATFORM_CAP_LOG|VELA_PLATFORM_CAP_TIME;carrier->ops.http_get=win_http_get;carrier->ops.resource_get=win_resource_get;carrier->ops.log=win_log;carrier->ops.time_ms=win_time_ms;carrier->initialized=1;return 1;}
void vela_hosted_shutdown(VelaHostedCarrier*carrier){if(carrier)memset(carrier,0,sizeof(*carrier));}
const char*vela_hosted_name(void){return "Windows/WinHTTP TLS";}
