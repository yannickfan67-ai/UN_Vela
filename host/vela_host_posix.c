#if defined(_WIN32)
#error "vela_host_posix.c is for POSIX hosts"
#endif
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <curl/curl.h>
#include "vela_host.h"

typedef struct {uint8_t *data;size_t cap,used;int overflow;} WriteBuffer;
static size_t curl_write(void *ptr,size_t size,size_t nmemb,void *userdata){WriteBuffer*w=(WriteBuffer*)userdata;size_t n=size*nmemb;if(size&&n/size!=nmemb)return 0;if(n>w->cap-w->used){w->overflow=1;return 0;}memcpy(w->data+w->used,ptr,n);w->used+=n;return n;}
static void text_copy(char*d,size_t cap,const char*s){if(!d||!cap)return;size_t i=0;while(s&&s[i]&&i+1<cap){d[i]=s[i];i++;}d[i]=0;}
static int curl_fetch(const char*url,uint8_t*data,size_t cap,size_t*len,char*type,size_t type_cap,char*status,size_t status_cap){if(!url||!data||!cap)return 0;CURL*c=curl_easy_init();if(!c){text_copy(status,status_cap,"curl init failed");return 0;}WriteBuffer w={data,cap,0,0};curl_easy_setopt(c,CURLOPT_URL,url);curl_easy_setopt(c,CURLOPT_FOLLOWLOCATION,1L);curl_easy_setopt(c,CURLOPT_MAXREDIRS,5L);curl_easy_setopt(c,CURLOPT_CONNECTTIMEOUT_MS,5000L);curl_easy_setopt(c,CURLOPT_TIMEOUT_MS,12000L);curl_easy_setopt(c,CURLOPT_USERAGENT,"UN_Vela/0.3 hosted-posix");curl_easy_setopt(c,CURLOPT_ACCEPT_ENCODING,"");curl_easy_setopt(c,CURLOPT_WRITEFUNCTION,curl_write);curl_easy_setopt(c,CURLOPT_WRITEDATA,&w);curl_easy_setopt(c,CURLOPT_SSL_VERIFYPEER,1L);curl_easy_setopt(c,CURLOPT_SSL_VERIFYHOST,2L);CURLcode rc=curl_easy_perform(c);long code=0;curl_easy_getinfo(c,CURLINFO_RESPONSE_CODE,&code);char*ct=0;curl_easy_getinfo(c,CURLINFO_CONTENT_TYPE,&ct);if(type&&type_cap)text_copy(type,type_cap,ct?ct:"");if(len)*len=w.used;if(status&&status_cap){char tmp[64];snprintf(tmp,sizeof(tmp),"HTTP %ld",code);text_copy(status,status_cap,rc==CURLE_OK?tmp:curl_easy_strerror(rc));}int ok=rc==CURLE_OK&&!w.overflow&&code>=200&&code<400;curl_easy_cleanup(c);return ok;}
static uint64_t posix_time_ms(void*context){(void)context;struct timeval tv;if(gettimeofday(&tv,0)!=0)return 0;return (uint64_t)tv.tv_sec*1000ull+(uint64_t)(tv.tv_usec/1000);}
static void posix_log(void*context,uint32_t level,const char*message){(void)context;fprintf(stderr,"UN_Vela[%u]: %s\n",(unsigned)level,message?message:"");}
static int posix_resource_get(void*context,const char*url,uint8_t*data,size_t cap,size_t*len,char*type,size_t type_cap,char*status,size_t status_cap){(void)context;return curl_fetch(url,data,cap,len,type,type_cap,status,status_cap);}
static int posix_http_get(void*context,const char*url,char*body,size_t cap,char*status,size_t status_cap){size_t n=0;char type[96];if(!body||cap<2)return 0;if(!posix_resource_get(context,url,(uint8_t*)body,cap-1,&n,type,sizeof(type),status,status_cap))return 0;body[n]=0;return 1;}
int vela_hosted_init(VelaHostedCarrier*carrier){if(!carrier)return 0;memset(carrier,0,sizeof(*carrier));if(curl_global_init(CURL_GLOBAL_DEFAULT)!=CURLE_OK)return 0;carrier->ops.abi_version=VELA_PLATFORM_ABI_VERSION;carrier->ops.struct_size=(uint32_t)sizeof(carrier->ops);carrier->ops.capabilities=VELA_PLATFORM_CAP_HTTP|VELA_PLATFORM_CAP_TLS|VELA_PLATFORM_CAP_RESOURCES|VELA_PLATFORM_CAP_LOG|VELA_PLATFORM_CAP_TIME;carrier->ops.http_get=posix_http_get;carrier->ops.resource_get=posix_resource_get;carrier->ops.log=posix_log;carrier->ops.time_ms=posix_time_ms;carrier->initialized=1;return 1;}
void vela_hosted_shutdown(VelaHostedCarrier*carrier){if(carrier&&carrier->initialized){carrier->initialized=0;curl_global_cleanup();}}
const char*vela_hosted_name(void){
#if defined(__APPLE__)
return "macOS/libcurl TLS";
#elif defined(__linux__)
return "Linux/libcurl TLS";
#else
return "POSIX/libcurl TLS";
#endif
}
