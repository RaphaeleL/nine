/*
 * nine — main.c
 *
 * Compiler entry point: CLI, pipeline orchestration (preprocess, lex, parse,
 * compile).
 *
 * Copyright (c) 2026 Raphaele Salvatore Licciardo
 * SPDX-License-Identifier: MIT
 */

#define QOL_IMPLEMENTATION
#define QOL_STRIP_PREFIX
#include "./include/main.h"

#include "../libs/build.h"
#include "./include/compiler.h"
#include "./include/constants.h"
#include "./include/lexer.h"
#include "./include/parser.h"
#include "./include/preprocessor.h"
#include "./include/stats.h"

static bool arg_is_set(arg_t *arg) { return arg && arg->value && strcmp(arg->value, "1") == 0; }

static bool arg_has_value(arg_t *arg) { return arg && arg->value && strcmp(arg->value, "1") != 0; }

static bool cli_flag_takes_value(const char *flag)
{
    return strcmp(flag, "--output") == 0 || strcmp(flag, "--target") == 0;
}

static bool cli_is_option_value(int argc, char **argv, int index)
{
    if (index <= 0 || index >= argc) {
        return false;
    }

    const char *prev = argv[index - 1];
    if (strcmp(prev, "--output") == 0 || strcmp(prev, "--target") == 0) {
        return true;
    }
    if (strcmp(prev, "-o") == 0 || strcmp(prev, "-t") == 0) {
        return true;
    }

    return false;
}

static bool cli_flag_enabled(int argc, char **argv, const char *short_flag, const char *long_flag)
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], short_flag) == 0 || strcmp(argv[i], long_flag) == 0) {
            return true;
        }
    }
    return false;
}

void version(void)
{
    info("Nine " VERSION "\n");
    info("Copyright (c) 2026 Raphaele Salvatore Licciardo\n");
}

void usage(void)
{
    info("Usage: nine [options] <file.9>\n");
    info("Options:\n");
    info("  -h, --help           Show help\n");
    info("  -v, --version        Show version\n");
    info("  -o, --output <path>  Output binary path (default: input name without "
         "extension)\n");
    info("  -s, --save-asm       Save assembly next to output path (with -o) or "
         "in .\n");
    info("  -r, --run            Run the linked binary after build\n");
    info("  -t, --target <arch>  Target architecture (default: arm64)\n");
}

int main(int argc, char **argv)
{
    if (DEBUG) {
        init_logger(.level = LOG_INFO, .time = true, .color = true, .time_color = true);
    } else {
        init_logger(.only = LOG_INFO, .only_set=true, .time = true, .color = !true, .time_color = !true);
    }


    add_argument("--version", NULL, "Show version");
    add_argument("--target", "arm64", "Set target architecture");
    add_argument("--output", NULL, "Output binary path");
    add_argument("--save-asm", NULL, "Save assembly in current directory");
    add_argument("--run", NULL, "Run the linked binary after build");
    init_argparser(argc, argv);

    if (arg_is_set(get_argument("--version"))) {
        version();
        return EXIT_SUCCESS;
    }

    const char *input_file_path = NULL;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (cli_flag_takes_value(argv[i]) && i + 1 < argc) {
                i++;
            }
            continue;
        }

        if (cli_is_option_value(argc, argv, i)) {
            continue;
        }

        if (input_file_path) {
            erro("Multiple input files provided\n");
            return EXIT_FAILURE;
        }
        input_file_path = argv[i];
    }

    if (!input_file_path) {
        erro("No input file provided\n");
        return EXIT_FAILURE;
    }

    arg_t *output_arg = get_argument("--output");
    CompileOptions opts = {
        .input_path = input_file_path,
        .output_path = arg_has_value(output_arg) ? output_arg->value : NULL,
        .save_asm = cli_flag_enabled(argc, argv, "-s", "--save-asm"),
        .run = cli_flag_enabled(argc, argv, "-r", "--run"),
    };

    String input_file = {0};
    TokenList tokens = {0};
    HashMap *directives = NULL;
    AstNode *program = NULL;
    int exit_code = EXIT_SUCCESS;
    double phase_start = 0.0;

    nine_stats_reset();

    if (!read_file(input_file_path, &input_file)) {
        erro("Failed to read file: %s\n", input_file_path);
        exit_code = EXIT_FAILURE;
        goto cleanup;
    }

    phase_start = nine_stats_now_ms();
    directives = preprocess(&input_file);
    nine_stats_set_preprocess_ms(nine_stats_now_ms() - phase_start);
    if (!directives) {
        erro("Preprocessing failed\n");
        exit_code = EXIT_FAILURE;
        goto cleanup;
    }

    phase_start = nine_stats_now_ms();
    if (!lex(&input_file, &tokens)) {
        erro("Lexing failed\n");
        exit_code = EXIT_FAILURE;
        goto cleanup;
    }
    nine_stats_set_lex_ms(nine_stats_now_ms() - phase_start);

    phase_start = nine_stats_now_ms();
    if (!parse(&tokens, &program)) {
        erro("Parsing failed\n");
        exit_code = EXIT_FAILURE;
        goto cleanup;
    }
    nine_stats_set_parse_ms(nine_stats_now_ms() - phase_start);

    if (!compile(program, &opts)) {
        exit_code = EXIT_FAILURE;
        goto cleanup;
    }

    nine_stats_print();

    if (opts.run && !nine_run_binary(&opts)) {
        exit_code = EXIT_FAILURE;
    }

cleanup:
    ast_release(program);
    token_list_release(&tokens);
    hm_release(directives);
    release_string(&input_file);
    return exit_code;
}
