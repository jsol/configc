#include <glib.h>
#include <errno.h>
#include <jansson.h>
#include <math.h>

#include "config.h"

static void
set_candidate(GHashTable *candidates, gchar *name, gchar *value)
{
  g_assert(candidates);
  g_assert(name);

  if (value == NULL) {
    return;
  }

  (void) g_hash_table_replace(candidates, (gpointer)name, (gpointer)value);
}


static void
set_env_var(GHashTable *candidates, const gchar *name, const gchar *env)
{
  const gchar *tmp;

  g_assert(candidates);
  g_assert(name);
  g_assert(env);

  tmp = g_getenv(env);
  set_candidate(candidates, (gchar *)name, g_strdup(tmp));
}

static gboolean
set_env(GHashTable *candidates)
{
  g_assert(candidates);

  set_env_var(candidates, "main.second", "SECOND_VAR");

  set_env_var(candidates, "main.deep.enumtest", "enum-test");

  return TRUE;
}



static gboolean
set_defaults(GHashTable *candidates)
{
  g_assert(candidates);
  set_candidate(candidates, "main.first", g_strdup("Just a string"));
  set_candidate(candidates, "main.second", g_strdup("7"));
  set_candidate(candidates, "main.third", g_strdup("FALSE"));
  set_candidate(candidates, "main.double_param", g_strdup("13.5"));
  set_candidate(candidates, "main.size", g_strdup("10 mb"));
  set_candidate(candidates, "main.deep.param", g_strdup("hello"));
  set_candidate(candidates, "main.deep.enumtest", g_strdup("hello"));
  set_candidate(candidates, "main.deep.params", g_strdup("Just a string"));
  set_candidate(candidates, "other", g_strdup("Just a string"));

  return TRUE;
}

static gboolean
parse_opts(GHashTable *candidates, gint *argc, gchar **argv[], GError **err)
{
  GOptionContext *context;
  gchar *main_first = NULL;
  gchar *main_second = NULL;
  gboolean main_third = FALSE;
  gchar *main_double_param = NULL;
  gchar *main_size = NULL;
  gchar *main_deep_enumtest = NULL;
  GOptionEntry entries[] =
  {
    { "first", 'f', 0 , G_OPTION_ARG_STRING, &main_first, "This is a variable", NULL },
    { "second", 'e', 0 , G_OPTION_ARG_STRING, &main_second, "", NULL },
    { "third", 't', 0 , G_OPTION_ARG_NONE, &main_third, "", NULL },
    { "test-double", 'd', 0 , G_OPTION_ARG_STRING, &main_double_param, "", NULL },
    { "test-size", 's', 0 , G_OPTION_ARG_STRING, &main_size, "", NULL },
    { "enum-test", 'g', 0 , G_OPTION_ARG_STRING, &main_deep_enumtest, "", NULL },
    { NULL }
  };

  g_assert(candidates);
  g_assert(err != NULL && *err == NULL);

  context = g_option_context_new ("");
  g_option_context_add_main_entries (context, entries, NULL);
  if (!g_option_context_parse (context, argc, argv, err)) {
    return FALSE;
  }
  set_candidate(candidates, "main.first", main_first);
  set_candidate(candidates, "main.second", main_second);
  if (main_third) {
    set_candidate(candidates, "main.third", g_strdup("TRUE"));
  }
  set_candidate(candidates, "main.double_param", main_double_param);
  set_candidate(candidates, "main.size", main_size);
  set_candidate(candidates, "main.deep.enumtest", main_deep_enumtest);

  return TRUE;
}
static gchar *
get_json_config_file()
{
  return g_strdup("./config.json");
}

static void
add_json_string_to_candidates(GHashTable *candidates, gchar *name, json_t *parent, gchar *json_name)
{
  json_t *val = NULL;

  val = json_object_get(parent, json_name);
  if (val && json_is_string(val)) {
    set_candidate(candidates, name, g_strdup(json_string_value(val)));
  }
}

static void
add_json_size_to_candidates(GHashTable *candidates, gchar *name, json_t *parent, gchar *json_name)
{
  json_t *val = NULL;

  val = json_object_get(parent, json_name);
  if (val && json_is_string(val)) {
    set_candidate(candidates, name, g_strdup(json_string_value(val)));
  }
}

static void
add_json_int_to_candidates(GHashTable *candidates, gchar *name, json_t *parent, gchar *json_name)
{
  json_t *val = NULL;

  val = json_object_get(parent, json_name);
  if (val && json_is_string(val)) {
    set_candidate(candidates, name, g_strdup(json_string_value(val)));
  }
  if (val && json_is_integer(val)) {
    set_candidate(candidates, name, g_strdup_printf("%lld", json_integer_value(val)));
  }
}

