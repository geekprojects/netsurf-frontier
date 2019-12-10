
#include "layout.h"
#include "gui.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <geek/core-string.h>

using namespace std;
using namespace Frontier;
using namespace Geek;
using namespace Geek::Core;

nserror frontier_layout_width(const struct plot_font_style *fstyle, const char *string, size_t length, int *width)
{
    if (string == NULL)
    {
        *width = 0;
        return NSERROR_OK;
    }

    FontHandle* font = g_frontierApp->createFontHandle(fstyle);

    wstring wstr = Geek::Core::utf82wstring(string, length);
    *width = g_frontierApp->getFontManager()->width(font, wstr);

    return NSERROR_OK;
}

nserror frontier_layout_position(const struct plot_font_style *fstyle, const char* text, size_t length, int x, size_t *char_offset, int *actual_x)
{
    printf("XXX: frontier_layout_position\n");
    FontHandle* font = g_frontierApp->createFontHandle(fstyle);

    string str = string(text, length);
    wstring wstr = Frontier::Utils::string2wstring(str);

    FontManager* fm = g_frontierApp->getFontManager();

    size_t pos = 1;
    int width = 0;
    for (pos = 1; pos < length; pos++)
    {
        wstring currentStr = wstr.substr(0, pos);
        int currentWidth = fm->width(font, currentStr);
#if 0
        printf("XXX: frontier_layout_position: pos=%zu, currentWidth=%d: %ls\n", pos, currentWidth, currentStr.c_str());
#endif

        if (currentWidth > x)
        {
            pos--;
            break;
        }

        width = currentWidth;
    }

    *char_offset = pos;
    *actual_x = width;

    return NSERROR_OK;
}

nserror frontier_layout_split(const struct plot_font_style *fstyle, const char* text, size_t length, int x, size_t *char_offset, int *actual_x)
{
    wstring wstr = Geek::Core::utf82wstring(text, length);

#if 0
    printf("XXX: frontier_layout_split: length=%zu, x=%d: %ls\n", length, x, wstr.c_str());
#endif

    FontManager* fm = g_frontierApp->getFontManager();
    FontHandle* font = g_frontierApp->createFontHandle(fstyle);

    size_t pos = 1;
    int currentX = 0;
    int lastSpacePos =  0;
    int lastSpaceX =  0;
    for (pos = 0; pos < wstr.length(); pos++)
    {
        if (iswspace(wstr[pos]))
        {
            lastSpaceX = currentX;
            lastSpacePos = pos;
        }

        if (currentX >= x)
        {
            if (lastSpacePos != 0)
            {
                *actual_x = lastSpaceX;
                *char_offset = lastSpacePos;
            }
            else
            {
                *actual_x = currentX;
                *char_offset = pos - 1;
            }

            return NSERROR_OK;
        }

        wstring currentStr = wstr.substr(0, pos);
        currentX = fm->width(font, currentStr);
    }

    *char_offset = pos;
    *actual_x = currentX;

    return NSERROR_OK;
}

// Required
struct gui_layout_table g_frontier_layout_table =
{
    .width = frontier_layout_width,
    .position = frontier_layout_position,
    .split = frontier_layout_split
};

