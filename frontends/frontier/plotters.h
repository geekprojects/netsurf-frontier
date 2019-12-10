#ifndef __NETSURF_FRONTEND_FRONTIER_PLOTTERS_H_
#define __NETSURF_FRONTEND_FRONTIER_PLOTTERS_H_

#include "gui.h"
#include "netsurf/plotters.h"

class FrontierPlotter
{
 private:
    NetSurfApp* m_app;
    Frontier::Rect m_clip;

 public:
    FrontierPlotter(NetSurfApp* app);
    ~FrontierPlotter();

    Frontier::Rect& getClip() { return m_clip; }
};

#endif
