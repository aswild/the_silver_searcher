#ifndef LANG_H
#define LANG_H

#define MAX_EXTENSIONS 12
#define SINGLE_EXT_LEN 20

typedef struct {
    const char *name;
    const char *extensions[MAX_EXTENSIONS];
} lang_spec_t;

extern const lang_spec_t langs[];

/**
 Return the language count.
 */
size_t get_lang_count(void);

/**
Combine multiple file type extensions
into a regular expression of the form \.(extension1|extension2...)$

Caller is responsible for freeing the returned string.
*/
char *make_lang_regex(size_t *ext_index, size_t len);

#endif
