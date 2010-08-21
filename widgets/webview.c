/*
 * webview.c - webkit webview widget
 *
 * Copyright (C) 2010 Mason Larobina <mason.larobina@gmail.com>
 * Copyright (C) 2007-2009 Julien Danjou <julien@danjou.info>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "luah.h"
#include "widgets/common.h"
#include <JavaScriptCore/JavaScript.h>
#include <webkit/webkit.h>
#include <libsoup/soup.h>
#include "math.h"

static struct {
    SoupSession   *session;
    SoupCookieJar *cookiejar;
} Soup = { NULL, NULL };

typedef enum {
    BOOL,
    CHAR,
    INT,
    FLOAT,
    DOUBLE,
    URI,
} property_value_t;

typedef enum {
    SETTINGS,
    WEBKITVIEW,
    SOUPSESSION,
    SOUPCOOKIEJAR,
} property_scope;

typedef union {
    gchar    *c;
    gboolean  b;
    gdouble   d;
    gfloat    f;
    gint      i;
} temp_value_t;

GHashTable *properties = NULL;

typedef struct {
    const gchar      *name;
    property_value_t type;
    property_scope   scope;
    gboolean         writable;
    const gchar      *signame;
} property_t;

property_t properties_table[] = {
  { "accept-language",                              CHAR,   SOUPSESSION, TRUE,  NULL },
  { "accept-language-auto",                         BOOL,   SOUPSESSION, TRUE,  NULL },
  { "accept-policy",                                INT,    SOUPCOOKIEJAR, TRUE,  NULL },
  { "auto-load-images",                             BOOL,   SETTINGS,    TRUE,  NULL },
  { "auto-resize-window",                           BOOL,   SETTINGS,    TRUE,  NULL },
  { "auto-shrink-images",                           BOOL,   SETTINGS,    TRUE,  NULL },
  { "cursive-font-family",                          CHAR,   SETTINGS,    TRUE,  NULL },
  { "custom-encoding",                              CHAR,   WEBKITVIEW,  TRUE,  NULL },
  { "default-encoding",                             CHAR,   SETTINGS,    TRUE,  NULL },
  { "default-font-family",                          CHAR,   SETTINGS,    TRUE,  NULL },
  { "default-font-size",                            INT,    SETTINGS,    TRUE,  NULL },
  { "default-monospace-font-size",                  INT,    SETTINGS,    TRUE,  NULL },
  { "editable",                                     BOOL,   WEBKITVIEW,  TRUE,  NULL },
  { "enable-caret-browsing",                        BOOL,   SETTINGS,    TRUE,  NULL },
  { "enable-default-context-menu",                  BOOL,   SETTINGS,    TRUE,  NULL },
  { "enable-developer-extras",                      BOOL,   SETTINGS,    TRUE,  NULL },
  { "enable-dom-paste",                             BOOL,   SETTINGS,    TRUE,  NULL },
  { "enable-file-access-from-file-uris",            BOOL,   SETTINGS,    TRUE,  NULL },
  { "enable-html5-database",                        BOOL,   SETTINGS,    TRUE,  NULL },
  { "enable-html5-local-storage",                   BOOL,   SETTINGS,    TRUE,  NULL },
  { "enable-java-applet",                           BOOL,   SETTINGS,    TRUE,  NULL },
  { "enable-offline-web-application-cache",         BOOL,   SETTINGS,    TRUE,  NULL },
  { "enable-page-cache",                            BOOL,   SETTINGS,    TRUE,  NULL },
  { "enable-plugins",                               BOOL,   SETTINGS,    TRUE,  NULL },
  { "enable-private-browsing",                      BOOL,   SETTINGS,    TRUE,  NULL },
  { "enable-scripts",                               BOOL,   SETTINGS,    TRUE,  NULL },
  { "enable-site-specific-quirks",                  BOOL,   SETTINGS,    TRUE,  NULL },
  { "enable-spatial-navigation",                    BOOL,   SETTINGS,    TRUE,  NULL },
  { "enable-spell-checking",                        BOOL,   SETTINGS,    TRUE,  NULL },
  { "enable-universal-access-from-file-uris",       BOOL,   SETTINGS,    TRUE,  NULL },
  { "enable-xss-auditor",                           BOOL,   SETTINGS,    TRUE,  NULL },
  { "encoding",                                     CHAR,   WEBKITVIEW,  FALSE, NULL },
  { "enforce-96-dpi",                               BOOL,   SETTINGS,    TRUE,  NULL },
  { "fantasy-font-family",                          CHAR,   SETTINGS,    TRUE,  NULL },
  { "full-content-zoom",                            BOOL,   WEBKITVIEW,  TRUE,  NULL },
  { "icon-uri",                                     CHAR,   WEBKITVIEW,  FALSE, NULL },
  { "idle-timeout",                                 INT,    SOUPSESSION, TRUE,  NULL },
  { "javascript-can-access-clipboard",              BOOL,   SETTINGS,    TRUE,  NULL },
  { "javascript-can-open-windows-automatically",    BOOL,   SETTINGS,    TRUE,  NULL },
  { "max-conns",                                    INT,    SOUPSESSION, TRUE,  NULL },
  { "max-conns-per-host",                           INT,    SOUPSESSION, TRUE,  NULL },
  { "minimum-font-size",                            INT,    SETTINGS,    TRUE,  NULL },
  { "minimum-logical-font-size",                    INT,    SETTINGS,    TRUE,  NULL },
  { "monospace-font-family",                        CHAR,   SETTINGS,    TRUE,  NULL },
  { "print-backgrounds",                            BOOL,   SETTINGS,    TRUE,  NULL },
  { "progress",                                     DOUBLE, WEBKITVIEW,  FALSE, NULL },
  { "proxy-uri",                                    URI,    SOUPSESSION, TRUE,  NULL },
  { "resizable-text-areas",                         BOOL,   SETTINGS,    TRUE,  NULL },
  { "sans-serif-font-family",                       CHAR,   SETTINGS,    TRUE,  NULL },
  { "serif-font-family",                            CHAR,   SETTINGS,    TRUE,  NULL },
  { "spell-checking-languages",                     CHAR,   SETTINGS,    TRUE,  NULL },
  { "ssl-ca-file",                                  CHAR,   SOUPSESSION, TRUE,  NULL },
  { "ssl-strict",                                   BOOL,   SOUPSESSION, TRUE,  NULL },
  { "tab-key-cycles-through-elements",              BOOL,   SETTINGS,    TRUE,  NULL },
  { "timeout",                                      INT,    SOUPSESSION, TRUE,  NULL },
  { "title",                                        CHAR,   WEBKITVIEW,  FALSE, NULL },
  { "transparent",                                  BOOL,   WEBKITVIEW,  TRUE,  NULL },
  { "uri",                                          CHAR,   WEBKITVIEW,  TRUE,  NULL },
  { "use-ntlm",                                     BOOL,   SOUPSESSION, TRUE,  NULL },
  { "user-agent",                                   CHAR,   SETTINGS,    TRUE,  NULL },
  { "user-stylesheet-uri",                          CHAR,   SETTINGS,    TRUE,  NULL },
  { "zoom-level",                                   FLOAT,  WEBKITVIEW,  TRUE,  NULL },
  { "zoom-step",                                    FLOAT,  SETTINGS,    TRUE,  NULL },
  { NULL,                                           0,      0,           0,     NULL },
};

static void
webview_init_properties() {
    properties = g_hash_table_new(g_str_hash, g_str_equal);
    for (property_t *p = properties_table; p->name; p++) {
        /* pre-compile "property::name" signals for each property */
        if (!p->signame) p->signame = g_strdup_printf("property::%s", p->name);
        g_hash_table_insert(properties, (gpointer) p->name, (gpointer) p);
    }
}

