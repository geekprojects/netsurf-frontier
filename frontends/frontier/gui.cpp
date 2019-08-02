
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
#include "netsurf/window.h"
#include "desktop/download.h"
#include "netsurf/download.h"
#include "netsurf/bitmap.h"
#include "netsurf/layout.h"
#include "netsurf/plotters.h"

nserror netsurf_path_to_nsurl(const char *path, struct nsurl **url);
}

#include <frontier/utils.h>
#include <geek/core-string.h>

using namespace std;
using namespace Geek;
using namespace Geek::Core;
using namespace Frontier;

void hexdump(const void* ptr, int len)
{
    const char* pos = (const char*)ptr;
    int i;
    for (i = 0; i < len; i += 16)
    {
        int j;
        printf("%08llx: ", (uint64_t)(pos + i));
        for (j = 0; j < 16 && (i + j) < len; j++)
        {
            printf("%02x ", (uint8_t)pos[i + j]);
        }
        for (j = 0; j < 16 && (i + j) < len; j++)
        {
            char c = pos[i + j];
            if (!isprint(c))
            {
                c = '.';
            }
            printf("%c", c);
        }
        printf("\n");
    }
}

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
    log(DEBUG, "createFontHandle: fontName=%s, size=%d, weight=%d, style=%s", fontName.c_str(), size, fstyle->weight, style.c_str());
    //

    FontHandle* handle = getFontManager()->openFont(fontName, style, size);
    //log(DEBUG, "createFontHandle: handle=%p", handle);

    return handle;
}

nserror NetSurfApp::layoutWidth(const struct plot_font_style *fstyle, const char *cstr, size_t length, int *width)
{
    log(DEBUG, "layout_width: string=%p, length=%d", cstr, length);

    if (cstr == NULL)
    {
        *width = 0;
        return NSERROR_OK;
    }

    FontHandle* font = createFontHandle(fstyle);

    wstring wstr = Geek::Core::utf82wstring(cstr, length);
    *width = getFontManager()->width(font, wstr);
    log(DEBUG, "layout_width: width=%d", *width);

    delete font;

    return NSERROR_OK;
}

nserror frontier_schedule(int t, void (*callback)(void *p), void *p)
{
    g_frontierApp->schedule(t, callback, p);

    return NSERROR_OK;
}

nserror frontier_warning(const char *message, const char *detail)
{
    printf("XXX: frontier_warning: %s (%s)\n", message, detail);
Frontier::Utils::stacktrace();
    return NSERROR_NOT_IMPLEMENTED;
}

// Required
static struct gui_misc_table frontier_misc_table = {
        frontier_schedule,
        frontier_warning,
        NULL, //gui_quit,
        NULL, //gui_launch_url,
        NULL, //cert_verify
        NULL, //gui_401login_open,
        NULL, // pdf_password (if we have Haru support)
};

struct gui_window* frontier_window_create(struct browser_window *bw, struct gui_window *existing, gui_window_create_flags flags)
{
    printf("XXX: frontier_window_create\n");
    return g_frontierApp->createWindow(bw, existing, flags);
}

void frontier_window_destroy(struct gui_window *gw)
{
    printf("XXX: frontier_window_destroy\n");
}

nserror frontier_window_invalidate(struct gui_window *gw, const struct rect *rect)
{
    printf("XXX: frontier_window_invalidate\n");
    gw->window->getBrowserWidget()->setDirty();
    gw->window->requestUpdate();

    return NSERROR_OK;
}

bool frontier_window_get_scroll(struct gui_window *gw, int *sx, int *sy)
{
    printf("XXX: frontier_window_get_scroll\n");
    return false;
}

nserror frontier_window_set_scroll(struct gui_window *gw, const struct rect *rect)
{
    printf("XXX: frontier_window_set_scroll\n");
    return NSERROR_OK;
}

