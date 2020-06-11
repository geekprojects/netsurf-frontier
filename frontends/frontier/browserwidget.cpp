
#include "frontier/browserwidget.h"
#include "frontier/browserwindow.h"
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
#include "netsurf/keypress.h"
#include "utils/utf8.h"
}

using namespace Frontier;
using namespace Geek;
using namespace Geek::Gfx;
using namespace std;

BrowserWidget::BrowserWidget(FrontierWindow* window) : Widget(window, L"NetSurfBrowser")
{
    m_drawMutex = Thread::createMutex();
    m_caretRect.h = 0;
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

        if (m_caretRect.h > 0)
        {
            surface->drawRect(m_caretRect.x, m_caretRect.y, 1, m_caretRect.h, 0xff000000);
        }
    }
    m_drawMutex->unlock();

    return true;
}

static uint32_t keyEventToUCS4(KeyEvent* keyEvent)
{
    wchar_t c = keyEvent->chr;
    uint32_t uc = 0;

    switch (keyEvent->key)
    {
        case KC_BACKSPACE:
            uc = NS_KEY_DELETE_LEFT;
            break;

        case KC_TAB:
            uc = NS_KEY_TAB;
            break;

        case KC_KP_ENTER:
        case KC_RETURN:
            uc = NS_KEY_NL;
            break;

        default:
           if (iswprint(c))
           {
               wstring cs = wstring(1, c);
               string cu = Geek::Core::wstring2utf8(cs);
               uc = utf8_to_ucs4(cu.c_str(), cu.length());
           }
           break;
    }

    return uc;
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
                flags |= BROWSER_MOUSE_PRESS_1;
            }
            else
            {
                flags |= BROWSER_MOUSE_CLICK_1;
            }

            int scrollY = window->getScrollBarV()->getPos();
            int x = mouseButtonEvent->x - pos.x;
            int y = (mouseButtonEvent->y - pos.y) + scrollY;
            x /= browser_window_get_scale(bw);
            y /= browser_window_get_scale(bw);

            if (flags != 0)
            {
                browser_window_mouse_click(
                    bw,
                    (browser_mouse_state)flags,
                    x,
                    y);
            }
            else
            {
                browser_window_mouse_track(bw, (browser_mouse_state)0,
                    x,
                    y);
            }

            setDirty(DIRTY_CONTENT);
        } break;

        case FRONTIER_EVENT_MOUSE_MOTION:
        {
            MouseMotionEvent* mouseMotionEvent = (MouseMotionEvent*)event;
            Vector2D pos = getAbsolutePosition();

            int scrollY = window->getScrollBarV()->getPos();
            browser_window_mouse_track(bw, (browser_mouse_state)0,
                mouseMotionEvent->x - pos.x,
                (mouseMotionEvent->y - pos.y) + scrollY);

            setDirty(DIRTY_CONTENT);
            return this;
        }


        case FRONTIER_EVENT_MOUSE_SCROLL:
            window->getScrollBarV()->handleEvent(event);
            setDirty(DIRTY_CONTENT);
            return this;

        case FRONTIER_EVENT_KEY:
        {
            KeyEvent* keyEvent = (KeyEvent*)event;

            if (keyEvent->direction)
            {
                uint32_t uc = keyEventToUCS4(keyEvent);
                if (uc != 0)
                {

                    log(DEBUG, "handleEvent: FRONTIER_EVENT_KEY: 0x%x", uc);
                    browser_window_key_press(bw, uc);
                }
            }

            return this;
        }

        default:
            break;
    }
    return this;
}

void BrowserWidget::setCaret(int x, int y, int h)
{
    m_caretRect.x = x;
    m_caretRect.y = y;
    m_caretRect.h = h;
}

void BrowserWidget::removeCaret()
{
    m_caretRect.h = 0;
}