static const gchar*
webview_eval_js(WebKitWebView *view, const gchar *script, const gchar *file) {
    WebKitWebFrame *frame;
    JSGlobalContextRef context;
    JSObjectRef globalobject;
    JSStringRef js_file;
    JSStringRef js_script;
    JSValueRef js_result;
    JSValueRef js_exc = NULL;
    JSStringRef js_result_string;
    GString *result = g_string_new(NULL);
    size_t js_result_size;

    frame = webkit_web_view_get_main_frame(WEBKIT_WEB_VIEW(view));
    context = webkit_web_frame_get_global_context(frame);
    globalobject = JSContextGetGlobalObject(context);

    /* evaluate the script and get return value*/
    js_script = JSStringCreateWithUTF8CString(script);
    js_file = JSStringCreateWithUTF8CString(file);
    js_result = JSEvaluateScript(context, js_script, globalobject, js_file, 0, &js_exc);
    if (js_result && !JSValueIsUndefined(context, js_result)) {
        js_result_string = JSValueToStringCopy(context, js_result, NULL);
        js_result_size = JSStringGetMaximumUTF8CStringSize(js_result_string);

        if (js_result_size) {
            char js_result_utf8[js_result_size];
            JSStringGetUTF8CString(js_result_string, js_result_utf8, js_result_size);
            g_string_assign(result, js_result_utf8);
        }

        JSStringRelease(js_result_string);
    }
    else if (js_exc) {
        size_t size;
        JSStringRef prop, val;
        JSObjectRef exc = JSValueToObject(context, js_exc, NULL);

        printf("Exception occured while executing script:\n");

        /* Print file */
        prop = JSStringCreateWithUTF8CString("sourceURL");
        val = JSValueToStringCopy(context, JSObjectGetProperty(context, exc, prop, NULL), NULL);
        size = JSStringGetMaximumUTF8CStringSize(val);
        if(size) {
            char cstr[size];
            JSStringGetUTF8CString(val, cstr, size);
            printf("At %s", cstr);
        }
        JSStringRelease(prop);
        JSStringRelease(val);

        /* Print line */
        prop = JSStringCreateWithUTF8CString("line");
        val = JSValueToStringCopy(context, JSObjectGetProperty(context, exc, prop, NULL), NULL);
        size = JSStringGetMaximumUTF8CStringSize(val);
        if(size) {
            char cstr[size];
            JSStringGetUTF8CString(val, cstr, size);
            printf(":%s: ", cstr);
        }
        JSStringRelease(prop);
        JSStringRelease(val);

        /* Print message */
        val = JSValueToStringCopy(context, exc, NULL);
        size = JSStringGetMaximumUTF8CStringSize(val);
        if(size) {
            char cstr[size];
            JSStringGetUTF8CString(val, cstr, size);
            printf("%s\n", cstr);
        }
        JSStringRelease(val);
    }

    /* cleanup */
    JSStringRelease(js_script);
    JSStringRelease(js_file);

    return g_string_free(result, FALSE);
}

