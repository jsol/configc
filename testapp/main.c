#include <glib.h>

#include "config.h"

int
main(int argc, char** argv)
{
    struct config cfg = {};
    GError *err = NULL;

    gboolean ok;
    ok = config_parse(&cfg, argc, argv, FALSE, &err);

    if (!ok) {
        g_print("Failed to parse config: %s\n", err->message);
        g_clear_error(&err);
    } else {
        g_print("Config OK\n");
        gchar *str = config_to_string(&cfg);
        g_print("Config: %s", str);
        g_free(str);
    }
    config_clear(&cfg);
}