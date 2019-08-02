#ifndef __NETSURF_FRONTIER_BROWSERWINDOW_H_
#define __NETSURF_FRONTIER_BROWSERWINDOW_H_

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

class NetSurfWindow : public Frontier::FrontierWindow
{
 private:
    NetSurfApp* m_netsurfApp;
    BrowserWidget* m_browserWidget;
    struct browser_window* m_bw;

    Frontier::Tabs* m_tabs;
    Frontier::Frame* m_tab;
    Frontier::TextInput* m_urlBar;
    Frontier::Label* m_statusLabel;
    Frontier::ScrollBar* m_vScrollBar;

    virtual bool init();

 public:
    NetSurfWindow(NetSurfApp* app);
    ~NetSurfWindow();

    BrowserWidget* getBrowserWidget() { return m_browserWidget; }
    Frontier::ScrollBar* getScrollBarV() { return m_vScrollBar; }

    void setBW(struct browser_window* bw) { m_bw = bw; }
    struct browser_window* getBW() { return m_bw; }
    void setURL(nsurl* url);
    void setTitle(std::wstring title);
    void setStatus(std::wstring title);

    void setExtent(int x, int y);

    void onBackButton();
    void onScrollV(int pos);
};

class BrowserWidget : public Frontier::Widget
{
 private:
    Geek::Mutex* m_drawMutex;

 public:
    BrowserWidget(Frontier::FrontierWindow* window);
    virtual ~BrowserWidget();

    virtual void calculateSize();
    virtual void layout();
    virtual bool draw(Geek::Gfx::Surface* surface);

    virtual Widget* handleEvent(Frontier::Event* event);
};


#endif