static gint
luaH_webview_eval_js(lua_State *L)
{
    widget_t *w = luaH_checkudata(L, 1, &widget_class);
    WebKitWebView *view = WEBKIT_WEB_VIEW(g_object_get_data(G_OBJECT(w->widget), "webview"));
    const gchar *script = luaL_checkstring(L, 2);
    const gchar *filename = luaL_checkstring(L, 3);

    /* evaluate javascript script and push return result onto lua stack */
    const gchar *result = webview_eval_js(view, script, filename);
    lua_pushstring(L, result);
    return 1;
}

static void
notify_cb(WebKitWebView *v, GParamSpec *ps, widget_t *w)
{
    (void) v;
    property_t *p;
    /* emit "property::name" signal if found in properties table */
    if ((p = g_hash_table_lookup(properties, ps->name))) {
        lua_State *L = globalconf.L;
        luaH_object_push(L, w->ref);
        luaH_object_emit_signal(L, -1, p->signame, 0, 0);
        lua_pop(L, 1);
    }
}

static void
notify_load_status_cb(WebKitWebView *v, GParamSpec *ps, widget_t *w)
{
    (void) ps;

    /* Get load status */
    WebKitLoadStatus status;
    g_object_get(G_OBJECT(v), "load-status", &status, NULL);

    /* get load status literal */
    gchar *name = NULL;
    switch (status) {

#define LT_CASE(a, l) case WEBKIT_LOAD_##a: name = l; break;
        LT_CASE(PROVISIONAL,                     "provisional")
        LT_CASE(COMMITTED,                       "committed")
        LT_CASE(FINISHED,                        "finished")
        LT_CASE(FIRST_VISUALLY_NON_EMPTY_LAYOUT, "first-visual")
        LT_CASE(FAILED,                          "failed")
#undef  LT_CASE

      default:
        warn("programmer error, unable to get load status literal");
        break;
    }

    lua_State *L = globalconf.L;
    luaH_object_push(L, w->ref);
    lua_pushstring(L, name);
    luaH_object_emit_signal(L, -2, "load-status", 1, 0);
    lua_pop(L, 1);
}

static gboolean
mime_type_decision_cb(WebKitWebView *v, WebKitWebFrame *f,
        WebKitNetworkRequest *r, gchar *mime, WebKitWebPolicyDecision *pd,
        widget_t *w)
{
    (void) v;
    (void) f;
    lua_State *L = globalconf.L;
    const gchar *uri = webkit_network_request_get_uri(r);
    gint ret;

    luaH_object_push(L, w->ref);
    lua_pushstring(L, uri);
    lua_pushstring(L, mime);
    ret = luaH_object_emit_signal(L, -3, "mime-type-decision", 2, 1);

    if (ret && !luaH_checkboolean(L, -1))
        /* User responded with false, ignore request */
        webkit_web_policy_decision_ignore(pd);
    else if (!webkit_web_view_can_show_mime_type(v, mime))
        webkit_web_policy_decision_download(pd);
    else
        webkit_web_policy_decision_use(pd);

    lua_pop(L, ret + 1);
    return TRUE;
}

