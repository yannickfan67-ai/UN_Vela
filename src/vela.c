#include <stdint.h>
#include <stddef.h>
#include "vela.h"

#define VELA_HISTORY_MAX 12
#define VELA_URL_CAP 256
#define VELA_RAW_CAP 32768
#define VELA_JS_TEXT_CAP 1536

static AsterDocument g_doc;
static char g_raw[VELA_RAW_CAP];
static char g_url[VELA_URL_CAP];
static char g_status[160];
static char g_history[VELA_HISTORY_MAX][VELA_URL_CAP];
static int g_url_len,g_viewport=720,g_viewport_h=420,g_scroll,g_history_count,g_history_index=-1;
static uint32_t g_features=VELA_PROFILE_FULL;
static VelaPlatformOps g_platform;static void*g_platform_context;static int g_platform_ready;

static void bytes_zero(void*p,size_t n){uint8_t*b=(uint8_t*)p;while(n--)*b++=0;}
static void bytes_copy(void*d,const void*s,size_t n){uint8_t*dd=(uint8_t*)d;const uint8_t*ss=(const uint8_t*)s;while(n--)*dd++=*ss++;}
static void copy(char*d,size_t cap,const char*s){size_t i=0;if(!cap)return;while(s&&s[i]&&i+1<cap){d[i]=s[i];i++;}d[i]=0;}
static void append(char*d,size_t cap,const char*s){size_t i=0;while(i<cap&&d[i])i++;while(s&&*s&&i+1<cap)d[i++]=*s++;if(i<cap)d[i]=0;}
static char lower_ascii(char c){return c>='A'&&c<='Z'?(char)(c+32):c;}
static int starts_ci(const char*s,const char*p){if(!s||!p)return 0;while(*p){if(!*s||lower_ascii(*s++)!=lower_ascii(*p++))return 0;}return 1;}
static const char*find_ci(const char*s,const char*needle){if(!s||!needle||!*needle)return s;for(;*s;s++)if(starts_ci(s,needle))return s;return 0;}
static void set_url(const char*s){copy(g_url,sizeof(g_url),s?s:"");g_url_len=0;while(g_url[g_url_len]&&g_url_len<(int)sizeof(g_url)-1)g_url_len++;}
static void request_repaint(void){if(g_platform_ready&&(g_platform.capabilities&VELA_PLATFORM_CAP_REPAINT)&&g_platform.request_repaint)g_platform.request_repaint(g_platform_context);}
static void platform_log(uint32_t level,const char*m){if(g_platform_ready&&(g_platform.capabilities&VELA_PLATFORM_CAP_LOG)&&g_platform.log)g_platform.log(g_platform_context,level,m);}
static void parse_current(void){aster_parse_html(&g_doc,g_raw);aster_layout(&g_doc,g_viewport>40?g_viewport-40:g_viewport);g_scroll=0;}
static int canonicalize_url(const char*url,char*out,size_t cap){if(!url||!out||cap<9)return 0;while(*url==' '||*url=='\t')url++;if(!*url){copy(g_status,sizeof(g_status),"Enter an HTTP address");return 0;}if(starts_ci(url,"https://")){if(!(g_platform_ready&&(g_platform.capabilities&VELA_PLATFORM_CAP_TLS))){copy(g_status,sizeof(g_status),"HTTPS requires a TLS-capable host");return 0;}copy(out,cap,url);return 1;}if(starts_ci(url,"http://")){copy(out,cap,url);return 1;}if(starts_ci(url,"file:")||starts_ci(url,"data:")||starts_ci(url,"mailto:")){copy(g_status,sizeof(g_status),"Unsupported URL scheme");return 0;}if(starts_ci(url,"javascript:")){copy(g_status,sizeof(g_status),"Use link activation for javascript: URLs");return 0;}copy(out,cap,"http://");append(out,cap,url);return 1;}
static int base_origin(const char*base,char*out,size_t cap,int directory){if(!base||!starts_ci(base,"http"))return 0;const char*p=find_ci(base,"://");if(!p)return 0;p+=3;const char*slash=p;while(*slash&&*slash!='/')slash++;if(!*slash){copy(out,cap,base);if(directory)append(out,cap,"/");return 1;}size_t authority=(size_t)(slash-base);if(!directory){if(authority>=cap)authority=cap-1;for(size_t i=0;i<authority;i++)out[i]=base[i];out[authority]=0;return 1;}const char*last=slash;for(const char*q=slash;*q;q++)if(*q=='/')last=q;size_t upto=(size_t)(last-base)+1;if(upto>=cap)upto=cap-1;for(size_t i=0;i<upto;i++)out[i]=base[i];out[upto]=0;return 1;}
static int resolve_url(const char*base,const char*ref,char*out,size_t cap){if(!ref||!*ref)return 0;if(starts_ci(ref,"http://")||starts_ci(ref,"https://"))return canonicalize_url(ref,out,cap);if(starts_ci(ref,"javascript:")){copy(out,cap,ref);return 1;}if(ref[0]=='#'){copy(out,cap,base?base:"");return out[0]!=0;}if(ref[0]=='/'&&ref[1]=='/'){if(base&&starts_ci(base,"https://"))copy(out,cap,"https:");else copy(out,cap,"http:");append(out,cap,ref);return 1;}if(ref[0]=='/'){if(!base_origin(base,out,cap,0))return canonicalize_url(ref,out,cap);append(out,cap,ref);return 1;}if(!base_origin(base,out,cap,1))return canonicalize_url(ref,out,cap);append(out,cap,ref);return 1;}
static size_t js_string(const char*p,char*out,size_t cap,const char**after){while(*p==' '||*p=='\t')p++;if(*p!='\''&&*p!='"')return 0;char q=*p++;size_t o=0;while(*p&&*p!=q){char c=*p++;if(c=='\\'&&*p){char e=*p++;if(e=='n')c='\n';else if(e=='r')c='\r';else if(e=='t')c='\t';else c=e;}if(o+1<cap)out[o++]=c;}if(*p==q)p++;out[o]=0;if(after)*after=p;return o;}
static int js_assign_value(const char*script,const char*expr,char*out,size_t cap){const char*p=find_ci(script,expr);if(!p)return 0;while(*p&&*p!='=')p++;if(*p!='=')return 0;p++;return js_string(p,out,cap,0)>0;}
static int js_call_value(const char*script,const char*name,char*out,size_t cap){const char*p=find_ci(script,name);if(!p)return 0;while(*p&&*p!='(')p++;if(*p!='(')return 0;p++;return js_string(p,out,cap,0)>0;}
static void execute_inline_scripts(char*redirect,size_t redirect_cap,char*title,size_t title_cap,char*body,size_t body_cap,char*write,size_t write_cap){const char*p=g_raw;while((p=find_ci(p,"<script"))!=0){const char*gt=p;while(*gt&&*gt!='>')gt++;if(!*gt)break;gt++;const char*end=find_ci(gt,"</script");if(!end)break;size_t n=(size_t)(end-gt);if(n>=VELA_JS_TEXT_CAP)n=VELA_JS_TEXT_CAP-1;char script[VELA_JS_TEXT_CAP];for(size_t i=0;i<n;i++)script[i]=gt[i];script[n]=0;char tmp[1024];if(js_assign_value(script,"document.title",tmp,sizeof(tmp)))copy(title,title_cap,tmp);if(js_assign_value(script,"document.body.innerhtml",tmp,sizeof(tmp)))copy(body,body_cap,tmp);if(js_assign_value(script,"window.location.href",tmp,sizeof(tmp))||js_assign_value(script,"location.href",tmp,sizeof(tmp)))copy(redirect,redirect_cap,tmp);if(js_call_value(script,"document.write",tmp,sizeof(tmp)))append(write,write_cap,tmp);if(js_call_value(script,"console.log",tmp,sizeof(tmp)))platform_log(1,tmp);p=end+8;}}
static int preprocess_scripts(char*redirect,size_t redirect_cap,char*title,size_t title_cap){redirect[0]=0;title[0]=0;if(!(g_features&VELA_FEATURE_JAVASCRIPT))return 1;char body[4096]={0},write[2048]={0};execute_inline_scripts(redirect,redirect_cap,title,title_cap,body,sizeof(body),write,sizeof(write));if(body[0]){char rebuilt[VELA_RAW_CAP];rebuilt[0]=0;append(rebuilt,sizeof(rebuilt),"<html><head><title>UN_Vela script</title></head><body>");append(rebuilt,sizeof(rebuilt),body);append(rebuilt,sizeof(rebuilt),write);append(rebuilt,sizeof(rebuilt),"</body></html>");copy(g_raw,sizeof(g_raw),rebuilt);}else if(write[0])append(g_raw,sizeof(g_raw),write);return 1;}
static void history_push(const char*url){if(!url||!*url)return;if(g_history_index>=0){const char*a=g_history[g_history_index],*b=url;int same=1;while(*a||*b)if(*a++!=*b++){same=0;break;}if(same)return;}if(g_history_index+1<g_history_count)g_history_count=g_history_index+1;if(g_history_count<VELA_HISTORY_MAX){g_history_index=g_history_count++;copy(g_history[g_history_index],VELA_URL_CAP,url);}else{for(int i=1;i<VELA_HISTORY_MAX;i++)copy(g_history[i-1],VELA_URL_CAP,g_history[i]);g_history_index=VELA_HISTORY_MAX-1;copy(g_history[g_history_index],VELA_URL_CAP,url);}}
static int navigate_depth(const char*url,int push,int depth){char normalized[VELA_URL_CAP];if(!url||!*url){copy(g_status,sizeof(g_status),"Enter an HTTP address");return 0;}if(!g_platform_ready||!(g_platform.capabilities&VELA_PLATFORM_CAP_HTTP)||!g_platform.http_get){copy(g_status,sizeof(g_status),"No HTTP backend installed");return 0;}if(!canonicalize_url(url,normalized,sizeof(normalized)))return 0;copy(g_status,sizeof(g_status),"Loading...");request_repaint();platform_log(1,"loading document");if(!g_platform.http_get(g_platform_context,normalized,g_raw,sizeof(g_raw),g_status,sizeof(g_status)))return 0;set_url(normalized);char redirect[VELA_URL_CAP],script_title[96];preprocess_scripts(redirect,sizeof(redirect),script_title,sizeof(script_title));parse_current();if(script_title[0])copy(g_doc.title,sizeof(g_doc.title),script_title);if(push)history_push(normalized);if(redirect[0]&&depth<2){char next[VELA_URL_CAP];if(resolve_url(normalized,redirect,next,sizeof(next))&&!starts_ci(next,"javascript:"))return navigate_depth(next,push,depth+1);}if(g_doc.title[0]){char s[160];copy(s,sizeof(s),g_status);copy(g_status,sizeof(g_status),g_doc.title);append(g_status,sizeof(g_status)," / ");append(g_status,sizeof(g_status),s);}request_repaint();return 1;}
static int navigate(const char*url,int push){return navigate_depth(url,push,0);}
static void start_page(void){copy(g_raw,sizeof(g_raw),"<html><head><title>UN_Vela</title><style>h2{color:#315e80} a{font-weight:bold}</style></head><body><header><h1>UN_Vela 0.3</h1><p>Portable browser shell powered by Aster Engine.</p></header><main><h2>Runtime</h2><p>HTTP/HTTPS carrier, CSS subset, image elements, history, links, scrolling and a safe JavaScript subset are available by capability.</p></main></body></html>");parse_current();}
int vela_set_platform(const VelaPlatformOps*platform,void*context){bytes_zero(&g_platform,sizeof(g_platform));g_platform_context=context;g_platform_ready=0;if(!platform)return 1;if(!vela_platform_is_compatible(platform))return 0;size_t n=platform->struct_size<sizeof(g_platform)?platform->struct_size:sizeof(g_platform);bytes_copy(&g_platform,platform,n);g_platform_ready=1;return 1;}
uint32_t vela_api_version(void){return VELA_API_VERSION;}
uint64_t vela_capabilities(void){return VELA_CAP_PLATFORM_ABI|VELA_CAP_HISTORY|VELA_CAP_SCROLL|VELA_CAP_LOCAL_HTML|VELA_CAP_ASTER_DOC|VELA_CAP_LINK_ACTIVATION|VELA_CAP_JS_SUBSET|VELA_CAP_FEATURE_PROFILE|VELA_CAP_RESOURCES|VELA_CAP_NAV_ACTIONS;}
uint64_t vela_platform_capabilities(void){return g_platform_ready?g_platform.capabilities:0;}
void vela_set_features(uint32_t f){g_features=f;request_repaint();}
uint32_t vela_features(void){return g_features;}
int vela_init_ex(int viewport_width,const VelaPlatformOps*platform,void*context){g_viewport=viewport_width>120?viewport_width:120;g_viewport_h=420;g_scroll=0;g_history_count=0;g_history_index=-1;g_features=VELA_PROFILE_FULL;set_url("");if(!vela_set_platform(platform,context))return 0;copy(g_status,sizeof(g_status),"Ready / Aster Engine ");append(g_status,sizeof(g_status),aster_version());start_page();return 1;}
void vela_init(int viewport_width){(void)vela_init_ex(viewport_width,0,0);}
void vela_set_viewport_size(int w,int h){if(w<120)w=120;if(h<80)h=80;int changed=w!=g_viewport;g_viewport=w;g_viewport_h=h;if(changed)aster_layout(&g_doc,g_viewport>40?g_viewport-40:g_viewport);vela_set_scroll(g_scroll);}
void vela_set_viewport(int w){vela_set_viewport_size(w,g_viewport_h);}
void vela_input_char(char c){if(g_url_len<(int)sizeof(g_url)-1){g_url[g_url_len++]=c;g_url[g_url_len]=0;request_repaint();}}
void vela_backspace(void){if(g_url_len){g_url[--g_url_len]=0;request_repaint();}}
int vela_load_url(const char*url){return navigate(url,1);}
int vela_go(void){return navigate(g_url,1);}
int vela_load_html(const char*html,const char*virtual_url){if(!html)return 0;copy(g_raw,sizeof(g_raw),html);set_url(virtual_url?virtual_url:"about:local");char redirect[VELA_URL_CAP],title[96];preprocess_scripts(redirect,sizeof(redirect),title,sizeof(title));parse_current();if(title[0])copy(g_doc.title,sizeof(g_doc.title),title);copy(g_status,sizeof(g_status),"Local document");history_push(g_url);request_repaint();return 1;}
int vela_can_back(void){return g_history_index>0;}
int vela_can_forward(void){return g_history_index>=0&&g_history_index+1<g_history_count;}
int vela_back(void){if(!vela_can_back())return 0;int target=g_history_index-1;char u[VELA_URL_CAP];copy(u,sizeof(u),g_history[target]);if(!navigate(u,0))return 0;g_history_index=target;return 1;}
int vela_forward(void){if(!vela_can_forward())return 0;int target=g_history_index+1;char u[VELA_URL_CAP];copy(u,sizeof(u),g_history[target]);if(!navigate(u,0))return 0;g_history_index=target;return 1;}
int vela_reload(void){if(!g_url[0])return 0;return navigate(g_url,0);}
int vela_navigate_action(VelaNavAction action){
    int before=g_scroll;
    int page=g_viewport_h>96?g_viewport_h-48:48;
    switch(action){
        case VELA_NAV_BACK:return vela_back();
        case VELA_NAV_FORWARD:return vela_forward();
        case VELA_NAV_RELOAD:return vela_reload();
        case VELA_NAV_LINE_UP:vela_scroll_by(-48);return g_scroll!=before;
        case VELA_NAV_LINE_DOWN:vela_scroll_by(48);return g_scroll!=before;
        case VELA_NAV_PAGE_UP:vela_scroll_by(-page);return g_scroll!=before;
        case VELA_NAV_PAGE_DOWN:vela_scroll_by(page);return g_scroll!=before;
        case VELA_NAV_HOME:vela_set_scroll(0);return g_scroll!=before;
        case VELA_NAV_END:vela_set_scroll(aster_document_height(&g_doc));return g_scroll!=before;
        default:return 0;
    }
}
int vela_activate_link(int x,int y){char href[VELA_URL_CAP];if(!aster_link_at(&g_doc,x,y,g_scroll,href,sizeof(href)))return 0;if(starts_ci(href,"javascript:")){if(!(g_features&VELA_FEATURE_JAVASCRIPT)){copy(g_status,sizeof(g_status),"JavaScript disabled by profile");return 0;}char script[VELA_JS_TEXT_CAP];copy(script,sizeof(script),href+11);char tmp[256];if(js_call_value(script,"console.log",tmp,sizeof(tmp)))platform_log(1,tmp);if(js_assign_value(script,"location.href",tmp,sizeof(tmp))){char u[VELA_URL_CAP];if(resolve_url(g_url,tmp,u,sizeof(u)))return navigate(u,1);}copy(g_status,sizeof(g_status),"JavaScript link executed");request_repaint();return 1;}char target[VELA_URL_CAP];if(!resolve_url(g_url,href,target,sizeof(target)))return 0;return navigate(target,1);}
int vela_resource_get(const char*ref,uint8_t*data,size_t data_cap,size_t*data_len,char*content_type,size_t content_type_cap,char*status,size_t status_cap){if(data_len)*data_len=0;if(content_type&&content_type_cap)content_type[0]=0;if(status&&status_cap)status[0]=0;if(!ref||!*ref||!data||!data_cap)return 0;if(!g_platform_ready||!(g_platform.capabilities&VELA_PLATFORM_CAP_RESOURCES)||!g_platform.resource_get){if(status&&status_cap)copy(status,status_cap,"No resource backend installed");return 0;}char target[VELA_URL_CAP];if(!resolve_url(g_url,ref,target,sizeof(target))||starts_ci(target,"javascript:")){if(status&&status_cap)copy(status,status_cap,"Unsupported resource URL");return 0;}return g_platform.resource_get(g_platform_context,target,data,data_cap,data_len,content_type,content_type_cap,status,status_cap);}
void vela_set_scroll(int y){int max=aster_document_height(&g_doc)-g_viewport_h+24;if(max<0)max=0;if(y<0)y=0;if(y>max)y=max;g_scroll=y;request_repaint();}
void vela_scroll_by(int dy){vela_set_scroll(g_scroll+dy);}
int vela_scroll(void){return g_scroll;}
const char*vela_url(void){return g_url;}
const char*vela_status(void){return g_status;}
const char*vela_title(void){return g_doc.title[0]?g_doc.title:VELA_NAME;}
const AsterDocument*vela_document(void){return &g_doc;}
void vela_paint(int x,int y,int width,int height){vela_set_viewport_size(width,height);aster_paint(&g_doc,x+18,y+10,width-36,height-20,g_scroll);}
