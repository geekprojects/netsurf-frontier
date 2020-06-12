
#include "frontier/browserwindow.h"
#include "frontier/browserwidget.h"
#include "frontier/gui.h"

#include <frontier/widgets/frame.h>
#include <frontier/widgets/iconbutton.h>
#include <frontier/widgets/label.h>
#include <frontier/widgets/textinput.h>
#include <frontier/widgets/tabs.h>
#include <frontier/widgets/scrollbar.h>

#include <geek/core-string.h>

extern "C" {
#include "netsurf/plotters.h"
#include "desktop/browser_history.h"
#include "desktop/searchweb.h"
#include "desktop/search.h"
}

using namespace Frontier;
using namespace Geek;
using namespace Geek::Gfx;
using namespace std;

NetSurfWindow::NetSurfWindow(NetSurfApp* app) : FrontierWindow(app, L"NetSurf Browser", WINDOW_NORMAL)
{
    m_netsurfApp = app;
    m_bw = NULL;

}

NetSurfWindow::~NetSurfWindow()
{
if (m_bw != NULL)
{
    browser_window_destroy(m_bw);
}
}


bool NetSurfWindow::init()
{
    m_tabs = new Tabs(this);

    m_tab = new Frame(this, false);
    m_tabs->addTab(L"Tab 1", m_tab, true);

    Frame* navFrame = new Frame(this, true);
    m_tab->add(navFrame);

    IconButton* backButton;
    navFrame->add(backButton = new IconButton(getApp(), getApp()->getTheme()->getIcon(FRONTIER_ICON_BACKWARD)));
    navFrame->add(new IconButton(getApp(), getApp()->getTheme()->getIcon(FRONTIER_ICON_FORWARD)));
    navFrame->add(m_urlBar = new TextInput(getApp(), L""));
    m_urlBar->signalEditingEnd().connect(sigc::mem_fun(*this, &NetSurfWindow::onAddressEntered));

    backButton->clickSignal().connect(sigc::mem_fun(*this, &NetSurfWindow::onBackButton));

    Frame* browserFrame = new Frame(getApp(), true);
    m_browserWidget = new BrowserWidget(this);
    browserFrame->add(m_browserWidget);
    browserFrame->add(m_vScrollBar = new ScrollBar(getApp(), false));
    m_tab->add(browserFrame);
    m_tab->add(m_statusLabel = new Label(getApp(), L"", ALIGN_LEFT));
    setContent(m_tabs);

    m_vScrollBar->changedPositionSignal().connect(sigc::mem_fun(*this, &NetSurfWindow::onScrollV));

    return true;
}

void NetSurfWindow::setTitle(wstring title)
{
    Tab* tab = m_tabs->getTab(m_tab);
    if (tab != NULL)
    {
        tab->setTitle(title);
    }
}

void NetSurfWindow::setStatus(wstring title)
{
    m_statusLabel->setText(title);
    requestUpdate();
}

void NetSurfWindow::setURL(nsurl* url)
{
    const char* urlc = nsurl_access(url);
    string str = string(urlc);
    wstring wstr = Utils::string2wstring(str);

    m_urlBar->setText(wstr);
}

void NetSurfWindow::setIcon(Surface* surface)
{
    Tab* tab = m_tabs->getTab(m_tab);
    if (tab == NULL)
    {
        return;
    }

    Icon* icon = NULL;
    if (surface != NULL)
    {
        icon = new SurfaceIcon(getApp()->getTheme(), surface);
    }
    else
    {
        icon = getApp()->getTheme()->getIcon(FRONTIER_ICON_PAGE4);
    }
    tab->setIcon(icon);
}

void NetSurfWindow::newContent()
{
    m_vScrollBar->setPos(0);
}

void NetSurfWindow::setExtent(int x, int y)
{
    m_vScrollBar->set(0, y, m_browserWidget->getSize().height);
}

void NetSurfWindow::onBackButton(Widget* widget)
{
    if (browser_window_back_available(m_bw))
    {
        browser_window_history_back(m_bw, false);
    }
}

void NetSurfWindow::onScrollV(int pos)
{
    browser_window_scroll_at_point(m_bw, 0, pos, 0, 1);
}

