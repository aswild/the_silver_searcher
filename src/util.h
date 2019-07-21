#ifndef UTIL_H
#define UTIL_H

#include <dirent.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "config.h"
#include "log.h"
#include "options.h"

#include <pcre2.h>

extern FILE *out_fd;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define H_SIZE (64 * 1024)

#define CHECK_AND_FREE(x) \
    do {                  \
        free(x);          \
        x = NULL;         \
    } while (0)

#ifdef __clang__
#else
#endif

#if defined(__GNUC__) || defined(__clang__)
// GCC/clang attributes, don't forget to #undef them at the bottom of this file
// clang-format off
#define ALWAYS_INLINE           __attribute__((always_inline))
#define FORMAT_PRINTF(a, b)     __attribute__((format(printf, a, b)))
#define NORETURN                __attribute__((noreturn))
// clang-format on
#else
#define ALWAYS_INLINE
#define FORMAT_PRINTF(a, b)
#define NORETURN
#endif

// disable ubsan's alignment checks for hash_strnstr which intentionally
// does unaligned access
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#if defined(__clang__) || (defined(__GNUC__) && GCC_VERSION >= 80100)
// clang and GCC 8.1 support no_sanitize("alignment")
#define NO_SANITIZE_ALIGNMENT __attribute__((no_sanitize("alignment")))
#elif defined(__GNUC__) && GCC_VERSION >= 40900
// gcc 4.9 supports ubsan and no_sanitize_undefined
#define NO_SANITIZE_ALIGNMENT __attribute__((no_sanitize_undefined))
#else
// older versions have no ubsan
#define NO_SANITIZE_ALIGNMENT
#endif
#undef GCC_VERSION

void *ag_malloc(size_t size);
void *ag_realloc(void *ptr, size_t size);
void *ag_calloc(size_t nelem, size_t elsize);
char *ag_strdup(const char *s);
char *ag_strndup(const char *s, size_t size);

typedef struct {
    size_t start; /* Byte at which the match starts */
    size_t end;   /* and where it ends */
} match_t;

typedef struct {
    size_t total_bytes;
    size_t total_files;
    size_t total_matches;
    size_t total_file_matches;
    struct timeval time_start;
    struct timeval time_end;
} ag_stats;


extern ag_stats stats;

/* Union to translate between chars and words without violating strict aliasing */
typedef union {
    char as_chars[sizeof(uint16_t)];
    uint16_t as_word;
} word_t;

void free_strings(char **strs, const size_t strs_len);

void generate_alpha_skip(const char *find, size_t f_len, size_t skip_lookup[], const int case_sensitive);
int is_prefix(const char *s, const size_t s_len, const size_t pos, const int case_sensitive);
size_t suffix_len(const char *s, const size_t s_len, const size_t pos, const int case_sensitive);
void generate_find_skip(const char *find, const size_t f_len, size_t **skip_lookup, const int case_sensitive);
void generate_hash(const char *find, const size_t f_len, uint8_t *H, const int case_sensitive);

/* max is already defined on spec-violating compilers such as MinGW */
size_t ag_max(size_t a, size_t b);
size_t ag_min(size_t a, size_t b);

const char *boyer_moore_strnstr(const char *s, const char *find, const size_t s_len, const size_t f_len,
                                const size_t alpha_skip_lookup[], const size_t *find_skip_lookup, const int case_insensitive);

NO_SANITIZE_ALIGNMENT
const char *hash_strnstr(const char *s, const char *find, const size_t s_len, const size_t f_len, uint8_t *h_table, const int case_sensitive);

size_t invert_matches(const char *buf, const size_t buf_len, match_t matches[], size_t matches_len);
void realloc_matches(match_t **matches, size_t *matches_size, size_t matches_len);

const char *ag_pcre2_version(void);
pcre2_code *ag_pcre2_compile(const char *q, uint32_t pcre_opts, bool use_jit);
static inline void ag_pcre2_free(pcre2_code **re) {
    if (re && *re) {
        pcre2_code_free(*re);
        *re = NULL;
    }
}

// convenience wrapper for pcre2_match, sets the match context to NULL and
// casts the subject to PCRE2_SPTR to avoid pointer-signedness warnings
static inline ALWAYS_INLINE int ag_pcre2_match(const pcre2_code *code, const char *subject, size_t length, size_t startoffset,
                                               uint32_t options, pcre2_match_data *match_data) {
    return pcre2_match(code, (PCRE2_SPTR)subject, length, startoffset, options, match_data, NULL);
}

int is_binary(const void *buf, const size_t buf_len);
int is_regex(const char *query);
int is_fnmatch(const char *filename);
int binary_search(const char *needle, char **haystack, int start, int end);

void init_wordchar_table(void);
int is_wordchar(char ch);

int is_lowercase(const char *s);

int is_directory(const char *path, const struct dirent *d);
int is_symlink(const char *path, const struct dirent *d);
int is_named_pipe(const char *path, const struct dirent *d);

char *join_paths(const char *a, const char *b);

void die(const char *fmt, ...) FORMAT_PRINTF(1, 2) NORETURN;

void ag_asprintf(char **ret, const char *fmt, ...) FORMAT_PRINTF(2, 3);
int ag_dsprintf(char **buf, size_t *bufsize, size_t pos, const char *fmt, ...) FORMAT_PRINTF(4, 5);

#ifndef HAVE_FGETLN
char *fgetln(FILE *fp, size_t *lenp);
#endif
#ifndef HAVE_GETLINE
ssize_t getline(char **lineptr, size_t *n, FILE *stream);
#endif
#ifndef HAVE_REALPATH
char *realpath(const char *path, char *resolved_path);
#endif
#ifndef HAVE_STRLCPY
size_t strlcpy(char *dest, const char *src, size_t size);
#endif
#ifndef HAVE_VASPRINTF
int vasprintf(char **ret, const char *fmt, va_list args);
#endif

// undefine gcc attribute macros
#undef ALWAYS_INLINE
#undef FORMAT_PRINTF
#undef NORETURN
#undef NO_SANITIZE_ALIGNMENT

#endif
