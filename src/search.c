#include "search.h"
#include "print.h"
#include "scandir.h"
#include <stdbool.h>

#ifdef OS_LINUX
#define is_procfile(path) (strncmp((path), "/proc", 5) == 0)
#define is_sysfile(path) (strncmp((path), "/sys", 4) == 0)
#endif

// globals
size_t alpha_skip_lookup[256];
size_t *find_skip_lookup;
uint8_t h_table[H_SIZE] __attribute__((aligned(64)));
work_queue_t *work_queue;
work_queue_t *work_queue_tail;
int done_adding_files;
pthread_cond_t files_ready;
pthread_mutex_t stats_mtx;
pthread_mutex_t work_queue_mtx;
symdir_t *symhash;

/* Returns: -1 if skipped, otherwise # of matches */
ssize_t search_buf(const char *buf, const size_t buf_len,
                   const char *dir_full_path) {
    int binary = -1; /* 1 = yes, 0 = no, -1 = don't know */
    size_t buf_offset = 0;

    if (opts.search_as_text || opts.search_stream) {
        binary = 0;
    } else if (!opts.search_binary_files && opts.mmap) { /* if not using mmap, binary files have already been skipped */
        binary = is_binary((const void *)buf, buf_len);
        if (binary) {
            log_debug("File %s is binary. Skipping...", dir_full_path);
            return -1;
        }
    }

    pcre2_match_data *mdata = NULL;
    size_t matches_len = 0;
    match_t *matches;
    size_t matches_size;
    size_t matches_spare;

    if (opts.invert_match) {
        /* If we are going to invert the set of matches at the end, we will need
         * one extra match struct, even if there are no matches at all. So make
         * sure we have a nonempty array; and make sure we always have spare
         * capacity for one extra.
         */
        matches_size = 100;
        matches = ag_malloc(matches_size * sizeof(match_t));
        matches_spare = 1;
    } else {
        matches_size = 0;
        matches = NULL;
        matches_spare = 0;
    }

    if (!opts.literal && opts.query_len == 1 && opts.query[0] == '.') {
        matches_size = 1;
        matches = matches == NULL ? ag_malloc(matches_size * sizeof(match_t)) : matches;
        matches[0].start = 0;
        matches[0].end = buf_len;
        matches_len = 1;
    } else if (opts.literal) {
        const char *match_ptr = buf;

        while (buf_offset < buf_len) {
/* hash_strnstr only for little-endian platforms that allow unaligned access */
#if defined(__i386__) || defined(__x86_64__)
            /* Decide whether to fall back on boyer-moore */
            if ((size_t)opts.query_len < 2 * sizeof(uint16_t) - 1 || opts.query_len >= UCHAR_MAX) {
                match_ptr = boyer_moore_strnstr(match_ptr, opts.query, buf_len - buf_offset, opts.query_len, alpha_skip_lookup, find_skip_lookup, opts.casing == CASE_INSENSITIVE);
            } else {
                match_ptr = hash_strnstr(match_ptr, opts.query, buf_len - buf_offset, opts.query_len, h_table, opts.casing == CASE_SENSITIVE);
            }
#else
            match_ptr = boyer_moore_strnstr(match_ptr, opts.query, buf_len - buf_offset, opts.query_len, alpha_skip_lookup, find_skip_lookup, opts.casing == CASE_INSENSITIVE);
#endif

            if (match_ptr == NULL) {
                break;
            }

            if (opts.word_regexp) {
                const char *start = match_ptr;
                const char *end = match_ptr + opts.query_len;

                /* Check whether both start and end of the match lie on a word
                 * boundary
                 */
                if ((start == buf ||
                     is_wordchar(*(start - 1)) != opts.literal_starts_wordchar) &&
                    (end == buf + buf_len ||
                     is_wordchar(*end) != opts.literal_ends_wordchar)) {
                    /* It's a match */
                } else {
                    /* It's not a match */
                    match_ptr += find_skip_lookup[0] - opts.query_len + 1;
                    buf_offset = match_ptr - buf;
                    continue;
                }
            }

            realloc_matches(&matches, &matches_size, matches_len + matches_spare);

            matches[matches_len].start = match_ptr - buf;
            matches[matches_len].end = matches[matches_len].start + opts.query_len;
            buf_offset = matches[matches_len].end;
            log_debug("Match found. File %s, offset %zu bytes.", dir_full_path, matches[matches_len].start);
            matches_len++;
            match_ptr += opts.query_len;

            if (opts.max_matches_per_file > 0 && matches_len >= opts.max_matches_per_file) {
                log_err("Too many matches in %s. Skipping the rest of this file.", dir_full_path);
                break;
            }
        }
    } else {
        mdata = pcre2_match_data_create(1, NULL);
        size_t *offset_vector;
        if (mdata == NULL) {
            log_err("Failed to allocated pcre match_data for %s! Skipping this file", dir_full_path);
            goto multiline_done;
        }
        if (opts.multiline) {
            while (buf_offset < buf_len &&
                   (ag_pcre2_match(opts.re, buf, buf_len, buf_offset, 0, mdata)) >= 0) {
                offset_vector = pcre2_get_ovector_pointer(mdata);
                log_debug("Regex match found. File %s, offset %zu bytes.", dir_full_path, offset_vector[0]);
                buf_offset = offset_vector[1];
                if (offset_vector[0] == offset_vector[1]) {
                    ++buf_offset;
                    log_debug("Regex match is of length zero. Advancing offset one byte.");
                }

                realloc_matches(&matches, &matches_size, matches_len + matches_spare);

                matches[matches_len].start = offset_vector[0];
                matches[matches_len].end = offset_vector[1];
                matches_len++;

                if (opts.max_matches_per_file > 0 && matches_len >= opts.max_matches_per_file) {
                    log_err("Too many matches in %s. Skipping the rest of this file.", dir_full_path);
                    break;
                }
            }
        } else {
            while (buf_offset < buf_len) {
                const char *const line = buf + buf_offset;
                const char *line_end = memchr(line, '\n', buf_len - buf_offset);
                if (!line_end) {
                    line_end = buf + buf_len;
                }
                const size_t line_len = line_end - line;

                size_t line_offset = 0;
                while (line_offset < line_len) {
                    int rv = ag_pcre2_match(opts.re, line, line_len, line_offset, 0, mdata);
                    if (rv < 0) {
                        break;
                    }
                    offset_vector = pcre2_get_ovector_pointer(mdata);
                    log_debug("Regex match found. File %s, offset %zu bytes.", dir_full_path, offset_vector[0]);
                    line_offset = offset_vector[1];
                    if (offset_vector[0] == offset_vector[1]) {
                        ++line_offset;
                        log_debug("Regex match is of length zero. Advancing offset one byte.");
                    }

                    realloc_matches(&matches, &matches_size, matches_len + matches_spare);

                    matches[matches_len].start = offset_vector[0] + buf_offset;
                    matches[matches_len].end = offset_vector[1] + buf_offset;
                    matches_len++;

                    if (opts.max_matches_per_file > 0 && matches_len >= opts.max_matches_per_file) {
                        log_err("Too many matches in %s. Skipping the rest of this file.", dir_full_path);
                        goto multiline_done;
                    }
                }
                buf_offset += line_len + 1;
            }
        }
    }

multiline_done:
    if (mdata != NULL) {
        pcre2_match_data_free(mdata);
    }

    if (opts.invert_match) {
        matches_len = invert_matches(buf, buf_len, matches, matches_len);
    }

    if (opts.stats) {
        pthread_mutex_lock(&stats_mtx);
        stats.total_bytes += buf_len;
        stats.total_files++;
        stats.total_matches += matches_len;
        if (matches_len > 0) {
            stats.total_file_matches++;
        }
        pthread_mutex_unlock(&stats_mtx);
    }

    if (!opts.print_nonmatching_files && (matches_len > 0 || opts.print_all_paths)) {
        if (binary == -1 && !opts.print_filename_only) {
            binary = is_binary((const void *)buf, buf_len);
        }
        pthread_mutex_lock(&print_mtx);
        if (opts.print_filename_only) {
            if (opts.print_count) {
                print_path_count(dir_full_path, opts.path_sep, (size_t)matches_len);
            } else {
                print_path(dir_full_path, opts.path_sep);
            }
        } else if (binary) {
            print_binary_file_matches(dir_full_path);
        } else {
            print_file_matches(dir_full_path, buf, buf_len, matches, matches_len);
        }
        pthread_mutex_unlock(&print_mtx);
        opts.match_found = 1;
    } else if (opts.search_stream && opts.passthrough) {
        fprintf(out_fd, "%s", buf);
    } else {
        log_debug("No match in %s", dir_full_path);
    }

    if (matches_len == 0 && opts.search_stream) {
        print_context_append(buf, buf_len - 1);
    }

    if (matches_size > 0) {
        free(matches);
    }

    /* FIXME: handle case where matches_len > SSIZE_MAX */
    return (ssize_t)matches_len;
}

