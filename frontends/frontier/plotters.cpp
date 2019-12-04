
#include "frontier/gui.h"

#include "netsurf/plotters.h"

#include <geek/core-string.h>

#include <typeinfo>

using namespace Frontier;
using namespace Geek;
using namespace Geek::Gfx;
using namespace std;

//#define RGB2BGR(a_ulColor) (a_ulColor & 0xFF000000) | ((a_ulColor & 0xFF0000) >> 16) | (a_ulColor & 0x00FF00) | ((a_ulColor & 0x0000FF) << 16)
#define RGB2BGR(a_ulColor) ((0xFF000000) | ((a_ulColor & 0xFF0000) >> 16) | (a_ulColor & 0x00FF00) | ((a_ulColor & 0x0000FF) << 16))

nserror frontier_plotter_clip(const struct redraw_context *ctx, const struct rect *clip)
{
#if 0
    if (clip != NULL)
    {
        printf("XXX: frontier_plotter_clip:  -> %d, %d -> %d, %d\n", clip->x0, clip->y0, clip->x1, clip->y1);
    }
    else
    {
        printf("XXX: frontier_plotter_clip: none\n");
    }
#endif

    return NSERROR_OK;
}

nserror frontier_plotter_arc(
    const struct redraw_context *ctx,
    const plot_style_t *pstyle,
    int x,
    int y,
    int radius,
    int angle1,
    int angle2)
{
    printf("XXX: frontier_plotter_arc\n");

    if (angle2 < angle1)
    {
        angle2 += 360;
    }

    float angle1_r = (float)(angle1) * (M_PI / 180.0);
    float angle2_r = (float)(angle2) * (M_PI / 180.0);
    float angle, b, c;
    float step = 0.1;

    int x0 = x;
    int y0 = y;

    b = angle1_r;
    c = angle2_r;

    int x1 = (int)(cos(b) * (float)radius);
    int y1 = (int)(sin(b) * (float)radius);

    int prevX = x0 + x1;
    int prevY = y0 + y1;

    PlotterContext* plotterCtx = (PlotterContext*)(ctx->priv);
    for(angle = (b + step); angle <= c; angle += step)
    {
        x1 = (int)(cos(angle) * (float)radius);
        y1 = (int)(sin(angle) * (float)radius);

        int dx = x0 + x1;
        int dy = y0 + y1;

        plotterCtx->surface->drawLine(
            prevX, prevY,
            dx, dy,
            RGB2BGR(pstyle->stroke_colour));
        prevX = dx;
        prevY = dy;
    }

    return NSERROR_OK;
}

nserror frontier_plotter_disc(
    const struct redraw_context *ctx,
    const plot_style_t *pstyle,
    int x,
    int y,
    int radius)
{
    printf("XXX: frontier_plotter_disc\n");
    PlotterContext* plotterCtx = (PlotterContext*)(ctx->priv);
    plotterCtx->surface->drawCircle(x, y, radius, RGB2BGR(pstyle->stroke_colour));

    return NSERROR_OK;
}

nserror frontier_plotter_line(
    const struct redraw_context *ctx,
    const plot_style_t *pstyle,
    const struct rect *line)
{
    printf("XXX: frontier_plotter_line\n");

    PlotterContext* plotterCtx = (PlotterContext*)(ctx->priv);
    plotterCtx->surface->drawLine(
        line->x0, line->y0,
        line->x1, line->y1,
        RGB2BGR(pstyle->stroke_colour));

    return NSERROR_OK;
}

nserror frontier_plotter_rectangle(
    const struct redraw_context *ctx,
    const plot_style_t *pstyle,
    const struct rect *rectangle)
{
#if 0
    printf("XXX: frontier_plotter_rectangle: %d, %d -> %d, %d\n", rectangle->x0, rectangle->y0, rectangle->x1, rectangle->y1);
#endif

    PlotterContext* plotterCtx = (PlotterContext*)(ctx->priv);
    int w = (rectangle->x1 - rectangle->x0) + 1;
    int h = (rectangle->y1 - rectangle->y0) + 1;

    switch (pstyle->fill_type)
    {
        case PLOT_OP_TYPE_SOLID:
            plotterCtx->surface->drawRectFilled(rectangle->x0, rectangle->y0, w, h, RGB2BGR(pstyle->fill_colour));
            break;
        default:
            printf("XXX: frontier_plotter_rectangle:  -> Unhandled fill_type: %d\n", pstyle->fill_type);
            break;
    }

    return NSERROR_OK;
}

nserror frontier_plotter_polygon(
    const struct redraw_context *ctx,
    const plot_style_t *pstyle,
    const int *p,
    unsigned int n)
{
    printf("XXX: frontier_plotter_polygon\n");

    return NSERROR_OK;
}

struct Vector
{
    float x;
    float y;
};

static void frontier_bezier(Vector& a, Vector& b, Vector& c, Vector& d, double t, Vector& p)
{
    p.x = pow((1 - t), 3) * a.x + 3 * t * pow((1 -t), 2) * b.x + 3 * (1-t) * pow(t, 2)* c.x + pow (t, 3)* d.x;
    p.y = pow((1 - t), 3) * a.y + 3 * t * pow((1 -t), 2) * b.y + 3 * (1-t) * pow(t, 2)* c.y + pow (t, 3)* d.y;
}

