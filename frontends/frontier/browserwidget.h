#ifndef __NETSURF_FRONTIER_BROWSERWIDGET_H_
#define __NETSURF_FRONTIER_BROWSERWIDGET_H_

#include <frontier/app.h>
#include <frontier/widgets.h>
#include <frontier/widgets/label.h>
#include <frontier/widgets/scrollbar.h>
#include <frontier/widgets/textinput.h>
#include <frontier/widgets/tabs.h>

#include <geek/core-thread.h>

extern "C" {
#include "utils/nsurl.h"
}

class NetSurfApp;
class BrowserWidget;

class BrowserWidget : public Frontier::Widget
{
 private:
    Geek::Mutex* m_drawMutex;
    Frontier::WindowCursor m_cursor;
    Geek::Rect m_caretRect;

 public:
    BrowserWidget(Frontier::FrontierWindow* window);
    virtual ~BrowserWidget();

    virtual void calculateSize();
    virtual void layout();
    virtual bool draw(Geek::Gfx::Surface* surface);

    virtual Widget* handleEvent(Frontier::Event* event);

    void setCursor(Frontier::WindowCursor cursor) { m_cursor = cursor; }
    virtual Frontier::WindowCursor getCursor() { return m_cursor; }
    void setCaret(int x, int y, int h);
    void removeCaret();
};


#endif