/* Return value: -1 if skipped, otherwise # of matches */
/* TODO: this will only match single lines. multi-line regexes silently don't match */
ssize_t search_stream(FILE *stream, const char *path) {
    char *line = NULL;
    ssize_t matches_count = 0;
    ssize_t line_len = 0;
    size_t line_cap = 0;
    size_t i;

    print_init_context();

    for (i = 1; (line_len = getline(&line, &line_cap, stream)) > 0; i++) {
        ssize_t result;
        opts.stream_line_num = i;
        result = search_buf(line, line_len, path);
        if (result > 0) {
            if (matches_count == -1) {
                matches_count = 0;
            }
            matches_count += result;
        } else if (matches_count <= 0 && result == -1) {
            matches_count = -1;
        }
        if (line[line_len - 1] == '\n') {
            line_len--;
        }
        print_trailing_context(path, line, line_len);
    }

    free(line);
    print_cleanup_context();
    return matches_count;
}

void search_file(const char *file_full_path) {
    int fd = -1;
    off_t f_len = 0;
    char *buf = NULL;
    struct stat statbuf;
    int rv = 0;
    int matches_count = -1;
    FILE *fp = NULL;

    rv = stat(file_full_path, &statbuf);
    if (rv != 0) {
        rv = lstat(file_full_path, &statbuf);
        if (S_ISLNK(statbuf.st_mode)) {
            log_debug("Skipping %s: broken symlink", file_full_path);
        } else {
            log_err("Skipping %s: Error fstat()ing file.", file_full_path);
        }
        goto cleanup;
    }

    if (opts.stdout_inode != 0 && opts.stdout_inode == statbuf.st_ino) {
        log_debug("Skipping %s: stdout is redirected to it", file_full_path);
        goto cleanup;
    }

    // handling only regular files and FIFOs
    if (!S_ISREG(statbuf.st_mode) && !S_ISFIFO(statbuf.st_mode)) {
        log_err("Skipping %s: Mode %u is not a file.", file_full_path, statbuf.st_mode);
        goto cleanup;
    }

    fd = open(file_full_path, O_RDONLY);
    if (fd < 0) {
        /* XXXX: strerror is not thread-safe */
        log_err("Skipping %s: Error opening file: %s", file_full_path, strerror(errno));
        goto cleanup;
    }

    // repeating stat check with file handle to prevent TOCTOU issue
    rv = fstat(fd, &statbuf);
    if (rv != 0) {
        rv = lstat(file_full_path, &statbuf);
        if (S_ISLNK(statbuf.st_mode)) {
            log_debug("Skipping %s: broken symlink", file_full_path);
        } else {
            log_err("Skipping %s: Error fstat()ing file.", file_full_path);
        }
        goto cleanup;
    }

    if (opts.stdout_inode != 0 && opts.stdout_inode == statbuf.st_ino) {
        log_debug("Skipping %s: stdout is redirected to it", file_full_path);
        goto cleanup;
    }

    // handling only regular files and FIFOs
    if (!S_ISREG(statbuf.st_mode) && !S_ISFIFO(statbuf.st_mode)) {
        log_err("Skipping %s: Mode %u is not a file.", file_full_path, statbuf.st_mode);
        goto cleanup;
    }

    print_init_context();

    if (statbuf.st_mode & S_IFIFO) {
        log_debug("%s is a named pipe. stream searching", file_full_path);
        fp = fdopen(fd, "r");
        matches_count = search_stream(fp, file_full_path);
        fclose(fp);
        goto cleanup;
    }

    f_len = statbuf.st_size;

#ifdef OS_LINUX
    if (f_len == 0 && !is_procfile(file_full_path)) {
#else
    if (f_len == 0) {
#endif
        if (opts.query[0] == '.' && opts.query_len == 1 && !opts.literal && opts.search_all_files) {
            matches_count = search_buf(buf, f_len, file_full_path);
        } else {
            log_debug("Skipping %s: file is empty.", file_full_path);
        }
        goto cleanup;
    }

    if (!opts.literal && f_len > INT_MAX) {
        log_err("Skipping %s: pcre_exec() can't handle files larger than %i bytes.", file_full_path, INT_MAX);
        goto cleanup;
    }

#ifdef _WIN32
    {
        HANDLE hmmap = CreateFileMapping(
            (HANDLE)_get_osfhandle(fd), 0, PAGE_READONLY, 0, f_len, NULL);
        buf = (char *)MapViewOfFile(hmmap, FILE_SHARE_READ, 0, 0, f_len);
        if (hmmap != NULL)
            CloseHandle(hmmap);
    }
    if (buf == NULL) {
        FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, GetLastError(), 0, (void *)&buf, 0, NULL);
        log_err("File %s failed to load: %s.", file_full_path, buf);
        LocalFree((void *)buf);
        goto cleanup;
    }
#else

#ifdef OS_LINUX
    if (is_procfile(file_full_path)) {
        // /proc files can't be mmap'd and show up as zero-length. Sometimes we can lseek them to get the size and sometimes we can't
        ssize_t bytes_read = 0;
        f_len = lseek(fd, 0, SEEK_END);
        if (f_len != -1) {
            // lseek succeeded, so we know how much to read
            lseek(fd, 0, SEEK_SET);
            buf = ag_malloc(f_len);
            bytes_read = read(fd, buf, f_len);
            if ((off_t)bytes_read != f_len) {
                die("expected to read %zu bytes but read %zu", (size_t)f_len, bytes_read);
            }
        } else {
            // lseek failed, so use the last-resort of dynamically reallocating the buffer until we've read as much as we can
            size_t buf_size = 4096;
            buf = ag_malloc(buf_size);
            while (TRUE) {
                bytes_read += read(fd, buf + bytes_read, buf_size - bytes_read);
                if (bytes_read < 0) {
                    die("Unable to read %s", file_full_path);
                } else if ((size_t)bytes_read < buf_size) {
                    break; // assume we hit the end
                } else {
                    // realloc the buffer and keep reading
                    buf_size *= 2;
                    buf = ag_realloc(buf, buf_size);
                }
            }
        }
        log_debug("%s: read %zu", file_full_path, bytes_read);
        f_len = (off_t)bytes_read;
    } else if (is_sysfile(file_full_path)) {
        // /sys files can't be mmap'd, and reading them might return less than the expected amount
        buf = ag_malloc(f_len);
        ssize_t bytes_read = read(fd, buf, f_len);
        if (bytes_read < 0) {
            die("Unable to read %s", file_full_path);
        }
        f_len = (off_t)bytes_read; // update file size with the actual amount we read
    } else if (opts.mmap) {
#else
    if (opts.mmap) {
#endif // OS_LINUX
        buf = mmap(0, f_len, PROT_READ, MAP_PRIVATE, fd, 0);
        if (buf == MAP_FAILED) {
            log_err("File %s failed to load: %s.", file_full_path, strerror(errno));
            goto cleanup;
        }
#if HAVE_MADVISE
        madvise(buf, f_len, MADV_SEQUENTIAL);
#elif HAVE_POSIX_FADVISE
        posix_fadvise(fd, 0, f_len, POSIX_MADV_SEQUENTIAL);
#endif
    } else {
        buf = ag_malloc(f_len);

        ssize_t bytes_read = 0;

        if (!opts.search_binary_files) {
            bytes_read = read(fd, buf, ag_min(f_len, 512));
            if (bytes_read < 0) {
                die("Failed to read %s: %s", file_full_path, strerror(errno));
            }
            // Optimization: If skipping binary files, don't read the whole buffer before checking if binary or not.
            if (is_binary(buf, bytes_read)) {
                log_debug("File %s is binary. Skipping...", file_full_path);
                goto cleanup;
            }
        }

        while (bytes_read < f_len) {
            ssize_t r = read(fd, buf + bytes_read, f_len);
            if (r < 0)
                break;
            bytes_read += r;
        }
        if (bytes_read != f_len) {
            die("File %s read(): expected to read %zu bytes but read %zu", file_full_path, (size_t)f_len, bytes_read);
        }
    }
#endif

    if (opts.search_zip_files) {
        log_debug("check if %s is compressed", file_full_path);
        ag_compression_type zip_type = is_zipped(buf, f_len);
        if (zip_type != AG_NO_COMPRESSION) {
            size_t _buf_len = f_len;
            char *_buf = decompress(zip_type, buf, f_len, file_full_path, &_buf_len);
            if (_buf == NULL || _buf_len == 0) {
                log_err("Cannot decompress zipped file %s", file_full_path);
                goto cleanup;
            }
            matches_count = search_buf(_buf, _buf_len, file_full_path);
            free(_buf);
            goto cleanup;
        }
    }

    matches_count = search_buf(buf, f_len, file_full_path);

cleanup:

    if (opts.print_nonmatching_files && matches_count == 0) {
        pthread_mutex_lock(&print_mtx);
        print_path(file_full_path, opts.path_sep);
        pthread_mutex_unlock(&print_mtx);
        opts.match_found = 1;
    }

    print_cleanup_context();
    if (buf != NULL) {
#ifdef _WIN32
        UnmapViewOfFile(buf);
#else
#ifdef OS_LINUX
        if (is_procfile(file_full_path) || is_sysfile(file_full_path)) {
            free(buf);
        } else if (opts.mmap) {
#else
        if (opts.mmap) {
#endif
            if (buf != MAP_FAILED) {
                munmap(buf, f_len);
            }
        } else {
            free(buf);
        }
#endif
    }
    if (fd != -1) {
        close(fd);
    }
}

void *search_file_worker(void *i) {
    work_queue_t *queue_item;
    int worker_id = *(int *)i;

    log_debug("Worker %i started", worker_id);
    while (TRUE) {
        pthread_mutex_lock(&work_queue_mtx);
        while (work_queue == NULL) {
            if (done_adding_files) {
                pthread_mutex_unlock(&work_queue_mtx);
                log_debug("Worker %i finished.", worker_id);
                pthread_exit(NULL);
            }
            pthread_cond_wait(&files_ready, &work_queue_mtx);
        }
        queue_item = work_queue;
        work_queue = work_queue->next;
        if (work_queue == NULL) {
            work_queue_tail = NULL;
        }
        pthread_mutex_unlock(&work_queue_mtx);

        search_file(queue_item->path);
        free(queue_item->path);
        free(queue_item);
    }
}

static int check_symloop_enter(const char *path, dirkey_t *outkey) {
#ifdef _WIN32
    return SYMLOOP_OK;
#else
    struct stat buf;
    symdir_t *item_found = NULL;
    symdir_t *new_item = NULL;

    memset(outkey, 0, sizeof(dirkey_t));
    outkey->dev = 0;
    outkey->ino = 0;

    int res = stat(path, &buf);
    if (res != 0) {
        log_err("Error stat()ing: %s", path);
        return SYMLOOP_ERROR;
    }

    outkey->dev = buf.st_dev;
    outkey->ino = buf.st_ino;

    HASH_FIND(hh, symhash, outkey, sizeof(dirkey_t), item_found);
    if (item_found) {
        return SYMLOOP_LOOP;
    }

    new_item = (symdir_t *)ag_malloc(sizeof(symdir_t));
    memcpy(&new_item->key, outkey, sizeof(dirkey_t));
    HASH_ADD(hh, symhash, key, sizeof(dirkey_t), new_item);
    return SYMLOOP_OK;
#endif
}

static int check_symloop_leave(dirkey_t *dirkey) {
#ifdef _WIN32
    return SYMLOOP_OK;
#else
    symdir_t *item_found = NULL;

    if (dirkey->dev == 0 && dirkey->ino == 0) {
        return SYMLOOP_ERROR;
    }

    HASH_FIND(hh, symhash, dirkey, sizeof(dirkey_t), item_found);
    if (!item_found) {
        log_err("item not found! weird stuff...\n");
        return SYMLOOP_ERROR;
    }

    HASH_DELETE(hh, symhash, item_found);
    free(item_found);
    return SYMLOOP_OK;
#endif
}

/* TODO: Append matches to some data structure instead of just printing them out.
 * Then ag can have sweet summaries of matches/files scanned/time/etc.
 */
void search_dir(ignores *ig, const char *base_path, const char *path, const int depth,
                dev_t original_dev) {
    struct dirent **dir_list = NULL;
    struct dirent *dir = NULL;
    scandir_baton_t scandir_baton;
    int results = 0;
    size_t base_path_len = 0;
    const char *path_start = path;
    pcre2_match_data *mdata = NULL;

    char *dir_full_path = NULL;
    const char *ignore_file = NULL;
    int i;

    int symres;
    dirkey_t current_dirkey;

    symres = check_symloop_enter(path, &current_dirkey);
    if (symres == SYMLOOP_LOOP) {
        log_err("Recursive directory loop: %s", path);
        return;
    }

    /* find .*ignore files to load ignore patterns from */
    for (i = 0; opts.skip_vcs_ignores ? (i == 0) : (ignore_pattern_files[i] != NULL); i++) {
        ignore_file = ignore_pattern_files[i];
        ag_asprintf(&dir_full_path, "%s/%s", path, ignore_file);
        load_ignore_patterns(ig, dir_full_path);
        free(dir_full_path);
        dir_full_path = NULL;
    }

    /* path_start is the part of path that isn't in base_path
     * base_path will have a trailing '/' because we put it there in parse_options
     */
    base_path_len = base_path ? strlen(base_path) : 0;
    for (i = 0; ((size_t)i < base_path_len) && (path[i]) && (base_path[i] == path[i]); i++) {
        path_start = path + i + 1;
    }
    log_debug("search_dir: path is '%s', base_path is '%s', path_start is '%s'", path, base_path, path_start);

    scandir_baton.ig = ig;
    scandir_baton.base_path = base_path;
    scandir_baton.base_path_len = base_path_len;
    scandir_baton.path_start = path_start;

    results = ag_scandir(path, &dir_list, &filename_filter, &scandir_baton);
    if (results == 0) {
        log_debug("No results found in directory %s", path);
        goto search_dir_cleanup;
    } else if (results == -1) {
        if (errno == ENOTDIR) {
            /* Not a directory. Probably a file. */
            if (depth == 0 && opts.paths_len == 1) {
                /* If we're only searching one file, don't print the filename header at the top. */
                if (opts.print_path == PATH_PRINT_DEFAULT || opts.print_path == PATH_PRINT_DEFAULT_EACH_LINE) {
                    opts.print_path = PATH_PRINT_NOTHING;
                }
                /* If we're only searching one file and --column or --number aren't specified, disable line numbers too. */
                if (opts.print_line_numbers == TRUE && !opts.column && opts.print_path == PATH_PRINT_NOTHING) {
                    opts.print_line_numbers = FALSE;
                }
            }
            search_file(path);
        } else {
            log_err("Error opening directory %s: %s", path, strerror(errno));
        }
        goto search_dir_cleanup;
    }

    mdata = pcre2_match_data_create(1, NULL);
    int rc = 0;
    work_queue_t *queue_item;

    for (i = 0; i < results; i++) {
        queue_item = NULL;
        dir = dir_list[i];
        ag_asprintf(&dir_full_path, "%s/%s", path, dir->d_name);
#ifndef _WIN32
        if (opts.one_dev) {
            struct stat s;
            if (lstat(dir_full_path, &s) != 0) {
                log_err("Failed to get device information for %s. Skipping...", dir->d_name);
                goto cleanup;
            }
            if (s.st_dev != original_dev) {
                log_debug("File %s crosses a device boundary (is probably a mount point.) Skipping...", dir->d_name);
                goto cleanup;
            }
        }
#endif

        /* If a link points to a directory then we need to treat it as a directory. */
        if (!opts.follow_symlinks && is_symlink(path, dir)) {
            log_debug("File %s ignored becaused it's a symlink", dir->d_name);
            goto cleanup;
        }

        if (!is_directory(path, dir)) {
            if (opts.file_search_regex || opts.filetype_regex) {
                bool filename_matched = true;
                if (opts.filetype_regex) {
                    rc = ag_pcre2_match(opts.filetype_regex, dir_full_path, strlen(dir_full_path), 0, 0, mdata);
                    if (rc < 0)
                        filename_matched = false;
                }
                if (filename_matched && opts.file_search_regex) {
                    const char *file_search_path = opts.file_search_regex_just_filename ? dir->d_name : dir_full_path;
                    rc = ag_pcre2_match(opts.file_search_regex, file_search_path, strlen(file_search_path), 0, 0, mdata);

                    /* XOR between finding a match and inverting that regex. Either but not both means
                     * to continue searching the file */
                    if (rc < 0)
                        filename_matched = !!(opts.invert_file_search_regex);
                    else
                        filename_matched = !(opts.invert_file_search_regex);
                }

                if (!filename_matched) { /* no match */
                    log_debug("Skipping %s due to file_search_regex.", dir_full_path);
                    goto cleanup;
                } else if (opts.match_files) {
                    log_debug("match_files: file_search_regex/filetype_regex matched for %s.", dir_full_path);
                    pthread_mutex_lock(&print_mtx);
                    if (!opts.file_search_regex_just_filename) {
                        print_path_match(dir_full_path, opts.path_sep, pcre2_get_ovector_pointer(mdata));
                    } else {
                        const size_t *m_ovec = pcre2_get_ovector_pointer(mdata);
                        if (m_ovec) {
                            size_t offset = strlen(path) + 1;
                            size_t ovec[2] = { m_ovec[0] + offset, m_ovec[1] + offset };
                            print_path_match(dir_full_path, opts.path_sep, ovec);
                        } else {
                            print_path_match(dir_full_path, opts.path_sep, NULL);
                        }
                    }
                    pthread_mutex_unlock(&print_mtx);
                    opts.match_found = 1;
                    goto cleanup;
                }
            }

            queue_item = ag_malloc(sizeof(work_queue_t));
            queue_item->path = dir_full_path;
            queue_item->next = NULL;
            pthread_mutex_lock(&work_queue_mtx);
            if (work_queue_tail == NULL) {
                work_queue = queue_item;
            } else {
                work_queue_tail->next = queue_item;
            }
            work_queue_tail = queue_item;
            pthread_cond_signal(&files_ready);
            pthread_mutex_unlock(&work_queue_mtx);
            log_debug("%s added to work queue", dir_full_path);
        } else if (opts.recurse_dirs) {
            if (depth < opts.max_search_depth || opts.max_search_depth == -1) {
                log_debug("Searching dir %s", dir_full_path);
                ignores *child_ig;
#ifdef HAVE_DIRENT_DNAMLEN
                child_ig = init_ignore(ig, dir->d_name, dir->d_namlen);
#else
                child_ig = init_ignore(ig, dir->d_name, strlen(dir->d_name));
#endif
                search_dir(child_ig, base_path, dir_full_path, depth + 1,
                           original_dev);
                cleanup_ignore(child_ig);
            } else {
                if (opts.max_search_depth == DEFAULT_MAX_SEARCH_DEPTH) {
                    /*
                     * If the user didn't intentionally specify a particular depth,
                     * this is a warning...
                     */
                    log_err("Skipping %s. Use the --depth option to search deeper.", dir_full_path);
                } else {
                    /* ... if they did, let's settle for debug. */
                    log_debug("Skipping %s. Use the --depth option to search deeper.", dir_full_path);
                }
            }
        }

    cleanup:
        free(dir);
        dir = NULL;
        if (queue_item == NULL) {
            free(dir_full_path);
            dir_full_path = NULL;
        }
    }

search_dir_cleanup:
    if (mdata != NULL) {
        pcre2_match_data_free(mdata);
    }
    check_symloop_leave(&current_dirkey);
    free(dir_list);
    dir_list = NULL;
}
