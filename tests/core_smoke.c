#include <stdio.h>
#include <string.h>
#include "vela.h"

static char last_resource[320];
static int fake_http(void*ctx,const char*url,char*body,size_t cap,char*status,size_t sc){(void)ctx;(void)url;snprintf(body,cap,"<html><head><title>Parsed</title></head><body><a href='/next'>next</a><script>document.title='JS Title';</script></body></html>");snprintf(status,sc,"HTTP 200");return 1;}
static int fake_resource(void*ctx,const char*url,uint8_t*data,size_t cap,size_t*len,char*type,size_t tc,char*status,size_t sc){(void)ctx;snprintf(last_resource,sizeof(last_resource),"%s",url);if(cap)data[0]=0x42;if(len)*len=cap?1:0;if(type&&tc)snprintf(type,tc,"image/bmp");if(status&&sc)snprintf(status,sc,"HTTP 200");return cap>0;}

void aster_document_init(AsterDocument*d){memset(d,0,sizeof(*d));}
int aster_parse_html(AsterDocument*d,const char*html){(void)html;aster_document_init(d);snprintf(d->title,sizeof(d->title),"Parsed");d->document_height=500;return 1;}
void aster_layout(AsterDocument*d,int width){(void)width;if(d)d->document_height=500;}
void aster_paint(const AsterDocument*d,int x,int y,int w,int h,int scroll){(void)d;(void)x;(void)y;(void)w;(void)h;(void)scroll;}
int aster_link_at(const AsterDocument*d,int x,int y,int scroll,char*url,size_t cap){(void)d;(void)x;(void)y;(void)scroll;snprintf(url,cap,"/next");return 1;}
int aster_document_height(const AsterDocument*d){return d?d->document_height:0;}
uint32_t aster_api_version(void){return ASTER_API_VERSION;}
const char*aster_version(void){return ASTER_VERSION;}

int main(void){
    VelaPlatformOps p;memset(&p,0,sizeof(p));p.abi_version=VELA_PLATFORM_ABI_VERSION;p.struct_size=sizeof(p);p.capabilities=VELA_PLATFORM_CAP_HTTP|VELA_PLATFORM_CAP_TLS|VELA_PLATFORM_CAP_RESOURCES;p.http_get=fake_http;p.resource_get=fake_resource;
    if(!vela_init_ex(640,&p,0))return 1;
    vela_set_viewport_size(640,240);
    if(!vela_load_url("https://example.test/a/page.html"))return 2;
    if(strcmp(vela_title(),"JS Title")!=0)return 3;
    if(!vela_activate_link(1,1)||strcmp(vela_url(),"https://example.test/next")!=0)return 4;
    uint8_t data[8];size_t n=0;char type[32],status[32];
    if(!vela_resource_get("asset.bmp",data,sizeof(data),&n,type,sizeof(type),status,sizeof(status)))return 5;
    if(strcmp(last_resource,"https://example.test/asset.bmp")!=0||n!=1||data[0]!=0x42)return 6;
    vela_set_features(VELA_PROFILE_LITE);
    if(!vela_load_html("<html><head><title>Parsed</title></head><body><script>document.title='Should Not Run';</script></body></html>","https://lite.test/"))return 7;
    if(strcmp(vela_title(),"Parsed")!=0)return 8;
    vela_set_scroll(999);if(vela_scroll()!=284)return 9;
    puts("UN_Vela core JS/profile/link/resource smoke passed");return 0;
}
