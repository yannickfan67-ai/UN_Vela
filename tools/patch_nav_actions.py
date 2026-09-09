from pathlib import Path

src = Path("src/vela.c")
s = src.read_text()
old_caps = "return VELA_CAP_PLATFORM_ABI|VELA_CAP_HISTORY|VELA_CAP_SCROLL|VELA_CAP_LOCAL_HTML|VELA_CAP_ASTER_DOC|VELA_CAP_LINK_ACTIVATION|VELA_CAP_JS_SUBSET|VELA_CAP_FEATURE_PROFILE|VELA_CAP_RESOURCES;"
new_caps = "return VELA_CAP_PLATFORM_ABI|VELA_CAP_HISTORY|VELA_CAP_SCROLL|VELA_CAP_LOCAL_HTML|VELA_CAP_ASTER_DOC|VELA_CAP_LINK_ACTIVATION|VELA_CAP_JS_SUBSET|VELA_CAP_FEATURE_PROFILE|VELA_CAP_RESOURCES|VELA_CAP_NAV_ACTIONS;"
if old_caps not in s:
    raise SystemExit("capability anchor not found")
s = s.replace(old_caps, new_caps, 1)
anchor = "int vela_reload(void){if(!g_url[0])return 0;return navigate(g_url,0);}\n"
impl = r'''int vela_navigate_action(VelaNavAction action){
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
'''
if anchor not in s:
    raise SystemExit("reload anchor not found")
s = s.replace(anchor, anchor + impl, 1)
src.write_text(s)

test = Path("tests/core_smoke.c")
t = test.read_text()
old = '''    vela_set_scroll(999);if(vela_scroll()!=284)return 9;\n    puts("UN_Vela core JS/profile/link/resource smoke passed");return 0;\n'''
new = '''    if(!(vela_capabilities()&VELA_CAP_NAV_ACTIONS))return 9;\n    vela_set_scroll(999);if(vela_scroll()!=284)return 10;\n    if(!vela_navigate_action(VELA_NAV_HOME)||vela_scroll()!=0)return 11;\n    if(!vela_navigate_action(VELA_NAV_END)||vela_scroll()!=284)return 12;\n    if(!vela_navigate_action(VELA_NAV_PAGE_UP)||vela_scroll()!=92)return 13;\n    if(!vela_navigate_action(VELA_NAV_LINE_DOWN)||vela_scroll()!=140)return 14;\n    if(vela_navigate_action((VelaNavAction)999))return 15;\n    puts("UN_Vela core JS/profile/link/resource/navigation smoke passed");return 0;\n'''
if old not in t:
    raise SystemExit("core smoke anchor not found")
test.write_text(t.replace(old, new, 1))
