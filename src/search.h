#ifndef SEARCH_H
#define SEARCH_H

#include "config.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

#include "ignore.h"
#include "util.h"
#include "uthash.h"

extern size_t alpha_skip_lookup[256];
extern size_t *find_skip_lookup;
extern uint8_t h_table[H_SIZE] __attribute__((aligned(64)));

struct work_queue_t {
    char *path;
    struct work_queue_t *next;
};
typedef struct work_queue_t work_queue_t;

extern work_queue_t *work_queue;
extern work_queue_t *work_queue_tail;
extern int done_adding_files;
extern pthread_cond_t files_ready;
extern pthread_mutex_t stats_mtx;
extern pthread_mutex_t work_queue_mtx;


/* For symlink loop detection */
#define SYMLOOP_ERROR (-1)
#define SYMLOOP_OK (0)
#define SYMLOOP_LOOP (1)

typedef struct {
    dev_t dev;
    ino_t ino;
} dirkey_t;

typedef struct {
    dirkey_t key;
    UT_hash_handle hh;
} symdir_t;

extern symdir_t *symhash;

ssize_t search_buf(const char *buf, const size_t buf_len,
                   const char *dir_full_path);
ssize_t search_stream(FILE *stream, const char *path);
void search_file(const char *file_full_path);

void *search_file_worker(void *i);

void search_dir(ignores *ig, const char *base_path, const char *path, const int depth, dev_t original_dev);

#endif