static gboolean
new_window_decision_cb(WebKitWebView *v, WebKitWebFrame *f,
        WebKitNetworkRequest *r, WebKitWebNavigationAction *na,
        WebKitWebPolicyDecision *pd, widget_t *w)
{
    (void) v;
    (void) f;
    lua_State *L = globalconf.L;
    const gchar *uri = webkit_network_request_get_uri(r);
    gchar *reason = NULL;
    gint ret = 0;

    luaH_object_push(L, w->ref);
    lua_pushstring(L, uri);

    switch (webkit_web_navigation_action_get_reason(na)) {

#define NR_CASE(a, l) case WEBKIT_WEB_NAVIGATION_REASON_##a: reason = l; break;
        NR_CASE(LINK_CLICKED,     "link-clicked");
        NR_CASE(FORM_SUBMITTED,   "form-submitted");
        NR_CASE(BACK_FORWARD,     "back-forward");
        NR_CASE(RELOAD,           "reload");
        NR_CASE(FORM_RESUBMITTED, "form-resubmitted");
        NR_CASE(OTHER,            "other");
#undef  NR_CASE

      default:
        warn("programmer error, unable to get web navigation reason literal");
        break;
    }

    lua_pushstring(L, reason);
    ret = luaH_object_emit_signal(L, -3, "new-window-decision", 2, 1);

    /* User responded with true, meaning a decision was made
     * and the signal was handled */
    if (ret && luaH_checkboolean(L, -1))
    {
        webkit_web_policy_decision_ignore(pd);
        lua_pop(L, ret + 1);

        return TRUE;
    }

    lua_pop(L, ret + 1);

    /* proceed with default behaviour */
    return FALSE;
}

static WebKitWebView*
create_web_view_cb(WebKitWebView *v, WebKitWebFrame *f, widget_t *w)
{
    (void) v;
    (void) f;
    gint ret;
    gint top;
    WebKitWebView *view = NULL;
    widget_t *new;

    lua_State *L = globalconf.L;
    luaH_object_push(L, w->ref);
    top = lua_gettop(L);
    ret = luaH_object_emit_signal(L, top, "create-web-view", 0, 1);
    if (ret && (new = luaH_checkudata(L, top + 1, &widget_class)))
        view = WEBKIT_WEB_VIEW(g_object_get_data(G_OBJECT(new->widget), "webview"));
    lua_pop(L, ret + 1);
    return view;
}

static gboolean
download_request_cb(WebKitWebView *v, GObject *dl, widget_t *w)
{
    (void) v;
    const gchar *uri = webkit_download_get_uri((WebKitDownload *) dl);
    const gchar *filename = webkit_download_get_suggested_filename((WebKitDownload *) dl);

    lua_State *L = globalconf.L;
    luaH_object_push(L, w->ref);
    lua_pushstring(L, uri);
    lua_pushstring(L, filename);
    luaH_object_emit_signal(L, -3, "download-request", 2, 0);
    lua_pop(L, 1);

    return FALSE;
}

static void
link_hover_cb(WebKitWebView *view, const char *t, const gchar *link, widget_t *w)
{
    (void) t;
    lua_State *L = globalconf.L;
    GObject *ws = G_OBJECT(view);
    gchar *last_hover = g_object_get_data(ws, "hovered-uri");

    /* links are identical, do nothing */
    if (last_hover && !g_strcmp0(last_hover, link))
        return;

    luaH_object_push(L, w->ref);

    if (last_hover) {
        lua_pushstring(L, last_hover);
        g_object_set_data(ws, "hovered-uri", NULL);
        luaH_object_emit_signal(L, -2, "link-unhover", 1, 0);
    }

    if (link) {
        lua_pushstring(L, link);
        g_object_set_data_full(ws, "hovered-uri", g_strdup(link), g_free);
        luaH_object_emit_signal(L, -2, "link-hover", 1, 0);
    }

    luaH_object_emit_signal(L, -1, "property::hovered_uri", 0, 0);
    lua_pop(L, 1);
}

/* Raises the "navigation-request" signal on a webkit navigation policy
 * decision request. The default action is to load the requested uri.
 *
 * The signal handler is able to:
 *  - return true for the handler execution to stop and the request to continue
 *  - return false for the handler execution to stop and the request to hault
 *  - do nothing and give the navigation decision to the next signal handler
 *
 * This signal is also where you would attach custom scheme handlers to take
 * over the navigation request by launching an external application.
 */
static gboolean
navigation_decision_cb(WebKitWebView *v, WebKitWebFrame *f,
        WebKitNetworkRequest *r, WebKitWebNavigationAction *a,
        WebKitWebPolicyDecision *p, widget_t *w)
{
    (void) v;
    (void) f;
    (void) a;

    lua_State *L = globalconf.L;
    const gchar *uri = webkit_network_request_get_uri(r);
    gint ret;

    luaH_object_push(L, w->ref);
    lua_pushstring(L, uri);
    ret = luaH_object_emit_signal(L, -2, "navigation-request", 1, 1);

    if (ret && !luaH_checkboolean(L, -1))
        /* User responded with false, do not continue navigation request */
        webkit_web_policy_decision_ignore(p);
    else
        webkit_web_policy_decision_use(p);

    lua_pop(L, ret + 1);
    return TRUE;
}

