#include <stdint.h>
#include <stddef.h>
#include "vela.h"

#define VELA_HISTORY_MAX 8
#define VELA_URL_CAP 192

static AsterDocument g_doc;
static char g_raw[32768];
static char g_url[VELA_URL_CAP];
static char g_status[128];
static char g_history[VELA_HISTORY_MAX][VELA_URL_CAP];
static int g_url_len;
static int g_viewport=720;
static int g_scroll;
static int g_history_count;
static int g_history_index=-1;
static VelaPlatformOps g_platform;
static void *g_platform_context;
static int g_platform_ready;

static void bytes_zero(void *p,size_t n){uint8_t*b=(uint8_t*)p;while(n--)*b++=0;}
static void bytes_copy(void*d,const void*s,size_t n){uint8_t*dd=(uint8_t*)d;const uint8_t*ss=(const uint8_t*)s;while(n--)*dd++=*ss++;}
static void copy(char*d,size_t cap,const char*s){size_t i=0;if(!cap)return;while(s&&s[i]&&i+1<cap){d[i]=s[i];i++;}d[i]=0;}
static void append(char*d,size_t cap,const char*s){size_t i=0;while(i<cap&&d[i])i++;while(s&&*s&&i+1<cap)d[i++]=*s++;if(i<cap)d[i]=0;}
static char lower_ascii(char c){return c>='A'&&c<='Z'?(char)(c+('a'-'A')):c;}
static int starts_ci(const char*s,const char*p){if(!s||!p)return 0;while(*p){if(!*s||lower_ascii(*s++)!=lower_ascii(*p++))return 0;}return 1;}
static void set_url(const char*s){copy(g_url,sizeof(g_url),s?s:"");g_url_len=0;while(g_url[g_url_len]&&g_url_len<(int)sizeof(g_url)-1)g_url_len++;}
static void request_repaint(void){
    if(g_platform_ready&&(g_platform.capabilities&VELA_PLATFORM_CAP_REPAINT)&&g_platform.request_repaint)
        g_platform.request_repaint(g_platform_context);
}
static void platform_log(uint32_t level,const char*message){
    if(g_platform_ready&&(g_platform.capabilities&VELA_PLATFORM_CAP_LOG)&&g_platform.log)
        g_platform.log(g_platform_context,level,message);
}
static void parse_current(void){
    aster_parse_html(&g_doc,g_raw);
    aster_layout(&g_doc,g_viewport>40?g_viewport-40:g_viewport);
    g_scroll=0;
}
static int canonicalize_url(const char*url,char*out,size_t cap){
    if(!url||!out||cap<9)return 0;
    while(*url==' '||*url=='\t')url++;
    if(!*url){copy(g_status,sizeof(g_status),"Enter an HTTP address");return 0;}
    if(starts_ci(url,"https://")){
        if(!(g_platform_ready&&(g_platform.capabilities&VELA_PLATFORM_CAP_TLS))){
            copy(g_status,sizeof(g_status),"HTTPS requires a TLS-capable host");
            return 0;
        }
        copy(out,cap,url);return 1;
    }
    if(starts_ci(url,"http://")){copy(out,cap,url);return 1;}
    if(starts_ci(url,"file:")||starts_ci(url,"javascript:")||starts_ci(url,"data:")||starts_ci(url,"mailto:")){
        copy(g_status,sizeof(g_status),"Unsupported URL scheme");return 0;
    }
    copy(out,cap,"http://");append(out,cap,url);return 1;
}
static void start_page(void){
    copy(g_raw,sizeof(g_raw),
        "<html><head><title>UN_Vela</title></head><body>"
        "<header><h1>UN_Vela</h1><p>Portable browser shell powered by Aster Engine.</p></header>"
        "<main><section><h2>Compatibility</h2>"
        "<p>The browser core is OS and CPU architecture independent. The host supplies network, windowing and other services through Vela Platform ABI 1.</p>"
        "<p>UN_Orion is one host; hosted Windows, Linux, macOS and other ports can use the same shell.</p>"
        "<h3>Carrier capabilities</h3><p>HTTP is a host capability. HTTPS is accepted only when the host advertises TLS support.</p>"
        "</section></main><footer><p>History, scrolling and local HTML remain browser-core features.</p></footer>"
        "</body></html>");
    parse_current();
}
static void history_push(const char *url){
    if(!url||!*url)return;
    if(g_history_index>=0){
        const char*a=g_history[g_history_index],*b=url;
        int same=1;
        while(*a||*b){if(*a++!=*b++){same=0;break;}}
        if(same)return;
    }
    if(g_history_index+1<g_history_count)g_history_count=g_history_index+1;
    if(g_history_count<VELA_HISTORY_MAX){
        g_history_index=g_history_count++;
        copy(g_history[g_history_index],VELA_URL_CAP,url);
    }else{
        for(int i=1;i<VELA_HISTORY_MAX;i++)copy(g_history[i-1],VELA_URL_CAP,g_history[i]);
        g_history_index=VELA_HISTORY_MAX-1;
        copy(g_history[g_history_index],VELA_URL_CAP,url);
    }
}
static int navigate(const char *url,int push){
    char normalized[VELA_URL_CAP];
    if(!url||!*url){copy(g_status,sizeof(g_status),"Enter an HTTP address");return 0;}
    if(!g_platform_ready||!(g_platform.capabilities&VELA_PLATFORM_CAP_HTTP)||!g_platform.http_get){
        copy(g_status,sizeof(g_status),"No HTTP backend installed");
        return 0;
    }
    if(!canonicalize_url(url,normalized,sizeof(normalized)))return 0;
    copy(g_status,sizeof(g_status),"Loading...");
    request_repaint();
    platform_log(1,"loading document");
    if(!g_platform.http_get(g_platform_context,normalized,g_raw,sizeof(g_raw),g_status,sizeof(g_status)))return 0;
    set_url(normalized);
    parse_current();
    if(push)history_push(normalized);
    if(g_doc.title[0]){
        char s[128];
        copy(s,sizeof(s),g_status);
        copy(g_status,sizeof(g_status),g_doc.title);
        append(g_status,sizeof(g_status)," / ");
        append(g_status,sizeof(g_status),s);
    }
    request_repaint();
    return 1;
}
int vela_set_platform(const VelaPlatformOps *platform,void *context){
    bytes_zero(&g_platform,sizeof(g_platform));
    g_platform_context=context;
    g_platform_ready=0;
    if(!platform)return 1;
    if(!vela_platform_is_compatible(platform))return 0;
    size_t n=platform->struct_size<sizeof(g_platform)?platform->struct_size:sizeof(g_platform);
    bytes_copy(&g_platform,platform,n);
    g_platform_ready=1;
    return 1;
}
uint32_t vela_api_version(void){return VELA_API_VERSION;}
uint64_t vela_capabilities(void){return VELA_CAP_PLATFORM_ABI|VELA_CAP_HISTORY|VELA_CAP_SCROLL|VELA_CAP_LOCAL_HTML|VELA_CAP_ASTER_DOC;}
uint64_t vela_platform_capabilities(void){return g_platform_ready?g_platform.capabilities:0;}
int vela_init_ex(int viewport_width,const VelaPlatformOps*platform,void*context){
    g_viewport=viewport_width>120?viewport_width:120;
    g_scroll=0;
    g_history_count=0;
    g_history_index=-1;
    set_url("");
    if(!vela_set_platform(platform,context))return 0;
    copy(g_status,sizeof(g_status),"Ready / Aster Engine ");
    append(g_status,sizeof(g_status),aster_version());
    start_page();
    return 1;
}
void vela_init(int viewport_width){(void)vela_init_ex(viewport_width,0,0);}
void vela_set_viewport(int viewport_width){
    if(viewport_width<120)viewport_width=120;
    if(viewport_width!=g_viewport){
        g_viewport=viewport_width;
        aster_layout(&g_doc,g_viewport>40?g_viewport-40:g_viewport);
        vela_set_scroll(g_scroll);
        request_repaint();
    }
}
void vela_input_char(char c){if(g_url_len<(int)sizeof(g_url)-1){g_url[g_url_len++]=c;g_url[g_url_len]=0;request_repaint();}}
void vela_backspace(void){if(g_url_len){g_url[--g_url_len]=0;request_repaint();}}
int vela_load_url(const char*url){return navigate(url,1);}
int vela_go(void){return navigate(g_url,1);}
int vela_load_html(const char*html,const char*virtual_url){
    if(!html)return 0;
    copy(g_raw,sizeof(g_raw),html);
    set_url(virtual_url?virtual_url:"about:local");
    parse_current();
    copy(g_status,sizeof(g_status),"Local document");
    history_push(g_url);
    request_repaint();
    return 1;
}
int vela_can_back(void){return g_history_index>0;}
int vela_can_forward(void){return g_history_index>=0&&g_history_index+1<g_history_count;}
int vela_back(void){
    if(!vela_can_back())return 0;
    int target=g_history_index-1;
    char u[VELA_URL_CAP];
    copy(u,sizeof(u),g_history[target]);
    if(!navigate(u,0))return 0;
    g_history_index=target;
    return 1;
}
int vela_forward(void){
    if(!vela_can_forward())return 0;
    int target=g_history_index+1;
    char u[VELA_URL_CAP];
    copy(u,sizeof(u),g_history[target]);
    if(!navigate(u,0))return 0;
    g_history_index=target;
    return 1;
}
int vela_reload(void){if(!g_url[0])return 0;return navigate(g_url,0);}
void vela_set_scroll(int y){
    int max=aster_document_height(&g_doc)>0?aster_document_height(&g_doc)-1:0;
    if(y<0)y=0;
    if(y>max)y=max;
    g_scroll=y;
    request_repaint();
}
void vela_scroll_by(int dy){vela_set_scroll(g_scroll+dy);}
int vela_scroll(void){return g_scroll;}
const char*vela_url(void){return g_url;}
const char*vela_status(void){return g_status;}
const char*vela_title(void){return g_doc.title[0]?g_doc.title:VELA_NAME;}
const AsterDocument*vela_document(void){return &g_doc;}
void vela_paint(int x,int y,int width,int height){vela_set_viewport(width);aster_paint(&g_doc,x+18,y+10,width-36,height-20,g_scroll);}
