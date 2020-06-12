
#include "gui.h"
#include "misc.h"

nserror frontier_schedule(int t, void (*callback)(void *p), void *p)
{
    g_frontierApp->schedule(t, callback, p);

    return NSERROR_OK;
}

nserror frontier_warning(const char *message, const char *detail)
{
    printf("XXX: frontier_warning: %s (%s)\n", message, detail);
    Frontier::Utils::stacktrace();

    return NSERROR_NOT_IMPLEMENTED;
}

struct gui_misc_table g_frontier_misc_table =
{
    frontier_schedule,
    //frontier_warning,
    NULL, //gui_quit,
    NULL, //gui_launch_url,
    NULL, //cert_verify
    NULL, //gui_401login_open,
    NULL, // pdf_password (if we have Haru support)
};

