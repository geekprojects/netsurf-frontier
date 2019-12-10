
#include <stdlib.h>
#include <string.h>

#include "gui.h"
#include "browserwindow.h"

extern "C" {
#include "utils/nsoption.h"
#include "utils/filename.h"
#include "utils/log.h"
#include "utils/messages.h"
#include "utils/url.h"
#include "utils/corestrings.h"
#include "utils/utf8.h"
#include "utils/utils.h"
#include "utils/nsurl.h"
#include "netsurf/misc.h"
#include "netsurf/clipboard.h"
#include "netsurf/search.h"
#include "netsurf/fetch.h"
#include "netsurf/netsurf.h"
#include "netsurf/content.h"
#include "netsurf/browser_window.h"
#include "netsurf/cookie_db.h"
#include "netsurf/url_db.h"
#include "content/fetch.h"
#include "content/backing_store.h"
#include "netsurf/window.h"
#include "desktop/download.h"
#include "netsurf/download.h"
#include "netsurf/bitmap.h"
#include "netsurf/plotters.h"

nserror netsurf_path_to_nsurl(const char *path, struct nsurl **url);
}

#include <frontier/utils.h>
#include <geek/core-string.h>

#include "misc.h"
#include "layout.h"
#include "bitmap.h"

using namespace std;
using namespace Geek;
using namespace Geek::Core;
using namespace Geek::Gfx;
using namespace Frontier;

NetSurfApp* g_frontierApp = NULL;

NetSurfApp::NetSurfApp() : FrontierApp(L"NetSurf")
{
}

NetSurfApp::~NetSurfApp()
{
}

bool NetSurfApp::init()
{
    bool res;
    res = FrontierApp::init();
    if (!res)
    {
        return false;
    }

    m_timerManager = new TimerManager();
    m_timerManager->start();

    return true;
}

struct ScheduleCallback
{
    void (*callback)(void *p);
    void* data;
    void call(Timer* timer)
    {
        callback(data);
    }
};

bool NetSurfApp::schedule(int t, void (*callback)(void *p), void *p)
{
    char buffer[64];
    string id = "";
    sprintf(buffer, "%p:%p", callback, p);
    id = string(buffer);

    Timer* timer = m_timerManager->findTimer(id);

    if (timer == NULL)
    {
        if (t < 0)
        {
            log(WARN, "schedule: Attempted to cancel unknown timer: %s", id.c_str());
            return false;
        }

        //log(DEBUG, "schedule: Creating timer: %s, period=%d", id.c_str(), t);
        ScheduleCallback* scheduleCallback = new ScheduleCallback();
        scheduleCallback->callback = callback;
        scheduleCallback->data = p;

        Timer* timer = new Timer(id, TIMER_ONE_SHOT, t);
        timer->signal().connect(sigc::mem_fun(*scheduleCallback, &ScheduleCallback::call));
        m_timerManager->addTimer(timer);
    }
    else
    {
        if (t >= 0)
        {
            //log(DEBUG, "schedule: Resetting timer: %s, period=%d", id.c_str(), t);
            timer->setPeriod(t);
            m_timerManager->resetTimer(timer);
        }
        else
        {
            //log(DEBUG, "schedule: Cancelling timer: %s", id.c_str());
            m_timerManager->cancelTimer(timer);
        }
    }
    return true;
}

struct gui_window* NetSurfApp::createWindow(struct browser_window *bw, struct gui_window *existing, gui_window_create_flags flags)
{
    NetSurfWindow* frontierWindow = new NetSurfWindow(this);
    frontierWindow->show();
    frontierWindow->setBW(bw);

    gui_window* g = new gui_window();

    g->window = frontierWindow;
    g->bw = bw;

    return g;
}

FontHandle* NetSurfApp::createFontHandle(const struct plot_font_style *fstyle)
{
    string fontName = "";
    string style = "Regular";
    switch (fstyle->family)
    {
        case PLOT_FONT_FAMILY_SERIF:
            fontName = "Times";
            if (fstyle->flags & FONTF_ITALIC)
            {
                if (fstyle->weight > 400)
                {
                    style = "Bold Italic";
                }
                else
                {
                    style = "Italic";
                }
            }
            else if (fstyle->weight > 400)
            {
                style = "Bold";
            }
            break;

        case PLOT_FONT_FAMILY_MONOSPACE:
            fontName = "Courier";
            break;
        case PLOT_FONT_FAMILY_SANS_SERIF:
        case PLOT_FONT_FAMILY_FANTASY:
        case PLOT_FONT_FAMILY_CURSIVE:
        default:
            fontName = "Helvetica";
            if (fstyle->flags & FONTF_ITALIC)
            {
                if (fstyle->weight > 400)
                {
                    style = "Bold Oblique";
                }
                else
                {
                    style = "Oblique";
                }
            }
            else if (fstyle->weight > 400)
            {
                style = "Bold";
            }

            break;
    }

    int size = fstyle->size >> PLOT_STYLE_RADIX;

    char buf[1024];
    snprintf(buf, 1024, "%s::%s::%d", fontName.c_str(), style.c_str(), size);
    string fontSignature = string(buf);

    auto it = m_fontCache.find(fontSignature);
    if (it != m_fontCache.end())
    {
        return it->second;
    }
    else
    {
        FontHandle* handle = getFontManager()->openFont(fontName, style, size);
#if 1
        log(DEBUG, "createFontHandle: fontName=%s, size=%d, weight=%d, style=%s", fontName.c_str(), size, fstyle->weight, style.c_str());
#endif

        m_fontCache.insert(make_pair(fontSignature, handle));

        return handle;
    }
}

