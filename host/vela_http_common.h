#ifndef UN_VELA_HTTP_COMMON_H
#define UN_VELA_HTTP_COMMON_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define VELA_HOST_HEADER_SLACK 16384u

typedef struct VelaHttpTarget {
    char host[256];
    char port[8];
    char path[1024];
} VelaHttpTarget;

static char vela_host_lower(char c){return c>='A'&&c<='Z'?(char)(c+('a'-'A')):c;}
static int vela_host_starts_ci(const char*s,const char*p){while(*p){if(!*s||vela_host_lower(*s++)!=vela_host_lower(*p++))return 0;}return 1;}
static void vela_host_copy(char*d,size_t cap,const char*s){size_t i=0;if(!d||!cap)return;while(s&&s[i]&&i+1<cap){d[i]=s[i];i++;}d[i]=0;}
static int vela_host_contains_ci(const char*s,size_t n,const char*needle){size_t nn=0;while(needle[nn])nn++;if(!nn||nn>n)return 0;for(size_t i=0;i+nn<=n;i++){size_t j=0;while(j<nn&&vela_host_lower(s[i+j])==vela_host_lower(needle[j]))j++;if(j==nn)return 1;}return 0;}
static int vela_host_hex(char c){if(c>='0'&&c<='9')return c-'0';c=vela_host_lower(c);if(c>='a'&&c<='f')return c-'a'+10;return -1;}

static int vela_host_parse_http_url(const char*url,VelaHttpTarget*out){
    if(!url||!out||!vela_host_starts_ci(url,"http://"))return 0;
    const char*s=url+7;size_t h=0,p=0;
    while(*s&&*s!='/'&&*s!=':'&&h+1<sizeof(out->host))out->host[h++]=*s++;
    out->host[h]=0;if(!h)return 0;
    vela_host_copy(out->port,sizeof(out->port),"80");
    if(*s==':'){
        s++;size_t q=0;while(*s>='0'&&*s<='9'&&q+1<sizeof(out->port))out->port[q++]=*s++;
        out->port[q]=0;if(!q)return 0;
    }
    if(!*s){vela_host_copy(out->path,sizeof(out->path),"/");return 1;}
    while(*s&&p+1<sizeof(out->path))out->path[p++]=*s++;
    out->path[p]=0;
    return out->path[0]=='/';
}

static size_t vela_host_find_body(const char*raw,size_t len,size_t*header_len){
    for(size_t i=0;i+3<len;i++)if(raw[i]=='\r'&&raw[i+1]=='\n'&&raw[i+2]=='\r'&&raw[i+3]=='\n'){if(header_len)*header_len=i;return i+4;}
    for(size_t i=0;i+1<len;i++)if(raw[i]=='\n'&&raw[i+1]=='\n'){if(header_len)*header_len=i;return i+2;}
    return 0;
}
static void vela_host_status(const char*raw,size_t len,char*status,size_t cap){
    if(!status||!cap)return;size_t i=0;while(i<len&&raw[i]!='\r'&&raw[i]!='\n'&&i+1<cap){status[i]=raw[i];i++;}status[i]=0;
}
static size_t vela_host_decode_chunked(const char*src,size_t len,char*out,size_t cap){
    size_t p=0,o=0;if(!out||!cap)return 0;
    while(p<len){
        size_t chunk=0;int digits=0;
        while(p<len&&src[p]!='\r'&&src[p]!='\n'){
            if(src[p]==';'){while(p<len&&src[p]!='\r'&&src[p]!='\n')p++;break;}
            int h=vela_host_hex(src[p++]);if(h<0)return 0;chunk=chunk*16u+(size_t)h;digits++;if(chunk>0x1000000u)return 0;
        }
        if(!digits)return 0;if(p<len&&src[p]=='\r')p++;if(p<len&&src[p]=='\n')p++;
        if(chunk==0){out[o]=0;return o;}if(chunk>len-p)return 0;
        size_t room=cap-1-o,copy=chunk<room?chunk:room;if(copy){memcpy(out+o,src+p,copy);o+=copy;}p+=chunk;
        if(p<len&&src[p]=='\r')p++;if(p<len&&src[p]=='\n')p++;
    }
    out[o]=0;return o;
}
static int vela_host_extract_body(const char*raw,size_t len,char*body,size_t body_cap,char*status,size_t status_cap){
    if(!raw||!body||body_cap<2)return 0;vela_host_status(raw,len,status,status_cap);
    size_t header_len=0,body_off=vela_host_find_body(raw,len,&header_len);if(!body_off)return 0;
    const char*src=raw+body_off;size_t src_len=len-body_off;
    if(src_len>=3&&(uint8_t)src[0]==0xEF&&(uint8_t)src[1]==0xBB&&(uint8_t)src[2]==0xBF){src+=3;src_len-=3;}
    if(vela_host_contains_ci(raw,header_len,"transfer-encoding: chunked")){
        size_t n=vela_host_decode_chunked(src,src_len,body,body_cap);return n>0||src_len==0;
    }
    size_t n=src_len<body_cap-1?src_len:body_cap-1;if(n)memcpy(body,src,n);body[n]=0;return 1;
}

#endif
