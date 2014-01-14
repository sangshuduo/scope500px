#ifndef __500PX_SCOPE_PARSER_FUNC_H__
#define __500PX_SCOPE_PARSER_FUNC_H__

/**
 * This is just an example result type with some sample
 * fields. You should modify the fields to match the
 * data you are expecting from your search source
 */
typedef struct {
    gchar *link;
    gchar *icon_url;
    gchar *title;
    gchar *description;
    gchar *creation_date;
    gchar *author;
} result_t;

GSList *get_results(const char *search_term);
void print_result_info(result_t *result);
void result_cleanup(gpointer data);

#endif /* __500PX_SCOPE_PARSER_FUNC_H__ */
