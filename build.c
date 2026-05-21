#define QOL_IMPLEMENTATION
#define QOL_STRIP_PREFIX
#include "./libs/build.h"

#include <stdlib.h>
#include <string.h>

#define SRC_DIR     "src"
#define OUT_DIR     "out"
#define LOG_DIR     "logs"
#define LOG_MAX     20
#define BINARY      "nine"
#define MAIN_SRC    SRC_DIR "/main.c"
#define INCLUDE     "-I" SRC_DIR "/include"

static bool is_main_source(const char *path) {
    return str_ends_with(path, "/main.c") || str_ends_with(path, "\\main.c");
}

typedef struct {
    char *path;
    time_t mtime;
} LogEntry;

static int log_entry_older_first(const void *a, const void *b) {
    const LogEntry *lhs = a;
    const LogEntry *rhs = b;
    if (lhs->mtime < rhs->mtime) return -1;
    if (lhs->mtime > rhs->mtime) return 1;
    return 0;
}

static const char *log_basename(const char *path) {
    const char *base = strrchr(path, '/');
    if (base) return base + 1;
    base = strrchr(path, '\\');
    return base ? base + 1 : path;
}

static void rotate_logs(void) {
    String files = {0};
    if (!get_files_in_dir(LOG_DIR, &files)) {
        return;
    }

    LogEntry *logs = NULL;
    size_t count = 0;
    size_t cap = 0;

    for (size_t i = 0; i < files.len; i++) {
        char *path = files.data[i];
        if (!str_starts_with(log_basename(path), "build_") || !str_ends_with(path, ".log")) {
            continue;
        }

        struct stat st;
        if (stat(path, &st) != 0) {
            continue;
        }
#if !defined(WINDOWS)
        if (!S_ISREG(st.st_mode)) {
            continue;
        }
#endif

        if (count == cap) {
            cap = cap ? cap * 2 : 8;
            LogEntry *grown = realloc(logs, cap * sizeof(LogEntry));
            if (!grown) {
                free(logs);
                release_string(&files);
                return;
            }
            logs = grown;
        }

        logs[count].path = path;
        logs[count].mtime = st.st_mtime;
        count++;
    }

    if (count >= LOG_MAX) {
        qsort(logs, count, sizeof(LogEntry), log_entry_older_first);
        size_t to_remove = count - (LOG_MAX - 1);
        for (size_t i = 0; i < to_remove; i++) {
            delete_file(logs[i].path);
        }
    }

    free(logs);
    release_string(&files);
}

static bool compile_sources(Procs *procs) {
    String sources = {0};
    if (!read_dir(SRC_DIR, &sources)) return false;

    mkdir_if_not_exists(OUT_DIR);

    bool ok = true;
    for (size_t i = 0; i < sources.len; i++) {
        const char *path = sources.data[i];
        if (!str_ends_with(path, ".c")) continue;
        if (is_main_source(path)) continue;

        char *name = get_filename_no_ext(path);
        if (!name) {
            ok = false;
            break;
        }

        char *obj = temp_sprintf("%s/%s.o", OUT_DIR, name);
        free(name);

        Cmd compile = {0};
        push(&compile, "cc", "-Wall", "-Wextra", INCLUDE, "-c", path, "-o", obj);
        compile.async = true;

        if (!run(&compile, .procs = procs)) {
            ok = false;
            break;
        }
    }

    release_string(&sources);
    return ok;
}

static bool link_binary(void) {
    String sources = {0};
    String objs = {0};
    if (!read_dir(SRC_DIR, &sources)) {
        return false;
    }

    for (size_t i = 0; i < sources.len; i++) {
        const char *path = sources.data[i];
        if (!str_ends_with(path, ".c")) {
            continue;
        }
        if (is_main_source(path)) {
            continue;
        }

        char *name = get_filename_no_ext(path);
        if (!name) {
            release_string(&sources);
            release_string(&objs);
            return false;
        }

        char *obj = temp_sprintf("%s/%s.o", OUT_DIR, name);
        free(name);

        char *obj_path = strdup(obj);
        if (!obj_path) {
            release_string(&sources);
            release_string(&objs);
            return false;
        }
        push(&objs, obj_path);
    }

    const char **inputs = calloc(objs.len + 1, sizeof(const char *));
    if (!inputs) {
        release_string(&sources);
        release_string(&objs);
        return false;
    }

    for (size_t i = 0; i < objs.len; i++) {
        inputs[i] = objs.data[i];
    }
    inputs[objs.len] = MAIN_SRC;

    int rebuild = needs_rebuild(BINARY, inputs, objs.len + 1);
    free(inputs);

    if (rebuild == 0) {
        info("Up to date: %s\n", BINARY);
        release_string(&sources);
        release_string(&objs);
        return true;
    }
    if (rebuild < 0) {
        release_string(&sources);
        release_string(&objs);
        return false;
    }

    Cmd link = {0};
    push(&link, "cc", "-Wall", "-Wextra", INCLUDE);
    for (size_t i = 0; i < objs.len; i++) {
        push(&link, objs.data[i]);
    }
    push(&link, MAIN_SRC, "-o", BINARY);

    bool ok = run_always(&link);

    release_string(&sources);
    release_string(&objs);
    return ok;
}

int main(void) {
    init_logger(.level = LOG_INFO, .time = true, .color = true, .time_color = true);
    mkdir_if_not_exists(LOG_DIR);
    rotate_logs();
    init_logger_logfile(LOG_DIR "/build_%s.log", DATETIME);
    auto_rebuild_plus(__FILE__, "./libs/build.h");

    Procs procs = {0};
    if (!compile_sources(&procs))   return EXIT_FAILURE;
    if (!procs_wait(&procs))        return EXIT_FAILURE;
    if (!link_binary())                    return EXIT_FAILURE;


    Cmd cmd = {0};
    push(&cmd, "./nine", "examples/000_helloworld.9", "-o", "./out/helloworld", "-s", "-r", "--target", "arm64");
    if (!run_always(&cmd)) return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
