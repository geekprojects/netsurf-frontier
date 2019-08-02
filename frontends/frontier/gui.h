#ifndef __NETSURF_FRONTIER_GUI_H_
#define __NETSURF_FRONTIER_GUI_H_

#include <stdlib.h>
#include <string.h>

extern "C" {
#include "utils/utils.h"
#include "netsurf/content.h"
#include "netsurf/browser_window.h"
#include "netsurf/window.h"
}

#include <frontier/frontier.h>
#include <geek/core-timers.h>

class NetSurfWindow;
class NetSurfApp;

struct PlotterContext
{
    Geek::Gfx::Surface* surface;
    NetSurfApp* app;
};

extern const struct plotter_table frontier_plotter_table;

struct gui_window
{
    NetSurfWindow* window;
    struct browser_window* bw;
};

class NetSurfApp : public Frontier::FrontierApp
{
 private:
    Geek::Core::TimerManager* m_timerManager;

 public:
    NetSurfApp();
    ~NetSurfApp();

    virtual bool init();

    bool schedule(int t, void (*callback)(void *p), void *p);

    struct gui_window* createWindow(struct browser_window *bw, struct gui_window *existing, gui_window_create_flags flags);

    Geek::FontHandle* createFontHandle(const struct plot_font_style *fstyle);

    nserror layoutWidth(const struct plot_font_style *fstyle, const char *string, size_t length, int *width);
};

extern NetSurfApp* g_frontierApp;

#endif
