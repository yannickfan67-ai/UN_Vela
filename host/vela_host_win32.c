#ifndef _WIN32
#error "vela_host_win32.c is for Windows hosts"
#endif

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vela_host.h"
#include "vela_http_common.h"

static uint64_t win_time_ms(void *context){(void)context;return (uint64_t)GetTickCount64();}
static void win_log(void *context,uint32_t level,const char *message){(void)context;fprintf(stderr,"UN_Vela[%u]: %s\n",(unsigned)level,message?message:"");}
static SOCKET connect_target(const VelaHttpTarget*t){
    struct addrinfo hints,*list=0,*it=0;memset(&hints,0,sizeof(hints));hints.ai_socktype=SOCK_STREAM;hints.ai_family=AF_UNSPEC;
    if(getaddrinfo(t->host,t->port,&hints,&list)!=0)return INVALID_SOCKET;
    SOCKET s=INVALID_SOCKET;
    for(it=list;it;it=it->ai_next){
        s=socket(it->ai_family,it->ai_socktype,it->ai_protocol);if(s==INVALID_SOCKET)continue;
        DWORD timeout=5000;(void)setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));(void)setsockopt(s,SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeout,sizeof(timeout));
        if(connect(s,it->ai_addr,(int)it->ai_addrlen)==0)break;closesocket(s);s=INVALID_SOCKET;
    }
    freeaddrinfo(list);return s;
}
static int send_all(SOCKET s,const char*buf,size_t len){
    size_t off=0;while(off<len){size_t remain=len-off;int ask=remain>0x7fffffffU?0x7fffffff:(int)remain;int n=send(s,buf+off,ask,0);if(n==SOCKET_ERROR||n==0)return 0;off+=(size_t)n;}return 1;
}
static int win_http_get(void *context,const char *url,char *body,size_t body_cap,char *status,size_t status_cap){
    (void)context;VelaHttpTarget t;if(!vela_host_parse_http_url(url,&t)){vela_host_copy(status,status_cap,"Unsupported URL");return 0;}
    SOCKET s=connect_target(&t);if(s==INVALID_SOCKET){vela_host_copy(status,status_cap,"Connect failed");return 0;}
    char req[1600];int rn=_snprintf_s(req,sizeof(req),_TRUNCATE,"GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: UN_Vela/0.2 hosted-win32\r\nAccept: text/html,text/plain;q=0.9,*/*;q=0.1\r\nAccept-Encoding: identity\r\nConnection: close\r\n\r\n",t.path,t.host);
    if(rn<=0||!send_all(s,req,(size_t)rn)){closesocket(s);vela_host_copy(status,status_cap,"Send failed");return 0;}
    size_t raw_cap=body_cap+VELA_HOST_HEADER_SLACK;if(raw_cap<body_cap){closesocket(s);vela_host_copy(status,status_cap,"Response too large");return 0;}
    char*raw=(char*)malloc(raw_cap);if(!raw){closesocket(s);vela_host_copy(status,status_cap,"Out of memory");return 0;}
    size_t used=0;while(used+1<raw_cap){size_t remain=raw_cap-1-used;int ask=remain>0x7fffffffU?0x7fffffff:(int)remain;int n=recv(s,raw+used,ask,0);if(n==0)break;if(n==SOCKET_ERROR)break;used+=(size_t)n;}
    closesocket(s);raw[used]=0;int ok=vela_host_extract_body(raw,used,body,body_cap,status,status_cap);free(raw);if(!ok&&status&&status_cap&&!status[0])vela_host_copy(status,status_cap,"Malformed HTTP response");return ok;
}
int vela_hosted_init(VelaHostedCarrier *carrier){
    if(!carrier)return 0;memset(carrier,0,sizeof(*carrier));WSADATA data;if(WSAStartup(MAKEWORD(2,2),&data)!=0)return 0;
    carrier->private_state[0]=1;carrier->ops.abi_version=VELA_PLATFORM_ABI_VERSION;carrier->ops.struct_size=(uint32_t)sizeof(carrier->ops);carrier->ops.capabilities=VELA_PLATFORM_CAP_HTTP|VELA_PLATFORM_CAP_LOG|VELA_PLATFORM_CAP_TIME;carrier->ops.http_get=win_http_get;carrier->ops.log=win_log;carrier->ops.time_ms=win_time_ms;carrier->initialized=1;return 1;
}
void vela_hosted_shutdown(VelaHostedCarrier *carrier){if(!carrier)return;if(carrier->private_state[0])WSACleanup();memset(carrier,0,sizeof(*carrier));}
const char *vela_hosted_name(void){return "Windows/WinSock";}