inline static gint
luaH_adjustment_push_values(lua_State *L, GtkAdjustment *a)
{
    gdouble view_size = gtk_adjustment_get_page_size(a);
    gdouble value = gtk_adjustment_get_value(a);
    gdouble max = gtk_adjustment_get_upper(a) - view_size;
    lua_pushnumber(L, value);
    lua_pushnumber(L, (max < 0 ? 0 : max));
    lua_pushnumber(L, view_size);
    return 3;
}

static gint
luaH_webview_get_vscroll(lua_State *L)
{
    widget_t *w = luaH_checkudata(L, 1, &widget_class);
    GtkAdjustment *a = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(w->widget));
    return luaH_adjustment_push_values(L, a);
}

static gint
luaH_webview_get_hscroll(lua_State *L)
{
    widget_t *w = luaH_checkudata(L, 1, &widget_class);
    GtkAdjustment *a = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(w->widget));
    return luaH_adjustment_push_values(L, a);
}

inline static void
adjustment_set(GtkAdjustment *a, gdouble new)
{
    gdouble view_size = gtk_adjustment_get_page_size(a);
    gdouble max = gtk_adjustment_get_upper(a) - view_size;
    gtk_adjustment_set_value(a, ((new < 0 ? 0 : new) > max ? max : new));
}

static gint
luaH_webview_set_scroll_vert(lua_State *L)
{
    widget_t *w = luaH_checkudata(L, 1, &widget_class);
    gdouble value = (gdouble) luaL_checknumber(L, 2);
    GtkAdjustment *a = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(w->widget));
    adjustment_set(a, value);
    return 0;
}

static gint
luaH_webview_set_scroll_horiz(lua_State *L)
{
    widget_t *w = luaH_checkudata(L, 1, &widget_class);
    gdouble value = (gdouble) luaL_checknumber(L, 2);
    GtkAdjustment *a = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(w->widget));
    adjustment_set(a, value);
    return 0;
}

static gint
luaH_webview_go_back(lua_State *L)
{
    widget_t *w = luaH_checkudata(L, 1, &widget_class);
    gint steps = (gint) luaL_checknumber(L, 2);
    GtkWidget *view = GTK_WIDGET(g_object_get_data(G_OBJECT(w->widget), "webview"));
    webkit_web_view_go_back_or_forward(WEBKIT_WEB_VIEW(view), steps * -1);
    return 0;
}

static gint
luaH_webview_go_forward(lua_State *L)
{
    widget_t *w = luaH_checkudata(L, 1, &widget_class);
    gint steps = (gint) luaL_checknumber(L, 2);
    GtkWidget *view = GTK_WIDGET(g_object_get_data(G_OBJECT(w->widget), "webview"));
    webkit_web_view_go_back_or_forward(WEBKIT_WEB_VIEW(view), steps);
    return 0;
}

static gint
luaH_webview_get_view_source(lua_State *L)
{
    widget_t *w = luaH_checkudata(L, 1, &widget_class);
    GtkWidget *view = GTK_WIDGET(g_object_get_data(G_OBJECT(w->widget), "webview"));
    lua_pushboolean(L, webkit_web_view_get_view_source_mode(WEBKIT_WEB_VIEW(view)));
    return 1;
}

static gint
luaH_webview_set_view_source(lua_State *L)
{
    const gchar *uri;
    widget_t *w = luaH_checkudata(L, 1, &widget_class);
    gboolean show = luaH_checkboolean(L, 2);
    GtkWidget *view = GTK_WIDGET(g_object_get_data(G_OBJECT(w->widget), "webview"));
    webkit_web_view_set_view_source_mode(WEBKIT_WEB_VIEW(view), show);
    if ((uri = webkit_web_view_get_uri(WEBKIT_WEB_VIEW(view))))
        webkit_web_view_load_uri(WEBKIT_WEB_VIEW(view), uri);
    return 0;
}

static gint
luaH_webview_reload(lua_State *L)
{
    widget_t *w = luaH_checkudata(L, 1, &widget_class);
    GtkWidget *view = GTK_WIDGET(g_object_get_data(G_OBJECT(w->widget), "webview"));
    webkit_web_view_reload(WEBKIT_WEB_VIEW(view));
    return 0;
}

static gint
luaH_webview_search(lua_State *L)
{
    widget_t *w = luaH_checkudata(L, 1, &widget_class);
    WebKitWebView *view = WEBKIT_WEB_VIEW(GTK_WIDGET(g_object_get_data(G_OBJECT(w->widget), "webview")));
    const gchar *text = luaL_checkstring(L, 2);
    gboolean case_sensitive = luaH_checkboolean(L, 3);
    gboolean forward = luaH_checkboolean(L, 4);
    gboolean wrap = luaH_checkboolean(L, 5);

    webkit_web_view_unmark_text_matches(view);
    webkit_web_view_search_text(view, text, case_sensitive, forward, wrap);
    webkit_web_view_mark_text_matches(view, text, case_sensitive, 0);
    webkit_web_view_set_highlight_text_matches(view, TRUE);
    return 0;
}