nserror frontier_plotter_path(
    const struct redraw_context *ctx,
    const plot_style_t *pstyle,
    const float *p,
    unsigned int n,
    const float transform[6])
{
    PlotterContext* plotterCtx = (PlotterContext*)(ctx->priv);
    printf("XXX: frontier_plotter_path: n=%u\n", n);

    unsigned int i;
    Vector current;
    Vector start;
    for (i = 0; i < n;)
    {
        int command = p[i];
        switch (command)
        {
            case PLOTTER_PATH_MOVE:
                current.x = p[i + 1];
                current.y = p[i + 2];
                start = current;

                i += 3;
                break;

            case PLOTTER_PATH_CLOSE:
                plotterCtx->surface->drawLine(current.x, current.y, start.x, start.y, RGB2BGR(pstyle->stroke_colour));
                i++;
                break;

            case PLOTTER_PATH_LINE:
            {
                Vector position;
                position.x = p[i + 1];
                position.y = p[i + 2];
                plotterCtx->surface->drawLine(current.x, current.y, position.x, position.y, RGB2BGR(pstyle->stroke_colour));
                i+= 3;
                current = position;
            } break;

            case PLOTTER_PATH_BEZIER:
            {
                Vector a;
                Vector b;
                Vector c;
                Vector r;

                a.x = p[i + 1];
                a.y = p[i + 2];
                b.x = p[i + 3];
                b.y = p[i + 4];
                c.x = p[i + 5];
                c.y = p[i + 6];

                plotterCtx->surface->drawLine(current.x, current.y, c.x, c.y, RGB2BGR(pstyle->stroke_colour));
                double t;
                for (t = 0.0; t <= 1.0; t += 0.1)
                {
                    frontier_bezier(current, a, b, c, t, r);
/*
                    if (pstyle->fill_colour != NS_TRANSPARENT) {
                                        if (AreaDraw(glob->rp, p_r.x, p_r.y) == -1) {
                                                NSLOG(netsurf, INFO,
                                                      "AreaDraw: vector list full");
                                        }
                                } else {
                                        Draw(glob->rp, p_r.x, p_r.y);
                                }
                        }
*/
                    plotterCtx->surface->drawLine(current.x, current.y, r.x, r.y, RGB2BGR(pstyle->stroke_colour));
                    current = r;
                }

                i += 7;
            } break;

            default:
                printf("XXX: frontier_plotter_path: ERROR: Unhandled command: %d\n", command);
                break;
        }
    }

    return NSERROR_OK;
}

nserror frontier_plotter_bitmap(
    const struct redraw_context *ctx,
    struct bitmap *bitmap,
    int x,
    int y,
    int width,
    int height,
    colour bg,
    bitmap_flags_t flags)
{
    PlotterContext* plotterCtx = (PlotterContext*)(ctx->priv);

#if 0
    printf("XXX: frontier_plotter_bitmap: %p\n", bitmap);
#endif

    Surface* surface = (Surface*)bitmap;

    Surface* swappedSurface = new HighDPISurface(surface->getWidth(), surface->getHeight(), 4);
    //Surface* swappedSurface = new Surface(surface->getWidth(), surface->getHeight(), 4);

    unsigned int sy;
    for (sy = 0; sy < surface->getHeight(); sy++)
    {
        unsigned int sx;
        for (sx = 0; sx < surface->getWidth(); sx++)
        {
            swappedSurface->drawPixel(sx, sy, RGB2BGR(surface->getPixel(sx, sy)));
        }
    }

    plotterCtx->surface->blit(x, y, swappedSurface, 0, 0, width * 2, height * 2);
    delete swappedSurface;

    return NSERROR_OK;
}

nserror frontier_plotter_text(
    const struct redraw_context *ctx,
    const plot_font_style_t *fstyle,
    int x,
    int y,
    const char *text,
    size_t length)
{
    if (length == 0)
    {
        return NSERROR_OK;
    }

    wstring wstr = Geek::Core::utf82wstring(text, length);

    PlotterContext* plotterCtx = (PlotterContext*)(ctx->priv);
    //printf("XXX: frontier_plotter_text: %d, %d, text=%ls\n", x, y, wstr.c_str());

    FontHandle* font = plotterCtx->app->createFontHandle(fstyle);
    y -= font->getPixelHeight(72);

    Surface* surface = plotterCtx->surface;
    uint32_t c = RGB2BGR(fstyle->foreground);// & 0xffffff;

    if (x > 0 && x < (int)surface->getWidth() && y > 0 && y < (int)surface->getHeight())
    {
        plotterCtx->app->getFontManager()->write(font, surface, x, y, wstr, c, true, NULL);
    }

    return NSERROR_OK;
}



const struct plotter_table frontier_plotter_table =
{
        .rectangle = frontier_plotter_rectangle,
        .line = frontier_plotter_line,
        .polygon = frontier_plotter_polygon,
        .clip = frontier_plotter_clip,
        .text = frontier_plotter_text,
        .disc = frontier_plotter_disc,
        .arc = frontier_plotter_arc,
        .bitmap = frontier_plotter_bitmap,
        .path = frontier_plotter_path,
        //.option_knockout = true
};


