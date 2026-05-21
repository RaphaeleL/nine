/*
 * nine — test_common.h
 *
 * Shared helpers for golden unit tests.
 *
 * Copyright (c) 2026 Raphaele Salvatore Licciardo
 * SPDX-License-Identifier: MIT
 */

#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <stdio.h>

#include "../src/include/lexer.h"
#include "../src/include/parser.h"
#include "../src/include/preprocessor.h"

static void nine_unittest_init(void)
{
    static bool ready = false;
    if (ready) {
        return;
    }

    add_argument("--target", "arm64", "Set target architecture");
    add_argument("--output", NULL, "Output binary path");
    add_argument("--save-asm", NULL, "Save assembly next to output path");

    char *argv[] = {"unittests", NULL};
    init_argparser(1, argv);
    ready = true;
}

static bool nine_build_source(const char *path, AstNode **program_out)
{
    String input_file = {0};
    TokenList tokens = {0};
    HashMap *directives = NULL;
    AstNode *program = NULL;

    if (!read_file(path, &input_file)) {
        return false;
    }

    directives = preprocess(&input_file);
    if (!directives) {
        release_string(&input_file);
        return false;
    }

    if (!lex(&input_file, &tokens)) {
        hm_release(directives);
        release_string(&input_file);
        return false;
    }

    if (!parse(&tokens, &program)) {
        token_list_release(&tokens);
        hm_release(directives);
        release_string(&input_file);
        return false;
    }

    token_list_release(&tokens);
    hm_release(directives);
    release_string(&input_file);

    *program_out = program;
    return true;
}

static char *nine_join_lines(const String *lines)
{
    if (!lines || lines->len == 0) {
        return strdup("");
    }

    size_t total = 1;
    for (size_t i = 0; i < lines->len; i++) {
        total += strlen(lines->data[i]) + 1;
    }

    char *joined = calloc(total, 1);
    if (!joined) {
        return NULL;
    }

    for (size_t i = 0; i < lines->len; i++) {
        if (i > 0) {
            strcat(joined, "\n");
        }
        strcat(joined, lines->data[i]);
    }

    return joined;
}

static char *nine_read_text_file(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        return NULL;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }

    long size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        return NULL;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return NULL;
    }

    char *text = calloc((size_t)size + 1, 1);
    if (!text) {
        fclose(fp);
        return NULL;
    }

    size_t read = fread(text, 1, (size_t)size, fp);
    fclose(fp);
    text[read] = '\0';
    return text;
}

#endif