nserror frontier_window_get_dimensions(struct gui_window *gw, int *width, int *height, bool scaled)
{
    BrowserWidget* widget = gw->window->getBrowserWidget();

    Size size = widget->getSize();
    *width = size.width;
    *height = size.height;
    printf("XXX: frontier_window_get_dimensions: %d, %d\n", *width, *height);

    return NSERROR_OK;
}

void frontier_window_update_extent(struct gui_window *gw)
{
int maxX;
int maxY;
int err;
    err = browser_window_get_extents(gw->window->getBW(), true, &maxX, &maxY);
    printf("XXX: frontier_window_update_extent: maxX=%d, maxY=%d\n", maxX, maxY);
if (err != NSERROR_OK)
{
return;
}
    gw->window->setExtent(maxX, maxY);
}

void frontier_window_set_title(struct gui_window *gw, const char *title)
{
    wstring wstr = Geek::Core::utf82wstring(title);

    gw->window->setTitle(wstr);
}

nserror frontier_window_set_url(struct gui_window *gw, struct nsurl *url)
{
    gw->window->setURL(url);
    return NSERROR_OK;
}

void frontier_window_set_status(struct gui_window *gw, const char *text)
{
    wstring wstr = Geek::Core::utf82wstring(text);

    gw->window->setStatus(wstr);
}

// Required
static struct gui_window_table frontier_window_table = {
        frontier_window_create,
        frontier_window_destroy,
        frontier_window_invalidate,
        frontier_window_get_scroll,
        frontier_window_set_scroll,
        frontier_window_get_dimensions,
        frontier_window_update_extent,

        /* from scaffold */
        frontier_window_set_title,
        frontier_window_set_url,
        NULL, //gui_window_set_icon,
        frontier_window_set_status,
        NULL, //gui_window_set_pointer,
        NULL, //gui_window_place_caret,
        NULL, //gui_window_remove_caret,
        NULL, //gui_window_start_throbber,
        NULL, //gui_window_stop_throbber,
        NULL, //drag_start
        NULL, //save_link
        NULL, //scroll_start
        NULL, //gui_window_new_content,
        NULL, //create_form_select_menu
        NULL, //file_gadget_open
        NULL, //drag_save_object
        NULL, //drag_save_selection
        NULL, //gui_start_selection
        NULL  //console_log
};