static gint
luaH_webview_clear_search(lua_State *L)
{
    widget_t *w = luaH_checkudata(L, 1, &widget_class);
    WebKitWebView *view = WEBKIT_WEB_VIEW(GTK_WIDGET(g_object_get_data(G_OBJECT(w->widget), "webview")));
    webkit_web_view_unmark_text_matches(view);
    return 0;
}

inline static GObject*
get_settings_object(GtkWidget *view, property_t *p)
{
    switch (p->scope) {
      case SETTINGS:
        return G_OBJECT(webkit_web_view_get_settings(WEBKIT_WEB_VIEW(view)));
      case WEBKITVIEW:
        return G_OBJECT(view);
      case SOUPSESSION:
        return G_OBJECT(Soup.session);
      case SOUPCOOKIEJAR:
	return G_OBJECT(Soup.cookiejar);
      default:
        break;
    }
    warn("programmer error: unknown settings scope for property: %s", p->name);
    return NULL;
}

static gint
luaH_webview_get_prop(lua_State *L)
{
    GObject *so;
    SoupURI *u;
    property_t *p;
    temp_value_t tmp;

    /* get webview widget */
    widget_t *w = luaH_checkudata(L, 1, &widget_class);
    GtkWidget *view = GTK_WIDGET(g_object_get_data(G_OBJECT(w->widget), "webview"));

    /* get property struct */
    const gchar *name = luaL_checkstring(L, 2);
    if ((p = g_hash_table_lookup(properties, name))) {
        so = get_settings_object(view, p);

        switch(p->type) {
          case BOOL:
            g_object_get(so, p->name, &tmp.b, NULL);
            lua_pushboolean(L, tmp.b);
            return 1;

          case INT:
            g_object_get(so, p->name, &tmp.i, NULL);
            lua_pushnumber(L, tmp.i);
            return 1;

          case FLOAT:
            g_object_get(so, p->name, &tmp.f, NULL);
            lua_pushnumber(L, tmp.f);
            return 1;

          case DOUBLE:
            g_object_get(so, p->name, &tmp.d, NULL);
            lua_pushnumber(L, tmp.d);
            return 1;

          case CHAR:
            g_object_get(so, p->name, &tmp.c, NULL);
            lua_pushstring(L, tmp.c);
            g_free(tmp.c);
            return 1;

          case URI:
            g_object_get(so, p->name, &u, NULL);
            tmp.c = soup_uri_to_string(u, 0);
            lua_pushstring(L, tmp.c);
            soup_uri_free(u);
            g_free(tmp.c);
            return 1;

          default:
            warn("programmer error: unknown property type for: %s", p->name);
            break;
        }
    }
    warn("unknown property: %s", name);
    return 0;
}

static gint
luaH_webview_set_prop(lua_State *L)
{
    size_t len;
    GObject *so;
    SoupURI *u;
    property_t *p;
    temp_value_t tmp;

    /* get webview widget */
    widget_t *w = luaH_checkudata(L, 1, &widget_class);
    GtkWidget *view = g_object_get_data(G_OBJECT(w->widget), "webview");

    /* get property struct */
    const gchar *name = luaL_checkstring(L, 2);
    if ((p = g_hash_table_lookup(properties, name))) {
        if (!p->writable) {
            warn("attempt to set read-only property: %s", p->name);
            return 0;
        }

        so = get_settings_object(view, p);
        switch(p->type) {
          case BOOL:
            tmp.b = luaH_checkboolean(L, 3);
            g_object_set(so, p->name, tmp.b, NULL);
            return 0;

          case INT:
            tmp.i = (gint) luaL_checknumber(L, 3);
            g_object_set(so, p->name, tmp.i, NULL);
            return 0;

          case FLOAT:
            tmp.f = (gfloat) luaL_checknumber(L, 3);
            g_object_set(so, p->name, tmp.f, NULL);
            return 0;

          case DOUBLE:
            tmp.d = (gdouble) luaL_checknumber(L, 3);
            g_object_set(so, p->name, tmp.d, NULL);
            return 0;

          case CHAR:
            tmp.c = (gchar*) luaL_checkstring(L, 3);
            g_object_set(so, p->name, tmp.c, NULL);
            return 0;

          case URI:
            tmp.c = (gchar*) luaL_checklstring(L, 3, &len);
            if (!len || g_strrstr(tmp.c, "://"))
                tmp.c = g_strdup(tmp.c);
            else
                tmp.c = g_strdup_printf("http://%s", tmp.c);
            u = soup_uri_new(tmp.c);
            if (!u || SOUP_URI_VALID_FOR_HTTP(u))
                g_object_set(so, p->name, u, NULL);
            else
                luaL_error(L, "cannot parse uri: %s", tmp.c);
            if (u) soup_uri_free(u);
            g_free(tmp.c);
            return 0;

          default:
            warn("programmer error: unknown property type for: %s", p->name);
            break;
        }
    }
    warn("unknown property: %s", name);
    return 0;
}