static void
add_json_double_to_candidates(GHashTable *candidates, gchar *name, json_t *parent, gchar *json_name)
{
  json_t *val = NULL;

  val = json_object_get(parent, json_name);
  if (val && json_is_string(val)) {
    set_candidate(candidates, name, g_strdup(json_string_value(val)));
  }
  if (val && json_is_number(val)) {
    set_candidate(candidates, name, g_strdup_printf("%lf", json_number_value(val)));
  }
}

static void
add_json_enum_to_candidates(GHashTable *candidates, gchar *name, json_t *parent, gchar *json_name)
{
  json_t *val = NULL;

  val = json_object_get(parent, json_name);
  if (val && json_is_string(val)) {
    set_candidate(candidates, name, g_strdup(json_string_value(val)));
  }
}

static void
add_json_boolean_to_candidates(GHashTable *candidates, gchar *name, json_t *parent, gchar *json_name)
{
  json_t *val = NULL;
  gchar *str = NULL;

  val = json_object_get(parent, json_name);
  if (val && json_is_boolean(val)) {
    if (json_is_true(val)) {
      str = g_strdup("TRUE");
    } else {
      str = g_strdup("FALSE");
    }
    set_candidate(candidates, name, str);
  }
}

static gboolean
parse_json(GHashTable *candidates, GError **err)
{
  gchar *file = NULL;
  json_error_t j_error;
  json_set_alloc_funcs(g_malloc, g_free);
  json_t *root = NULL;
  json_t *root_main = NULL;
  json_t *root_main_deep = NULL;

  file = get_json_config_file();
  root = json_load_file(file, 0, &j_error);
  g_free(file);

  if (root == NULL) {
    g_set_error(err,
                CONFIG_ERROR,
                ERROR_CONFIG_NO_FILE,
                "Could not parse config file %s, error: %s",
                file,
                g_strdup(j_error.text));
    return FALSE;
  }
  if (root != NULL) {
    root_main = json_object_get(root, "main");
  }
  if (root_main != NULL) {
    root_main_deep = json_object_get(root_main, "deep");
  }
  if (root_main != NULL) {
    
        add_json_boolean_to_candidates(candidates, "main.third", root_main, "third");
        add_json_double_to_candidates(candidates, "main.double_param", root_main, "double");
        add_json_size_to_candidates(candidates, "main.size", root_main, "size");
        add_json_string_to_candidates(candidates, "main.first", root_main, "first");
        add_json_int_to_candidates(candidates, "main.second", root_main, "second");
  }
  if (root_main_deep != NULL) {
    
        add_json_enum_to_candidates(candidates, "main.deep.enumtest", root_main_deep, "param_enum");
        add_json_string_to_candidates(candidates, "main.deep.params", root_main_deep, "params");
        add_json_string_to_candidates(candidates, "main.deep.param", root_main_deep, "param");
  }

  json_decref(root);
  return TRUE;
}

static gboolean
set_string(const gchar *name, const gchar *str, gchar **dst, gint64 min, gint64 max, gint options, const gchar* opts[], GError **err)
{
  g_assert(name);
  g_assert(str);
  g_assert(dst);
  g_assert(err != NULL && *err == NULL);

  if (str == NULL) {
      g_set_error(err,
                  CONFIG_ERROR,
                  ERROR_CONFIG_NOT_SET,
                  "Parameter %s not set",
                  name);
    return FALSE;
  }

  if (strlen(str) < min) {
    g_set_error(err,
                  CONFIG_ERROR,
                  ERROR_CONFIG_TOO_SMALL,
                  "Parameter %s too short (min %ld chars)",
                  name, min);
    return FALSE;
  }

  if (strlen(str) > max) {
      g_set_error(err,
                  CONFIG_ERROR,
                  ERROR_CONFIG_TOO_BIG,
                  "Parameter %s too long (max %ld chars)",
                  name, max);
    return FALSE;
  }

  if (options > 0) {
    gboolean ok = FALSE;
    for (gint i = 0; i < options; i++) {
        if (g_strcmp0(opts[i], str) == 0) {
            ok = TRUE;
            break;
        }
    }
    if (!ok) {
      g_set_error(err,
              CONFIG_ERROR,
              ERROR_CONFIG_INVALID,
              "Parameter %s has an invalid value",
              name);
      return FALSE;
    }
  }

  *dst = g_strdup(str);

  return TRUE;
}