/*
static struct gui_download_table frontier_download_table = {
        NULL, //gui_download_window_create,
        NULL, //gui_download_window_data,
        NULL, //gui_download_window_error,
        NULL, //gui_download_window_done,
};


static struct gui_clipboard_table frontier_clipboard_table = {
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
    else
    {
        return "text/html";
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

void* frontier_bitmap_create(int width, int height, unsigned int state)
{
    printf("XXX: frontier_bitmap_create: width=%d, height=%d, state=%d\n", width, height, state);
    Geek::Gfx::Surface* surface = new Geek::Gfx::Surface(width, height, 4);
    printf("XXX: frontier_bitmap_create:  -> surface=%p\n", surface);

    return surface;
}

void frontier_bitmap_destroy(void *bitmap)
{
    Geek::Gfx::Surface* surface = (Geek::Gfx::Surface*)bitmap;
    printf("XXX: frontier_bitmap_destroy\n");
    delete surface;
}

void frontier_bitmap_set_opaque(void *bitmap, bool opaque)
{
    printf("XXX: frontier_bitmap_set_opaque\n");
}

bool frontier_bitmap_get_opaque(void *bitmap)
{
    printf("XXX: frontier_bitmap_get_opaque\n");
    return false;
}

bool frontier_bitmap_test_opaque(void *bitmap)
{
    printf("XXX: frontier_bitmap_test_opaque\n");
    return false;
}

unsigned char * frontier_bitmap_get_buffer(void *bitmap)
{
    printf("XXX: frontier_bitmap_get_buffer\n");
    Geek::Gfx::Surface* surface = (Geek::Gfx::Surface*)bitmap;
    return surface->getDrawingBuffer();
}

size_t frontier_bitmap_get_rowstride(void *bitmap)
{
    Geek::Gfx::Surface* surface = (Geek::Gfx::Surface*)bitmap;
    printf("XXX: frontier_bitmap_get_rowstride\n");
    return surface->getStride();
}

int frontier_bitmap_get_width(void *bitmap)
{
    Geek::Gfx::Surface* surface = (Geek::Gfx::Surface*)bitmap;
    printf("XXX: frontier_bitmap_get_width\n");
    return surface->getWidth();
}

int frontier_bitmap_get_height(void *bitmap)
{
    Geek::Gfx::Surface* surface = (Geek::Gfx::Surface*)bitmap;
    printf("XXX: frontier_bitmap_get_height\n");
    return surface->getHeight();
}

size_t frontier_bitmap_get_bpp(void *bitmap)
{
    printf("XXX: frontier_bitmap_get_bpp\n");
    return 4;
}

bool frontier_bitmap_save(void *bitmap, const char *path, unsigned flags)
{
    printf("XXX: frontier_bitmap_save: path=%s\n", path);
    return false;
}

void frontier_bitmap_modified(void *bitmap)
{
    printf("XXX: frontier_bitmap_modified\n");
}

nserror frontier_bitmap_render(struct bitmap *bitmap, struct hlcache_handle *content)
{
    printf("XXX: frontier_bitmap_render\n");
    Geek::Gfx::Surface* surface = (Geek::Gfx::Surface*)bitmap;

PlotterContext plotterContext;
plotterContext.surface = surface;
plotterContext.app = g_frontierApp;

    struct redraw_context ctx =
    {
        .interactive = false,
        .background_images = true,
        .plot = &frontier_plotter_table,
.priv = &plotterContext
    };

    int surfaceWidth = surface->getWidth();
    int surfaceHeight = surface->getHeight();

    int cwidth = min(max(content_get_width(content), surfaceWidth), 1024);

    /* The height is set in proportion with the width, according to the
    * aspect ratio of the required thumbnail. */
    int cheight = ((cwidth * surfaceHeight) + (surfaceWidth / 2)) / surfaceHeight;

    content_scaled_redraw(content, cwidth, cheight, &ctx);

    return NSERROR_OK;
}

// Required
static struct gui_bitmap_table frontier_bitmap_table = {
        /*.create =*/ frontier_bitmap_create,
        /*.destroy =*/ frontier_bitmap_destroy,
        /*.set_opaque =*/ frontier_bitmap_set_opaque,
        /*.get_opaque =*/ frontier_bitmap_get_opaque,
        /*.test_opaque =*/ frontier_bitmap_test_opaque,
        /*.get_buffer =*/ frontier_bitmap_get_buffer,
        /*.get_rowstride =*/ frontier_bitmap_get_rowstride,
        /*.get_width =*/ frontier_bitmap_get_width,
        /*.get_height =*/ frontier_bitmap_get_height,
        /*.get_bpp =*/ frontier_bitmap_get_bpp,
        /*.save =*/ frontier_bitmap_save,
        /*.modified =*/ frontier_bitmap_modified,
        /*.render =*/ frontier_bitmap_render,
};

nserror frontier_layout_width(const struct plot_font_style *fstyle, const char *string, size_t length, int *width)
{
    g_frontierApp->layoutWidth(fstyle, string, length, width);
    return NSERROR_OK;
}