static gint
luaH_webview_loading(lua_State *L)
{
    widget_t *w = luaH_checkudata(L, 1, &widget_class);
    GtkWidget *view = g_object_get_data(G_OBJECT(w->widget), "webview");
    WebKitLoadStatus s;
    g_object_get(G_OBJECT(view), "load-status", &s, NULL);
    switch (s) {
      case WEBKIT_LOAD_PROVISIONAL:
      case WEBKIT_LOAD_COMMITTED:
      case WEBKIT_LOAD_FIRST_VISUALLY_NON_EMPTY_LAYOUT:
        lua_pushboolean(L, TRUE);
        break;

      default:
        lua_pushboolean(L, FALSE);
        break;
    }
    return 1;
}

void
show_scrollbars(widget_t *w, gboolean show)
{
    GtkWidget *view = g_object_get_data(G_OBJECT(w->widget), "webview");
    WebKitWebFrame *mf = webkit_web_view_get_main_frame(WEBKIT_WEB_VIEW(view));
    gulong id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(view), "hide_handler_id"));

    if (show) {
        if (id)
            g_signal_handler_disconnect((gpointer) mf, id);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(w->widget), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        id = 0;
    } else if (!id) {
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(w->widget), GTK_POLICY_NEVER, GTK_POLICY_NEVER);
        id = g_signal_connect(G_OBJECT(mf), "scrollbars-policy-changed", G_CALLBACK(true_cb), NULL);
    }
    g_object_set_data(G_OBJECT(view), "hide_handler_id", GINT_TO_POINTER(id));
}

static gint
luaH_webview_index(lua_State *L, luakit_token_t token)
{
    widget_t *w = luaH_checkudata(L, 1, &widget_class);
    GtkWidget *view = g_object_get_data(G_OBJECT(w->widget), "webview");
    temp_value_t tmp;

    switch(token)
    {
      LUAKIT_WIDGET_INDEX_COMMON

      /* push property methods */
      PF_CASE(GET_PROP,         luaH_webview_get_prop)
      PF_CASE(SET_PROP,         luaH_webview_set_prop)
      /* push scroll adjustment methods */
      PF_CASE(GET_SCROLL_HORIZ, luaH_webview_get_hscroll)
      PF_CASE(GET_SCROLL_VERT,  luaH_webview_get_vscroll)
      PF_CASE(SET_SCROLL_HORIZ, luaH_webview_set_scroll_horiz)
      PF_CASE(SET_SCROLL_VERT,  luaH_webview_set_scroll_vert)
      /* push search methods */
      PF_CASE(CLEAR_SEARCH,     luaH_webview_clear_search)
      PF_CASE(SEARCH,           luaH_webview_search)
      /* push history navigation methods */
      PF_CASE(GO_BACK,          luaH_webview_go_back)
      PF_CASE(GO_FORWARD,       luaH_webview_go_forward)
      /* push misc webview methods */
      PF_CASE(EVAL_JS,          luaH_webview_eval_js)
      PF_CASE(LOADING,          luaH_webview_loading)
      PF_CASE(RELOAD,           luaH_webview_reload)
      /* push source viewing methods */
      PF_CASE(GET_VIEW_SOURCE,  luaH_webview_get_view_source)
      PF_CASE(SET_VIEW_SOURCE,  luaH_webview_set_view_source)

      /* push string properties */
      PS_CASE(HOVERED_URI, g_object_get_data(G_OBJECT(view), "hovered-uri"))

      case L_TK_URI:
        g_object_get(G_OBJECT(view), "uri", &tmp.c, NULL);
        lua_pushstring(L, tmp.c);
        g_free(tmp.c);
        return 1;

      default:
        warn("unknown property: %s", luaL_checkstring(L, 2));
        break;
    }

    return 0;
}

/* The __newindex method for the webview object */
static gint
luaH_webview_newindex(lua_State *L, luakit_token_t token)
{
    size_t len;
    widget_t *w = luaH_checkudata(L, 1, &widget_class);
    GtkWidget *view = g_object_get_data(G_OBJECT(w->widget), "webview");
    temp_value_t tmp;

    switch(token)
    {
      case L_TK_URI:
        tmp.c = (gchar*) luaL_checklstring(L, 3, &len);
        if (g_strrstr(tmp.c, "://") || !g_strcmp0(tmp.c, "about:blank"))
            tmp.c = g_strdup(tmp.c);
        else
            tmp.c = g_strdup_printf("http://%s", tmp.c);
        webkit_web_view_load_uri(WEBKIT_WEB_VIEW(view), tmp.c);
        g_free(tmp.c);
        break;

      case L_TK_SHOW_SCROLLBARS:
        show_scrollbars(w, luaH_checkboolean(L, 3));
        return 0;

      default:
        warn("unknown property: %s", luaL_checkstring(L, 2));
        return 0;
    }

    return luaH_object_emit_property_signal(L, 1);
}