void NetSurfWindow::onAddressEntered(Frontier::TextInput* widget)
{
    wstring urlws = widget->getText();
    string urls = Frontier::Utils::wstring2string(urlws);
    log(DEBUG, "onAddressEntered: URL: %s", urls.c_str());

    nsurl* url;
    nserror ret;
    ret = search_web_omni(
        urls.c_str(),
        SEARCH_WEB_OMNI_NONE,
        &url);
    if (ret == NSERROR_OK)
    {
        browser_window_navigate(m_bw, url, NULL, BW_NAVIGATE_HISTORY, NULL, NULL, NULL);
    }
}

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
    gw->window->getBrowserWidget()->setDirty(DIRTY_CONTENT);
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

nserror frontier_window_get_dimensions(struct gui_window *gw, int *width, int *height)
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

void frontier_window_new_content(struct gui_window *gw);

nserror frontier_window_event(struct gui_window *gw, enum gui_window_event event)
{
    switch (event)
    {
        case GW_EVENT_UPDATE_EXTENT:
            frontier_window_update_extent(gw);
            return NSERROR_OK;

        case GW_EVENT_NEW_CONTENT:
            frontier_window_new_content(gw);
            return NSERROR_OK;


        default:
            printf("XXX: frontier_window_event: Unknown event: %d\n", event);
            return NSERROR_OK;
    }
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

void frontier_window_set_icon(struct gui_window *gw, struct hlcache_handle *icon)
{
    printf("XXX: frontier_window_set_icon: icon=%p\n", icon);

    if (icon != NULL)
    {
        struct bitmap *icon_bitmap = NULL;
        icon_bitmap = content_get_bitmap(icon);
        printf("XXX: frontier_window_set_icon: icon_bitmap=%p\n", icon_bitmap);

        Surface* surface = (Surface*)icon_bitmap;
        gw->window->setIcon(surface);
    }
    else
    {
        printf("XXX: frontier_window_set_icon: Clearing icon\n");
        gw->window->setIcon(NULL);
    }
}

void frontier_window_set_status(struct gui_window *gw, const char *text)
{
    wstring wstr = Geek::Core::utf82wstring(text);

    gw->window->setStatus(wstr);
}

void frontier_window_new_content(struct gui_window *gw)
{
    printf("XXX: frontier_window_new_content: Here!\n");
    gw->window->newContent();
}

void frontier_window_set_pointer(struct gui_window *gw, enum gui_pointer_shape shape)
{
    Frontier::WindowCursor cursor;
    switch (shape)
    {
        case GUI_POINTER_POINT:
            cursor = CURSOR_POINTING;
            break;
        case GUI_POINTER_CARET:
            cursor = CURSOR_EDIT;
            break;
        default:
            cursor = CURSOR_ARROW;
            break;
    }

    gw->window->getBrowserWidget()->setCursor(cursor);
}

void frontier_window_place_caret(struct gui_window* gw, int x, int y, int height, const struct rect *clip)
{
    y += 1;
    height -= 1;

    if (y < clip->y0)
    {
        height -= clip->y0 - y;
        y = clip->y0;
    }

    if (y + height > clip->y1)
    {
        height = clip->y1 - y + 1;
    }

    gw->window->getBrowserWidget()->setCaret(x, y, height);
}

void frontier_window_remove_caret(struct gui_window *gw)
{
    gw->window->getBrowserWidget()->removeCaret();
}

void frontier_console_log(
    struct gui_window *gw,
    browser_window_console_source src,
    const char *msg,
    size_t msglen,
    browser_window_console_flags flags)
{
    printf("XXX: frontier_console_log: %s\n", msg);
}

// Required
struct gui_window_table g_frontier_window_table =
{
    frontier_window_create,
    frontier_window_destroy,
    frontier_window_invalidate,
    frontier_window_get_scroll,
    frontier_window_set_scroll,
    frontier_window_get_dimensions,
    //frontier_window_update_extent,
    frontier_window_event,

    /* from scaffold */
    frontier_window_set_title,
    frontier_window_set_url,
    frontier_window_set_icon,
    frontier_window_set_status,
    frontier_window_set_pointer,
    frontier_window_place_caret,
    //frontier_window_remove_caret,
    //NULL, //gui_window_start_throbber,
    //NULL, //gui_window_stop_throbber,
    NULL, //drag_start
    NULL, //save_link
    //NULL, //scroll_start
    //frontier_window_new_content,
    NULL, //create_form_select_menu
    NULL, //file_gadget_open
    NULL, //drag_save_object
    NULL, //drag_save_selection
    frontier_console_log  //console_log
};


