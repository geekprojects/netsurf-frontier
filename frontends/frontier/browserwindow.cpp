
#include "frontier/browserwindow.h"
#include "frontier/gui.h"

#include <frontier/widgets/frame.h>
#include <frontier/widgets/iconbutton.h>
#include <frontier/widgets/label.h>
#include <frontier/widgets/textinput.h>
#include <frontier/widgets/tabs.h>
#include <frontier/widgets/scrollbar.h>

extern "C" {
#include "netsurf/plotters.h"
#include "desktop/browser_history.h"
}

using namespace Frontier;
using namespace Geek;
using namespace std;

NetSurfWindow::NetSurfWindow(NetSurfApp* app) : FrontierWindow(app, L"NetSurf Browser", WINDOW_NORMAL)
{
    m_netsurfApp = app;
    m_bw = NULL;

}

NetSurfWindow::~NetSurfWindow()
{
}


bool NetSurfWindow::init()
{
    m_tabs = new Tabs(this);

    m_tab = new Frame(this, false);
    m_tabs->addTab(L"Tab 1", m_tab);

    Frame* navFrame = new Frame(this, true);
    m_tab->add(navFrame);

    IconButton* backButton;
    navFrame->add(backButton = new IconButton(this, getApp()->getTheme()->getIcon(FRONTIER_ICON_BACKWARD)));
    navFrame->add(new IconButton(this, getApp()->getTheme()->getIcon(FRONTIER_ICON_FORWARD)));
    navFrame->add(m_urlBar = new TextInput(this, L""));

    backButton->clickSignal().connect(sigc::mem_fun(*this, &NetSurfWindow::onBackButton));

    Frame* browserFrame = new Frame(this, true);
    m_browserWidget = new BrowserWidget(this);
    browserFrame->add(m_browserWidget);
    browserFrame->add(m_vScrollBar = new ScrollBar(this));
    m_tab->add(browserFrame);
    m_tab->add(m_statusLabel = new Label(this, L"", ALIGN_LEFT));
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

void NetSurfWindow::setExtent(int x, int y)
{
m_vScrollBar->set(0, y, m_browserWidget->getSize().height);
}

void NetSurfWindow::onBackButton()
{
    if (browser_window_back_available(m_bw))
    {
        browser_window_history_back(m_bw, false);
    }
}

void NetSurfWindow::onScrollV(int pos)
{
log(DEBUG, "onScrollV: pos=%d", pos);
    browser_window_scroll_at_point(m_bw, 0, pos, 0, 1);
}

BrowserWidget::BrowserWidget(FrontierWindow* window) : Widget(window, L"NetSurfBrowser")
{
    m_drawMutex = Thread::createMutex();
}

BrowserWidget::~BrowserWidget()
{
}

void BrowserWidget::calculateSize()
{
    m_minSize.width = 100;
    m_minSize.height = 100;

    m_maxSize.width = WIDGET_SIZE_UNLIMITED;
    m_maxSize.height = WIDGET_SIZE_UNLIMITED;
}

void BrowserWidget::layout()
{
    NetSurfWindow* window = (NetSurfWindow*)getWindow();
    browser_window* bw = window->getBW();

    if (bw != NULL && browser_window_has_content(bw))
    {
        browser_window_schedule_reformat(bw);
    }
}

bool BrowserWidget::draw(Geek::Gfx::Surface* surface)
{
    m_drawMutex->lock();

    NetSurfWindow* window = (NetSurfWindow*)getWindow();
    browser_window* bw = window->getBW();

    surface->clear(0xffffff);

    if (bw != NULL && browser_window_has_content(bw))
    {
        PlotterContext plotterContext;
        plotterContext.surface = surface;
        plotterContext.app = (NetSurfApp*)getApp();

        struct redraw_context ctx =
        {
            .interactive = true,
            .background_images = true,
            .plot = &frontier_plotter_table,
            .priv = &plotterContext
        };

        struct rect clip;
        clip.x0 = 0;
        clip.y0 = 0;//window->getScrollBarV()->getPos();
        clip.x1 = surface->getWidth();
        clip.y1 = surface->getHeight();

        browser_window_redraw(
            window->getBW(),
            0,
            -window->getScrollBarV()->getPos(),
            &clip,
            &ctx);

    }
m_drawMutex->unlock();

    return true;
}

Widget* BrowserWidget::handleEvent(Frontier::Event* event)
{
    NetSurfWindow* window = (NetSurfWindow*)getWindow();
    browser_window* bw = window->getBW();

    switch (event->eventType)
    {
        case FRONTIER_EVENT_MOUSE_BUTTON:
        {
            MouseButtonEvent* mouseButtonEvent = (MouseButtonEvent*)event;

            Vector2D pos = getAbsolutePosition();

            int flags = 0;
            if (mouseButtonEvent->direction)
            {
                flags |= BROWSER_MOUSE_CLICK_1;
            }

            browser_window_mouse_click(
                bw,
                (browser_mouse_state)flags,
                mouseButtonEvent->x - pos.x,
                mouseButtonEvent->y - pos.y);

            setDirty(DIRTY_CONTENT);
        } break;

        case FRONTIER_EVENT_MOUSE_MOTION:
        {
/*
            MouseMotionEvent* mouseMotionEvent = (MouseMotionEvent*)event;
            Vector2D pos = getAbsolutePosition();

            browser_window_mouse_track(bw, (browser_mouse_state)0,
                mouseMotionEvent->x - pos.x,
                mouseMotionEvent->y - pos.y);

            setDirty(DIRTY_CONTENT);
*/
            return this;
        } break;

        default:
            break;
    }
    return this;
}

