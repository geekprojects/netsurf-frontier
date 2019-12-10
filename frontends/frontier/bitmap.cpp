
#include "bitmap.h"
#include "plotters.h"

extern "C" {
#include "utils/utils.h"
}

#include <geek/gfx-surface.h>

using namespace Geek;
using namespace Geek::Gfx;



void* frontier_bitmap_create(int width, int height, unsigned int state)
{
#if 0
    printf("XXX: frontier_bitmap_create: width=%d, height=%d, state=%d\n", width, height, state);
#endif
    Geek::Gfx::Surface* surface = new Geek::Gfx::Surface(width, height, 4);
#if 0
    printf("XXX: frontier_bitmap_create:  -> surface=%p\n", surface);
#endif

    return surface;
}

void frontier_bitmap_destroy(void *bitmap)
{
    Geek::Gfx::Surface* surface = (Geek::Gfx::Surface*)bitmap;
#if 0
    printf("XXX: frontier_bitmap_destroy\n");
#endif
    delete surface;
}

void frontier_bitmap_set_opaque(void *bitmap, bool opaque)
{
#if 0
    printf("XXX: frontier_bitmap_set_opaque\n");
#endif
}

bool frontier_bitmap_get_opaque(void *bitmap)
{
#if 0
    printf("XXX: frontier_bitmap_get_opaque\n");
#endif
    return false;
}

bool frontier_bitmap_test_opaque(void *bitmap)
{
#if 0
    printf("XXX: frontier_bitmap_test_opaque\n");
#endif
    return false;
}

unsigned char * frontier_bitmap_get_buffer(void *bitmap)
{
#if 0
    printf("XXX: frontier_bitmap_get_buffer\n");
#endif
    Geek::Gfx::Surface* surface = (Geek::Gfx::Surface*)bitmap;
    return surface->getDrawingBuffer();
}

size_t frontier_bitmap_get_rowstride(void *bitmap)
{
    Geek::Gfx::Surface* surface = (Geek::Gfx::Surface*)bitmap;
#if 0
    printf("XXX: frontier_bitmap_get_rowstride\n");
#endif
    return surface->getStride();
}

int frontier_bitmap_get_width(void *bitmap)
{
    Geek::Gfx::Surface* surface = (Geek::Gfx::Surface*)bitmap;
#if 0
    printf("XXX: frontier_bitmap_get_width\n");
#endif
    return surface->getWidth();
}

int frontier_bitmap_get_height(void *bitmap)
{
    Geek::Gfx::Surface* surface = (Geek::Gfx::Surface*)bitmap;
#if 0
    printf("XXX: frontier_bitmap_get_height\n");
#endif
    return surface->getHeight();
}

size_t frontier_bitmap_get_bpp(void *bitmap)
{
#if 0
    printf("XXX: frontier_bitmap_get_bpp\n");
#endif
    return 4;
}

bool frontier_bitmap_save(void *bitmap, const char *path, unsigned flags)
{
    printf("XXX: frontier_bitmap_save: path=%s\n", path);
    return false;
}

void frontier_bitmap_modified(void *bitmap)
{
#if 0
    printf("XXX: frontier_bitmap_modified\n");
#endif
}

nserror frontier_bitmap_render(struct bitmap *bitmap, struct hlcache_handle *content)
{
#if 0
    printf("XXX: frontier_bitmap_render\n");
#endif
    Geek::Gfx::Surface* surface = (Geek::Gfx::Surface*)bitmap;

    PlotterContext plotterContext;
    plotterContext.surface = surface;
    plotterContext.app = g_frontierApp;

    struct redraw_context ctx =
    {
        .interactive = true,
        .background_images = true,
        .plot = &frontier_plotter_table,
        .priv = &plotterContext
    };

    int surfaceWidth = surface->getWidth();
    int surfaceHeight = surface->getHeight();

    int cwidth = content_get_width(content);
    if (cwidth < surfaceWidth)
    {
        cwidth = surfaceWidth;
    }
    if (cwidth > 1024)
    {
        cwidth = 1024;
    }

    /* The height is set in proportion with the width, according to the
    * aspect ratio of the required thumbnail. */
    int cheight = ((cwidth * surfaceHeight) + (surfaceWidth / 2)) / surfaceHeight;

    content_scaled_redraw(content, cwidth, cheight, &ctx);

    return NSERROR_OK;
}

// Required
struct gui_bitmap_table g_frontier_bitmap_table =
{
    .create = frontier_bitmap_create,
    .destroy = frontier_bitmap_destroy,
    .set_opaque = frontier_bitmap_set_opaque,
    .get_opaque = frontier_bitmap_get_opaque,
    .test_opaque = frontier_bitmap_test_opaque,
    .get_buffer = frontier_bitmap_get_buffer,
    .get_rowstride = frontier_bitmap_get_rowstride,
    .get_width = frontier_bitmap_get_width,
    .get_height = frontier_bitmap_get_height,
    .get_bpp = frontier_bitmap_get_bpp,
    .save = frontier_bitmap_save,
    .modified = frontier_bitmap_modified,
    .render = frontier_bitmap_render,
};

