#ifndef __NETSURF_FRONTEND_FRONTIER_PLOTTERS_H_
#define __NETSURF_FRONTEND_FRONTIER_PLOTTERS_H_

#include "gui.h"

class FrontierPlotter
{
 private:
    NetSurfApp* m_app;

 public:
    FrontierPlotter(NetSurfApp* app);
    ~FrontierPlotter();
};

#endif
