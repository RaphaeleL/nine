#define QOL_STRIP_PREFIX
#include "../libs/build.h"

#include <ctype.h>
#include <string.h>

#include "./include/preprocessor.h"
#include "./include/stats.h"

static bool is_blank(const char *line) {
    if (!line) {
        return true;
    }
    for (const char *p = line; *p; p++) {
        if (!isspace((unsigned char)*p)) {
            return false;
        }
    }
    return true;
}

static void strip_line_comment(char *line) {
    if (!line) {
        return;
    }

    bool in_string = false;
    for (char *p = line; p[0] && p[1]; p++) {
        if (*p == '"') {
            in_string = !in_string;
            continue;
        }
        if (!in_string && p[0] == '/' && p[1] == '/') {
            *p = '\0';
            break;
        }
    }

    str_trim(line);
}

static void drop_line(String *output, size_t index) {
    free(output->data[index]);
    dropn(output, index);
}

static char *join_tokens(const String *tokens, size_t start) {
    if (!tokens || start >= tokens->len) {
        return strdup("");
    }

    size_t len = 0;
    for (size_t i = start; i < tokens->len; i++) {
        len += strlen(tokens->data[i]) + 1;
    }

    char *joined = calloc(len + 1, 1);
    if (!joined) {
        return NULL;
    }

    for (size_t i = start; i < tokens->len; i++) {
        if (i > start) {
            strcat(joined, " ");
        }
        strcat(joined, tokens->data[i]);
    }

    return joined;
}

static bool parse_define(const char *line, HashMap *directives) {
    if (!line || !directives) {
        return false;
    }

    String tokens = {0};
    if (!str_split(line, ' ', &tokens) || tokens.len < 3) {
        release_string(&tokens);
        return false;
    }

    char *key = strdup(tokens.data[1]);
    char *value = join_tokens(&tokens, 2);
    release_string(&tokens);

    if (!key || !value) {
        free(key);
        free(value);
        return false;
    }

    hm_put(directives, key, value);
    free(key);

    return true;
}

static void apply_directives(String *output, HashMap *directives) {
    if (!output || !directives) {
        return;
    }

    for (size_t i = 0; i < output->len; i++) {
        const char *line = output->data[i];

        for (size_t b = 0; b < directives->capacity; b++) {
            if (directives->buckets[b].state != QOL_HM_USED) {
                continue;
            }

            const char *key = (const char *)directives->buckets[b].key;
            const char *value = (const char *)hm_get(directives, (void *)key);
            if (!value || !str_contains(line, key)) {
                continue;
            }

            char *replaced = str_replace(line, key, value);
            if (!replaced) {
                continue;
            }

            free(output->data[i]);
            output->data[i] = replaced;
            line = replaced;
            nine_stats_inc_optimizations();
        }
    }
}

HashMap *preprocess(String *output) {
    if (!output) {
        return NULL;
    }

    HashMap *directives = hm_create();
    if (!directives) {
        return NULL;
    }

    for (size_t i = 0; i < output->len; ) {
        strip_line_comment(output->data[i]);

        if (is_blank(output->data[i])) {
            drop_line(output, i);
            continue;
        }

        const char *line = output->data[i];

        if (str_starts_with(line, "#define")) {
            if (!parse_define(line, directives)) {
                hm_release(directives);
                return NULL;
            }
            drop_line(output, i);
            continue;
        }

        if (str_starts_with(line, "#include")) {
            drop_line(output, i);
            continue;
        }

        if (line[0] == '#') {
            TODO("unsupported preprocessor directive");
        }

        i++;
    }

    apply_directives(output, directives);
    return directives;
}
