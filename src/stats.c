/*
 * nine — stats.c
 *
 * Optional compile statistics: phase timings, call counts, macro replacements.
 *
 * Copyright (c) 2026 Raphaele Salvatore Licciardo
 * SPDX-License-Identifier: MIT
 */

#define QOL_STRIP_PREFIX
#include "../libs/build.h"
#include "./include/constants.h"

#if STATS

#include <stdio.h>
#include <time.h>

#include "./include/stats.h"

static NineStats g_stats = {0};

double nine_stats_now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

void nine_stats_reset(void) { g_stats = (NineStats){0}; }

void nine_stats_set_preprocess_ms(double ms) { g_stats.preprocess_ms = ms; }

void nine_stats_set_lex_ms(double ms) { g_stats.lex_ms = ms; }

void nine_stats_set_parse_ms(double ms) { g_stats.parse_ms = ms; }

void nine_stats_set_compile_ms(double ms) { g_stats.compile_ms = ms; }

void nine_stats_inc_function_calls(void) { g_stats.function_calls++; }

void nine_stats_inc_optimizations(void) { g_stats.optimizations++; }

static bool stats_phase_critical(double ms) { return ms >= STATS_PHASE_WARN_MS; }

static bool stats_compile_critical(double ms) { return ms >= STATS_COMPILE_WARN_MS; }

static bool stats_total_critical(double ms) { return ms >= STATS_TOTAL_WARN_MS; }

#define stats_log(critical, fmt, ...)                                                              \
    do {                                                                                           \
        if (critical) {                                                                            \
            warn(fmt, ##__VA_ARGS__);                                                              \
        } else {                                                                                   \
            hint(fmt, ##__VA_ARGS__);                                                              \
        }                                                                                          \
    } while (0)

void nine_stats_print(void)
{
    double total = g_stats.preprocess_ms + g_stats.lex_ms + g_stats.parse_ms + g_stats.compile_ms;

    bool preprocess_critical = stats_phase_critical(g_stats.preprocess_ms);
    bool lex_critical = stats_phase_critical(g_stats.lex_ms);
    bool parse_critical = stats_phase_critical(g_stats.parse_ms);
    bool compile_critical = stats_compile_critical(g_stats.compile_ms);
    bool total_critical = stats_total_critical(total);
    bool banner_critical =
        preprocess_critical || lex_critical || parse_critical || compile_critical || total_critical;

    stats_log(preprocess_critical, "Preprocessing time:   %.3f ms\n", g_stats.preprocess_ms);
    stats_log(lex_critical, "Lexing time:          %.3f ms\n", g_stats.lex_ms);
    stats_log(parse_critical, "Parsing time:         %.3f ms\n", g_stats.parse_ms);
    stats_log(compile_critical, "Compile time:         %.3f ms\n", g_stats.compile_ms);
    stats_log(total_critical, "Total time:           %.3f ms\n", total);
    stats_log(false, "Total function calls: %zu\n", g_stats.function_calls);
    stats_log(false, "Total optimizations:  %zu\n", g_stats.optimizations);
}

#endif