static gboolean
set_int(const gchar *name, const gchar *str, gint64 *dst, gint64 min, gint64 max, gint options, const gint64 *opts, GError **err)
{
  gint64 tmp;
  gchar *endp;

  g_assert(name);
  g_assert(str);
  g_assert(dst);
  g_assert(err != NULL && *err == NULL);

  if (str == NULL) {
      g_set_error(err,
                  CONFIG_ERROR,
                  ERROR_CONFIG_NOT_SET,
                  "Parameter %s not set",
                  name);
    return FALSE;
  }

  tmp = g_ascii_strtoll(str, &endp, 10);

  if (tmp == 0 && endp == str) {
    g_set_error(err,
                CONFIG_ERROR,
                ERROR_CONFIG_INVALID,
                "Parameter %s has an invalid value",
                name);
    return FALSE;
  }

  if (tmp == G_MAXINT64 && errno == ERANGE) {
    g_set_error(err,
                CONFIG_ERROR,
                ERROR_CONFIG_TOO_BIG,
                "Parameter %s too big (max %ld)",
                name, max);
    return FALSE;
  }

  if (tmp == G_MININT64 && errno == ERANGE) {
    g_set_error(err,
                CONFIG_ERROR,
                ERROR_CONFIG_TOO_SMALL,
                "Parameter %s too small (min %ld)",
                name, min);
    return FALSE;
  }

  if (tmp < min) {
    g_set_error(err,
                CONFIG_ERROR,
                ERROR_CONFIG_TOO_SMALL,
                "Parameter %s too small (min %ld)",
                 name, min);
    return FALSE;
  }

  if (tmp > max) {
      g_set_error(err,
                  CONFIG_ERROR,
                  ERROR_CONFIG_TOO_BIG,
                  "Parameter %s too big (max %ld)",
                  name, max);
    return FALSE;
  }

  if (options > 0) {
    gboolean ok = FALSE;
    for (gint i = 0; i < options; i++) {
        if (opts[i] == tmp) {
            ok = TRUE;
            break;
        }
    }
    if (!ok) {
      g_set_error(err,
              CONFIG_ERROR,
              ERROR_CONFIG_INVALID,
              "Parameter %s has an invalid value",
              name);
      return FALSE;
    }
  }

  *dst = tmp;

  return TRUE;
}

static gboolean
set_size(const gchar *name, const gchar *str, gint64 *dst, gint64 min, gint64 max, gint options, const gint64 *opts, GError **err)
{
  gint64 tmp;
  gchar *endp;

  g_assert(name);
  g_assert(str);
  g_assert(dst);
  g_assert(err != NULL && *err == NULL);

  if (str == NULL) {
      g_set_error(err,
                  CONFIG_ERROR,
                  ERROR_CONFIG_NOT_SET,
                  "Parameter %s not set",
                  name);
    return FALSE;
  }

  tmp = g_ascii_strtoll(str, &endp, 10);

  if (tmp == 0 && endp == str) {
    g_set_error(err,
                CONFIG_ERROR,
                ERROR_CONFIG_INVALID,
                "Parameter %s has an invalid value",
                name);
    return FALSE;
  }

  if (tmp == G_MAXINT64 && errno == ERANGE) {
    g_set_error(err,
                CONFIG_ERROR,
                ERROR_CONFIG_TOO_BIG,
                "Parameter %s too big (max %ld)",
                name, max);
    return FALSE;
  }

  if (tmp == G_MININT64 && errno == ERANGE) {
    g_set_error(err,
                CONFIG_ERROR,
                ERROR_CONFIG_TOO_SMALL,
                "Parameter %s too small (min %ld)",
                name, min);
    return FALSE;
  }

  if (g_str_has_suffix(str, "kb") || g_str_has_suffix(str, "KB") ) {
    tmp = tmp * 1024;
  }

  if (g_str_has_suffix(str, "mb") || g_str_has_suffix(str, "MB") ) {
    tmp = tmp * 1024 * 1024;
  }

  if (g_str_has_suffix(str, "gb") || g_str_has_suffix(str, "GB") ) {
    tmp = tmp * 1024 * 1024 * 1024;
  }

  if (tmp < min) {
    g_set_error(err,
                CONFIG_ERROR,
                ERROR_CONFIG_TOO_SMALL,
                "Parameter %s too small (min %ld)",
                 name, min);
    return FALSE;
  }

  if (tmp > max) {
      g_set_error(err,
                  CONFIG_ERROR,
                  ERROR_CONFIG_TOO_BIG,
                  "Parameter %s too big (max %ld)",
                  name, max);
    return FALSE;
  }

  if (options > 0) {
    gboolean ok = FALSE;
    for (gint i = 0; i < options; i++) {
        if (opts[i] == tmp) {
            ok = TRUE;
            break;
        }
    }
    if (!ok) {
      g_set_error(err,
              CONFIG_ERROR,
              ERROR_CONFIG_INVALID,
              "Parameter %s has an invalid value",
              name);
      return FALSE;
    }
  }

  *dst = tmp;

  return TRUE;
}

