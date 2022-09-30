
#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <glib.h>

#define CONFIG_ERROR config_error_quark()
#define ERROR_CONFIG_NOT_SET 1
#define ERROR_CONFIG_TOO_BIG 2
#define ERROR_CONFIG_TOO_SMALL 3
#define ERROR_CONFIG_INVALID 4
#define ERROR_CONFIG_NO_FILE 5


enum config_main_deep_enumtest {
    MAIN_DEEP_ENUMTEST_HELLO,
    MAIN_DEEP_ENUMTEST_GOODBYE,
};



struct deep {
    gchar *param; /**  */
    enum config_main_deep_enumtest enumtest; /**  */
    gchar *params; /**  */
};

struct main {
    gint64 second; /**  */
    gboolean third; /**  */
    gdouble double_param; /**  */
    gint64 size; /**  */
    struct deep deep; /**  */
    gchar *first; /** This is a variable */
};

struct config {
    gchar *other; /**  */
    struct main main; /**  */
};


gboolean
config_parse(struct config *cfg, gint argc, gchar *argv[], gboolean die_on_json_error, GError **err);

void
config_clear(struct config *cfg);

gchar *
config_to_string(struct config *cfg);


const gchar *
config_name_enum_main_deep_enumtest(enum config_main_deep_enumtest val);


GQuark
config_error_quark(void);
#endif /* _CONFIG_H_ */


