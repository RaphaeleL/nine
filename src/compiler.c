/*
 * nine — compiler.c
 *
 * Target selection and dispatch to the active code generator backend.
 *
 * Copyright (c) 2026 Raphaele Salvatore Licciardo
 * SPDX-License-Identifier: MIT
 */

#define QOL_STRIP_PREFIX
#include "./include/compiler.h"

#include <string.h>

#include "../libs/build.h"
#include "./include/arm64.h"
#include "./include/stats.h"
#include "./include/x86_64.h"

Target get_target(void)
{
    arg_t *target = get_argument("--target");
    if (!target) {
        erro("No target provided\n");
        exit(EXIT_FAILURE);
    }
    if (strcmp(target->value, "arm64") == 0) {
        return TARGET_ARM64;
    }
    if (strcmp(target->value, "x86_64") == 0) {
        return TARGET_X86_64;
    }
    return TARGET_UNSUPPORTED;
}

bool nine_output_binary_path(const CompileOptions *opts, char *buf, size_t buflen)
{
    if (!opts || !opts->input_path || !buf || buflen == 0) {
        return false;
    }

    const char *stem_source = opts->output_path ? opts->output_path : opts->input_path;
    char *base = get_filename_no_ext(stem_source);
    if (!base) {
        return false;
    }

    int n;
    if (opts->output_path) {
        n = snprintf(buf, buflen, "%s", opts->output_path);
    } else {
        n = snprintf(buf, buflen, "./%s", base);
    }
    free(base);
    return n > 0 && (size_t)n < buflen;
}

bool nine_run_binary(const CompileOptions *opts)
{
    char bin_path[256] = {0};
    if (!nine_output_binary_path(opts, bin_path, sizeof(bin_path))) {
        erro("Failed to resolve output binary path\n");
        return false;
    }

    Cmd run_bin = {0};
    push(&run_bin, bin_path);
    info("Running: %s\n", bin_path);
    return run_always(&run_bin);
}

const char *target_name(Target target)
{
    const char *real_target = "n/a";
    switch (target) {
    case TARGET_ARM64:
        return "arm64";
    case TARGET_X86_64:
        return "x86_64";
    case TARGET_UNSUPPORTED:
        return real_target;
    }
}

bool compile(AstNode *program, const CompileOptions *opts)
{
    if (!program || !opts || !opts->input_path) {
        return false;
    }

    double phase_start = nine_stats_now_ms();
    bool ok = false;
    Target target = get_target();

    switch (target) {
    case TARGET_ARM64:
        ok = arm64_compile(program, opts);
        break;
    case TARGET_X86_64:
        x86_64_compile(program);
        ok = true;
        break;
    case TARGET_UNSUPPORTED:
        erro("Unsupported target: %s\n", target_name(target));
        ok = false;
        break;
    }

    nine_stats_set_compile_ms(nine_stats_now_ms() - phase_start);
    return ok;
}