static gboolean
expose_cb(GtkWidget *widget, GdkEventExpose *e, widget_t *w)
{
    (void) e;
    (void) widget;
    lua_State *L = globalconf.L;
    luaH_object_push(L, w->ref);
    luaH_object_emit_signal(L, -1, "expose", 0, 0);
    lua_pop(L, 1);
    return FALSE;
}

static gboolean
wv_button_press_cb(GtkWidget *view, GdkEventButton *ev, widget_t *w)
{
    if((ev->type != GDK_BUTTON_PRESS) || (ev->button != 1))
        return FALSE;

    /* get webview hit context */
    WebKitHitTestResult *ht = webkit_web_view_get_hit_test_result(WEBKIT_WEB_VIEW(view), ev);
    guint c;
    g_object_get(ht, "context", &c, NULL);
    gint context = (gint) c;

    lua_State *L = globalconf.L;
    luaH_object_push(L, w->ref);
    /* raise "form-active" when a user clicks on a form field and raise
     * "root-active" when a user clicks elsewhere */
    if (context & WEBKIT_HIT_TEST_RESULT_CONTEXT_EDITABLE)
        luaH_object_emit_signal(L, -1, "form-active", 0, 0);
    else if (context & WEBKIT_HIT_TEST_RESULT_CONTEXT_DOCUMENT)
        luaH_object_emit_signal(L, -1, "root-active", 0, 0);
    lua_pop(L, 1);
    return FALSE;
}

static void
webview_destructor(widget_t *w)
{
    GtkWidget *view = g_object_get_data(G_OBJECT(w->widget), "webview");
    gtk_widget_destroy(GTK_WIDGET(view));
    gtk_widget_destroy(GTK_WIDGET(w->widget));
}

widget_t *
widget_webview(widget_t *w)
{
    w->index = luaH_webview_index;
    w->newindex = luaH_webview_newindex;
    w->destructor = webview_destructor;

    /* init properties hash table */
    if (!properties)
        webview_init_properties();

    /* init soup session & cookies handling */
    if (!Soup.session) {
        Soup.session = webkit_get_default_session();
        gchar *cookie_file = g_build_filename(globalconf.data_dir, "cookies.txt", NULL);
        Soup.cookiejar = soup_cookie_jar_text_new(cookie_file, FALSE);
        soup_session_add_feature(Soup.session, (SoupSessionFeature*) Soup.cookiejar);
        g_free(cookie_file);
    }

    GtkWidget *view = webkit_web_view_new();
    w->widget = gtk_scrolled_window_new(NULL, NULL);
    g_object_set_data(G_OBJECT(w->widget), "widget", w);
    g_object_set_data(G_OBJECT(w->widget), "webview", view);
    gtk_container_add(GTK_CONTAINER(w->widget), view);

    /* set initial scrollbars state */
    show_scrollbars(w, TRUE);

    /* connect webview signals */
    g_object_connect((GObject*)view,
      "signal::button-press-event",                   (GCallback)wv_button_press_cb,     w,
      "signal::button-release-event",                 (GCallback)button_release_cb,      w,
      "signal::create-web-view",                      (GCallback)create_web_view_cb,     w,
      "signal::download-requested",                   (GCallback)download_request_cb,    w,
      "signal::expose-event",                         (GCallback)expose_cb,              w,
      "signal::focus-in-event",                       (GCallback)focus_cb,               w,
      "signal::focus-out-event",                      (GCallback)focus_cb,               w,
      "signal::hovering-over-link",                   (GCallback)link_hover_cb,          w,
      "signal::key-press-event",                      (GCallback)key_press_cb,           w,
      "signal::mime-type-policy-decision-requested",  (GCallback)mime_type_decision_cb,  w,
      "signal::navigation-policy-decision-requested", (GCallback)navigation_decision_cb, w,
      "signal::new-window-policy-decision-requested", (GCallback)new_window_decision_cb, w,
      "signal::notify",                               (GCallback)notify_cb,              w,
      "signal::notify::load-status",                  (GCallback)notify_load_status_cb,  w,
      "signal::parent-set",                           (GCallback)parent_set_cb,          w,
      NULL);

    /* show widgets */
    gtk_widget_show(view);
    gtk_widget_show(w->widget);

    return w;
}

// vim: ft=c:et:sw=4:ts=8:sts=4:tw=80