static gboolean
set_double(const gchar *name, const gchar *str, gdouble *dst, gdouble min, gdouble max, gint options, const gdouble *opts, GError **err)
{
  gdouble tmp;
  gchar *endp;

  g_assert(name);
  g_assert(str);
  g_assert(dst);
  g_assert(err != NULL && *err == NULL);

  if (str == NULL) {
      g_set_error(err,
                  CONFIG_ERROR,
                  ERROR_CONFIG_NOT_SET,
                  "Parameter %s not set",
                  name);
    return FALSE;
  }

  tmp = g_ascii_strtod(str, &endp);

  if (tmp == 0.0 && endp == str) {
    g_set_error(err,
                CONFIG_ERROR,
                ERROR_CONFIG_INVALID,
                "Parameter %s has an invalid value",
                name);
    return FALSE;
  }

  if (tmp == HUGE_VAL && errno == ERANGE) {
    g_set_error(err,
                CONFIG_ERROR,
                ERROR_CONFIG_TOO_BIG,
                "Parameter %s too big (max %lf)",
                name, max);
    return FALSE;
  }

  if (tmp == 0 && errno == ERANGE) {
    g_set_error(err,
                CONFIG_ERROR,
                ERROR_CONFIG_TOO_SMALL,
                "Parameter %s too small (min %lf)",
                name, min);
    return FALSE;
  }

  if (tmp < min) {
    g_set_error(err,
                CONFIG_ERROR,
                ERROR_CONFIG_TOO_SMALL,
                "Parameter %s too small (min %lf)",
                 name, min);
    return FALSE;
  }

  if (tmp > max) {
      g_set_error(err,
                  CONFIG_ERROR,
                  ERROR_CONFIG_TOO_BIG,
                  "Parameter %s too big (max %lf)",
                  name, max);
    return FALSE;
  }

  if (options > 0) {
    gboolean ok = FALSE;
    for (gint i = 0; i < options; i++) {
        if (opts[i] == tmp) {
            ok = TRUE;
            break;
        }
    }
    if (!ok) {
      g_set_error(err,
              CONFIG_ERROR,
              ERROR_CONFIG_INVALID,
              "Parameter %s has an invalid value",
              name);
      return FALSE;
    }
  }

  *dst = tmp;

  return TRUE;
}

static gboolean
set_boolean(const gchar *name, const gchar *str, gboolean *dst, GError **err)
{
  g_assert(name);
  g_assert(str);
  g_assert(dst);
  g_assert(err != NULL && *err == NULL);

  if (str == NULL) {
      g_set_error(err,
                  CONFIG_ERROR,
                  ERROR_CONFIG_NOT_SET,
                  "Parameter %s not set",
                  name);
    return FALSE;
  }

  if (g_strcmp0(str, "TRUE") == 0) {
    *dst = TRUE;
    return TRUE;
  }
  if (g_strcmp0(str, "FALSE") == 0) {
    *dst = FALSE;
    return TRUE;
  }

  g_set_error(err,
              CONFIG_ERROR,
              ERROR_CONFIG_INVALID,
              "Parameter %s has an invalid value",
              name);

  return FALSE;
}


static gboolean
set_enum_main_deep_enumtest(const gchar *name, const gchar *str, enum config_main_deep_enumtest *dst, GError **err)
{
  g_assert(name);
  g_assert(str);
  g_assert(dst);
  g_assert(err != NULL && *err == NULL);

  if (str == NULL) {
      g_set_error(err,
                  CONFIG_ERROR,
                  ERROR_CONFIG_NOT_SET,
                  "Parameter %s not set",
                  name);
    return FALSE;
  }
  if (g_strcmp0(str, "hello") == 0) {
    *dst = MAIN_DEEP_ENUMTEST_HELLO;
    return TRUE;
  }
  if (g_strcmp0(str, "goodbye") == 0) {
    *dst = MAIN_DEEP_ENUMTEST_GOODBYE;
    return TRUE;
  }

  g_set_error(err,
              CONFIG_ERROR,
              ERROR_CONFIG_INVALID,
              "Parameter %s has an invalid value",
              name);

  return FALSE;
}


