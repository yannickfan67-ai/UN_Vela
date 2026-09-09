#if defined(_WIN32)
#error "vela_host_posix.c is for POSIX hosts"
#endif

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <unistd.h>
#include "vela_host.h"
#include "vela_http_common.h"

static uint64_t posix_time_ms(void *context){
    (void)context;struct timeval tv;if(gettimeofday(&tv,0)!=0)return 0;
    return (uint64_t)tv.tv_sec*1000ull+(uint64_t)(tv.tv_usec/1000);
}
static void posix_log(void *context,uint32_t level,const char *message){
    (void)context;fprintf(stderr,"UN_Vela[%u]: %s\n",(unsigned)level,message?message:"");
}
static int connect_target(const VelaHttpTarget*t){
    struct addrinfo hints,*list=0,*it=0;memset(&hints,0,sizeof(hints));hints.ai_socktype=SOCK_STREAM;hints.ai_family=AF_UNSPEC;
    if(getaddrinfo(t->host,t->port,&hints,&list)!=0)return -1;
    int fd=-1;
    for(it=list;it;it=it->ai_next){
        fd=socket(it->ai_family,it->ai_socktype,it->ai_protocol);if(fd<0)continue;
        struct timeval timeout;timeout.tv_sec=5;timeout.tv_usec=0;
        (void)setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
        (void)setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));
        if(connect(fd,it->ai_addr,it->ai_addrlen)==0)break;close(fd);fd=-1;
    }
    freeaddrinfo(list);return fd;
}
static int send_all(int fd,const char*buf,size_t len){
    size_t off=0;while(off<len){ssize_t n=send(fd,buf+off,len-off,0);if(n<=0)return 0;off+=(size_t)n;}return 1;
}
static int posix_http_get(void *context,const char *url,char *body,size_t body_cap,char *status,size_t status_cap){
    (void)context;VelaHttpTarget t;if(!vela_host_parse_http_url(url,&t)){vela_host_copy(status,status_cap,"Unsupported URL");return 0;}
    int fd=connect_target(&t);if(fd<0){vela_host_copy(status,status_cap,"Connect failed");return 0;}
    char req[1600];int rn=snprintf(req,sizeof(req),"GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: UN_Vela/0.2 hosted-posix\r\nAccept: text/html,text/plain;q=0.9,*/*;q=0.1\r\nAccept-Encoding: identity\r\nConnection: close\r\n\r\n",t.path,t.host);
    if(rn<=0||(size_t)rn>=sizeof(req)||!send_all(fd,req,(size_t)rn)){close(fd);vela_host_copy(status,status_cap,"Send failed");return 0;}
    size_t raw_cap=body_cap+VELA_HOST_HEADER_SLACK;if(raw_cap<body_cap){close(fd);vela_host_copy(status,status_cap,"Response too large");return 0;}
    char*raw=(char*)malloc(raw_cap);if(!raw){close(fd);vela_host_copy(status,status_cap,"Out of memory");return 0;}
    size_t used=0;while(used+1<raw_cap){ssize_t n=recv(fd,raw+used,raw_cap-1-used,0);if(n==0)break;if(n<0){if(errno==EINTR)continue;break;}used+=(size_t)n;}
    close(fd);raw[used]=0;int ok=vela_host_extract_body(raw,used,body,body_cap,status,status_cap);free(raw);if(!ok&&status&&status_cap&&!status[0])vela_host_copy(status,status_cap,"Malformed HTTP response");return ok;
}
int vela_hosted_init(VelaHostedCarrier *carrier){
    if(!carrier)return 0;memset(carrier,0,sizeof(*carrier));carrier->ops.abi_version=VELA_PLATFORM_ABI_VERSION;carrier->ops.struct_size=(uint32_t)sizeof(carrier->ops);carrier->ops.capabilities=VELA_PLATFORM_CAP_HTTP|VELA_PLATFORM_CAP_LOG|VELA_PLATFORM_CAP_TIME;carrier->ops.http_get=posix_http_get;carrier->ops.log=posix_log;carrier->ops.time_ms=posix_time_ms;carrier->initialized=1;return 1;
}
void vela_hosted_shutdown(VelaHostedCarrier *carrier){if(carrier)carrier->initialized=0;}
const char *vela_hosted_name(void){
#if defined(__APPLE__)
    return "macOS/POSIX";
#elif defined(__linux__)
    return "Linux/POSIX";
#else
    return "POSIX";
#endif
}
