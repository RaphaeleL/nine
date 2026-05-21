#ifndef STATS_H
#define STATS_H

#include "./constants.h"

#if STATS

typedef struct {
    double preprocess_ms;
    double lex_ms;
    double parse_ms;
    double compile_ms;
    size_t function_calls;
    size_t optimizations;
} NineStats;

void nine_stats_reset(void);
void nine_stats_set_preprocess_ms(double ms);
void nine_stats_set_lex_ms(double ms);
void nine_stats_set_parse_ms(double ms);
void nine_stats_set_compile_ms(double ms);
void nine_stats_inc_function_calls(void);
void nine_stats_inc_optimizations(void);
void nine_stats_print(void);
double nine_stats_now_ms(void);

#else

#define nine_stats_reset() ((void)0)
#define nine_stats_set_preprocess_ms(ms) ((void)(ms))
#define nine_stats_set_lex_ms(ms) ((void)(ms))
#define nine_stats_set_parse_ms(ms) ((void)(ms))
#define nine_stats_set_compile_ms(ms) ((void)(ms))
#define nine_stats_inc_function_calls() ((void)0)
#define nine_stats_inc_optimizations() ((void)0)
#define nine_stats_print() ((void)0)
#define nine_stats_now_ms() (0.0)

#endif

#endif