nserror frontier_layout_position(const struct plot_font_style *fstyle, const char* text, size_t length, int x, size_t *char_offset, int *actual_x)
{
    printf("XXX: frontier_layout_position\n");
#if 0
    FontHandle* font = g_frontierApp->getTheme()->getFont(true);

    string str = string(text, length);
    wstring wstr = Frontier::Utils::string2wstring(str);

    FontManager* fm = g_frontierApp->getFontManager();

    size_t pos = 1;
    int width = 0;
    for (pos = 1; pos < length; pos++)
    {
        wstring currentStr = wstr.substr(0, pos);
        int currentWidth = fm->width(font, currentStr);
        printf("XXX: frontier_layout_position: pos=%zu, currentWidth=%d: %ls\n", pos, currentWidth, currentStr.c_str());

        if (currentWidth > x)
        {
            pos--;
            break;
        }

        width = currentWidth;
    }

    *char_offset = pos;
    *actual_x = width;

    return NSERROR_OK;
#endif
    return NSERROR_NOT_IMPLEMENTED;
}

nserror frontier_layout_split(const struct plot_font_style *fstyle, const char* text, size_t length, int x, size_t *char_offset, int *actual_x)
{
    printf("XXX: frontier_layout_split: length=%zu, x=%d\n", length, x);

    wstring wstr = Geek::Core::utf82wstring(text, length);

    FontManager* fm = g_frontierApp->getFontManager();
    FontHandle* font = g_frontierApp->createFontHandle(fstyle);

    size_t pos = 1;
    int currentX = 0;
    int lastSpacePos =  0;
    int lastSpaceX =  0;
    for (pos = 0; pos < wstr.length(); pos++)
    {
        if (wstr[pos] == ' ')
        {
            lastSpaceX = currentX;
            lastSpacePos = pos;
        }

        if (currentX >= x)
        {
            if (lastSpacePos != 0)
            {
                *actual_x = lastSpaceX;
                *char_offset = lastSpacePos;
            }
            else
            {
                *actual_x = currentX;
                *char_offset = pos;
            }

            delete font;
            return NSERROR_OK;
        }

        wstring currentStr = wstr.substr(0, pos);
        currentX = fm->width(font, currentStr);
        //printf("XXX: frontier_layout_split: pos=%zu, currentX=%d: %ls\n", pos, currentX, currentStr.c_str());
    }

    *char_offset = pos;
    *actual_x = currentX;

    delete font;

    return NSERROR_OK;
}


// Required
static struct gui_layout_table frontier_layout_table = {
  /*.width = */ frontier_layout_width,
  /*.position = */ frontier_layout_position,
  /*.split = */ frontier_layout_split
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
    return NSERROR_OK;
}

void gui_init(int argc, char** argv)
{
        char *addr;
        nsurl *url;
        nserror error;


//addr = strdup("https://www.hackaday.com");
//addr = strdup("about:license");
if (argc > 1)
{
        addr = strdup(argv[1]);
}
else
{
        addr = strdup("about:about");
}
//addr = strdup("http://www.righto.com/");
//addr = strdup("http://www.example.com/");
//addr = strdup("file:///Users/ian/Downloads/netsurf-all-3.8/index.html");

        error = nsurl_create(addr, &url);
        if (error == NSERROR_OK) {
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
        &frontier_misc_table,
        &frontier_window_table,
        NULL, //&frontier_download_table,
        NULL, //&frontier_clipboard_table,
        &frontier_fetch_table,
        NULL, /* use POSIX file */
        NULL, /* default utf8 */
        NULL, /* default search */
        NULL, /* default web search */
        NULL, /* default low level cache persistant storage */
        &frontier_bitmap_table,
        &frontier_layout_table
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
    if (ret != NSERROR_OK) {
        // FIXME: must not die when in replicant!
        printf("Options failed to initialise\n");
        exit(1);
    }
    //nsoption_read(options.Path(), NULL);
    nsoption_commandline(&argc, argv, NULL);

    ret = netsurf_init(NULL);
    if (ret != NSERROR_OK)
    {
        // FIXME: must not die when in replicant!
        printf("NetSurf failed to initialise\n");
        exit(1);
    }

    gui_init(argc, argv);

    netsurf_exit();

    /* finalise options */
    nsoption_finalise(nsoptions, nsoptions_default);

    /* finalise logging */
    nslog_finalise();
}