static gboolean
check_and_set(struct config *cfg, GHashTable *candidates, GError **err)
{
    const gchar *valid_main_deep_param[] = { "hello", "goodbye"};

  g_assert(cfg);
  g_assert(candidates);
  g_assert(err != NULL && *err == NULL);
    if (!set_string("main.first", g_hash_table_lookup(candidates, "main.first"), &cfg->main.first, 1, 10, 0, NULL, err)) {
        return FALSE;
    }
    if (!set_int("main.second", g_hash_table_lookup(candidates, "main.second"), &cfg->main.second, 1, 10, 0, NULL, err)) {
        return FALSE;
    }
    if (!set_boolean("main.third", g_hash_table_lookup(candidates, "main.third"), &cfg->main.third, err)) {
        return FALSE;
    }
    if (!set_double("main.double_param", g_hash_table_lookup(candidates, "main.double_param"), &cfg->main.double_param, -10, 100, 0, NULL, err)) {
        return FALSE;
    }
    if (!set_size("main.size", g_hash_table_lookup(candidates, "main.size"), &cfg->main.size, 0, 10000000, 0, NULL, err)) {
        return FALSE;
    }
    if (!set_string("main.deep.param", g_hash_table_lookup(candidates, "main.deep.param"), &cfg->main.deep.param, 1, 24, 2, valid_main_deep_param, err)) {
        return FALSE;
    }
    if (!set_enum_main_deep_enumtest("main.deep.enumtest", g_hash_table_lookup(candidates, "main.deep.enumtest"), &cfg->main.deep.enumtest, err)) {
        return FALSE;
    }
    if (!set_string("main.deep.params", g_hash_table_lookup(candidates, "main.deep.params"), &cfg->main.deep.params, 1, 24, 0, NULL, err)) {
        return FALSE;
    }
    if (!set_string("other", g_hash_table_lookup(candidates, "other"), &cfg->other, 1, 24, 0, NULL, err)) {
        return FALSE;
    }

  return TRUE;
}

gboolean
config_parse(struct config *cfg, gint argc, gchar *argv[], gboolean die_on_json_error, GError **err) {
  GHashTable * candidates = NULL;

  candidates = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, g_free);

  set_defaults(candidates);



  if (die_on_json_error) {
    if (!parse_json(candidates, err)) {
        goto err;
    }
  } else {
    (void)parse_json(candidates, NULL);
  }


  set_env(candidates);


  if (!parse_opts(candidates, &argc, &argv, err)) {
    goto err;
  }


  if (!check_and_set(cfg, candidates, err)) {
    goto err;
  }
  g_hash_table_remove_all(candidates);
  g_clear_pointer(&candidates, g_hash_table_unref);

  return TRUE;

err:
  g_hash_table_remove_all(candidates);
  g_clear_pointer(&candidates, g_hash_table_unref);
  config_clear(cfg);

  return FALSE;
}


const gchar *
config_name_enum_main_deep_enumtest(enum config_main_deep_enumtest val)
{
    switch (val) {
    case MAIN_DEEP_ENUMTEST_HELLO:
      return "hello";
    case MAIN_DEEP_ENUMTEST_GOODBYE:
      return "goodbye";
    default:
      g_assert_not_reached();
    }

    return "";
}


void
config_clear(struct config *cfg)
{
  g_free(cfg->main.first);
  g_free(cfg->main.deep.param);
  g_free(cfg->main.deep.params);
  g_free(cfg->other);
  memset(cfg, 0, sizeof(*cfg));
}

gchar *
config_to_string(struct config *cfg)
{
  return g_strdup_printf("main.first: %s\n"
"main.second: %ld\n"
"main.third: %d\n"
"main.double_param: %lf\n"
"main.size: %ld\n"
"main.deep.param: %s\n"
"main.deep.enumtest: %s\n"
"main.deep.params: %s\n"
"other: %s\n"
,
  cfg->main.first,
  cfg->main.second,
  cfg->main.third,
  cfg->main.double_param,
  cfg->main.size,
  cfg->main.deep.param,
  config_name_enum_main_deep_enumtest(cfg->main.deep.enumtest),
  cfg->main.deep.params,
  cfg->other);
}

GQuark
config_error_quark(void)
{
  return g_quark_from_static_string ("xxconfig-error-quark");
}