/*
static struct gui_download_table frontier_download_table =
{
    NULL, //gui_download_window_create,
    NULL, //gui_download_window_data,
    NULL, //gui_download_window_error,
    NULL, //gui_download_window_done,
};


static struct gui_clipboard_table frontier_clipboard_table =
{
    NULL, //gui_get_clipboard,
    NULL, //gui_set_clipboard,
};
*/

const char* frontier_fetch_filetype(const char *unix_path)
{
    printf("XXX: frontier_fetch_filetype: unix_path=%s\n", unix_path);

    string name = string(unix_path);
    size_t pos = name.rfind(".");
    string ext = "";

    if (pos != string::npos)
    {
        ext = name.substr(pos + 1);
    }

    if (ext == "css")
    {
        return "text/css";
    }
    else if (ext == "png")
    {
        return "image/png";
    }
    else if (ext == "svg")
    {
        return "image/svg";
    }
    else if (ext == "js")
    {
        return "text/javascript";
    }
    else if (ext == "htm" || ext == "html")
    {
        return "text/html";
    }
    else
    {
        return "text/plain";
    }
}

struct nsurl* frontier_fetch_get_resource_url(const char *path)
{
    nsurl* url = NULL;

    printf("XXX: frontier_fetch_get_resource_url: path=%s\n", path);
    std::string urlstr = "/Users/ian/Downloads/netsurf-all-3.8/netsurf/frontends/frontier/res/" + std::string(path);
    netsurf_path_to_nsurl(urlstr.c_str(), &url);
    printf("XXX: frontier_fetch_get_resource_url: url=%s\n", nsurl_access(url));
    return url;
}


// Required
static struct gui_fetch_table frontier_fetch_table = {
        frontier_fetch_filetype,
        frontier_fetch_get_resource_url,
        NULL, // ???
        NULL, // release_resource_data
        NULL, // fetch_mimetype
};

/**
 * Ensures output logging stream is correctly configured
 */
static bool nslog_stream_configure(FILE *fptr)
{
    /* set log stream to be non-buffering */
    setbuf(fptr, NULL);

    return true;
}

static nserror set_defaults(struct nsoption_s *defaults)
{
    //nsoption_set_bool(enable_javascript, true);
    return NSERROR_OK;
}

void gui_init(int argc, char** argv)
{
    char *addr;
    nsurl *url;
    nserror error;

    if (argc > 1)
    {
        addr = strdup(argv[1]);
    }
    else
    {
        addr = strdup("about:about");
    }

    error = nsurl_create(addr, &url);
    if (error == NSERROR_OK)
    {
        error = browser_window_create(
            BW_CREATE_HISTORY,
            url,
            NULL,
            NULL,
            NULL);

        printf("XXX: browser_window_create error=%d\n", error);
        nsurl_unref(url);
    }

    g_frontierApp->main();
}

int main(int argc, char** argv)
{
    nserror ret;
    struct netsurf_table frontier_table =
    {
        &g_frontier_misc_table,
        &g_frontier_window_table,
        NULL, //&frontier_download_table,
        NULL, //&frontier_clipboard_table,
        &frontier_fetch_table,
        NULL, /* use POSIX file */
        NULL, /* default utf8 */
        NULL, /* default search */
        NULL, /* default web search */
        filesystem_llcache_table, /* default low level cache persistant storage */
        &g_frontier_bitmap_table,
        &g_frontier_layout_table
    };

    ret = netsurf_register(&frontier_table);
    if (ret != NSERROR_OK)
    {
        printf("NetSurf operation table failed registration: %d", ret);
        exit(1);
    }
    printf("NetSurf registered!\n");

    g_frontierApp = new NetSurfApp();
    g_frontierApp->init();

    nslog_init(nslog_stream_configure, &argc, argv);

    ret = nsoption_init(set_defaults, &nsoptions, &nsoptions_default);
    if (ret != NSERROR_OK)
    {
        printf("Options failed to initialise\n");
        exit(1);
    }

    //nsoption_read(options.Path(), NULL);
    nsoption_commandline(&argc, argv, NULL);

    //nsoption_set_bool(enable_javascript, true);

    string configDir = g_frontierApp->getConfigDir();

    string storeDir = configDir + "/store";
    ret = netsurf_init(storeDir.c_str());
    if (ret != NSERROR_OK)
    {
        // FIXME: must not die when in replicant!
        printf("NetSurf failed to initialise\n");
        exit(1);
    }

  string cookiesfile = configDir + "/cookies";
  nsoption_setnull_charp(cookie_file, (char *)strdup(cookiesfile.c_str()));

    urldb_load_cookies(nsoption_charp(cookie_file));

    gui_init(argc, argv);

    urldb_save_cookies(nsoption_charp(cookie_file));

    netsurf_exit();

    /* finalise options */
    nsoption_finalise(nsoptions, nsoptions_default);

    /* finalise logging */
    nslog_finalise();
}

