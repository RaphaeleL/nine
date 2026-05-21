/*
 * nine — arm64.c
 *
 * macOS ARM64 backend: emit assembly, invoke clang, link and optionally run
 * output.
 *
 * Copyright (c) 2026 Raphaele Salvatore Licciardo
 * SPDX-License-Identifier: MIT
 */

#define QOL_STRIP_PREFIX
#include "./include/arm64.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "../libs/build.h"
#include "./include/constants.h"

static FILE *asm_out = NULL;
static String literals = {0};

static void emit(const char *fmt, ...)
{
    if (!asm_out) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(asm_out, fmt, args);
    va_end(args);
}

static void emit_header(void)
{
    emit(".global _main\n");
    emit(".align 2\n");
}

static void emit_prologue(void)
{
    emit("    stp x29, x30, [sp, #-16]!\n");
    emit("    mov x29, sp\n");
}

static void emit_epilogue(void)
{
    emit("    mov x0, #0\n");
    emit("    ldp x29, x30, [sp], #16\n");
    emit("    ret\n");
}

static void emit_print(AstNode *call)
{
    if (!call || !call->value) {
        erro("print requires a string argument\n");
        return;
    }

    char *value = strdup(call->value);
    if (!value) {
        return;
    }
    push(&literals, value);

    size_t id = literals.len - 1;
    emit("    adrp x0, Lstr%zu@PAGE\n", id);
    emit("    add x0, x0, Lstr%zu@PAGEOFF\n", id);
    emit("    bl _puts\n");
}

static void emit_builtin_call(AstNode *call)
{
    if (!call || call->kind != AST_CALL) {
        return;
    }

    if (strcmp(call->name, "print") == 0) {
        emit_print(call);
        return;
    }

    erro("Unsupported builtin: %s\n", call->name);
}

static void emit_invoke(AstNode *invoke)
{
    if (!invoke || invoke->kind != AST_INVOKE || !invoke->name) {
        return;
    }

    emit("    bl _%s\n", invoke->name);
}

static void emit_statement(AstNode *stmt)
{
    if (!stmt) {
        return;
    }

    switch (stmt->kind) {
    case AST_CALL:
        emit_builtin_call(stmt);
        break;
    case AST_INVOKE:
        emit_invoke(stmt);
        break;
    default:
        erro("Unsupported statement: %s\n", ast_kind_name(stmt->kind));
        break;
    }
}

static void emit_block(AstNode *block)
{
    if (!block || block->kind != AST_BLOCK)
        return;

    for (size_t i = 0; i < block->children_len; i++) {
        emit_statement(block->children[i]);
    }
}

static void emit_function(AstNode *function)
{
    if (!function || function->kind != AST_FUNCTION || !function->name)
        return;

    emit("\n_%s:\n", function->name);
    emit_prologue();

    if (function->children_len > 0)
        emit_block(function->children[0]);

    emit_epilogue();
}

static void emit_literals(void)
{
    for (size_t i = 0; i < literals.len; i++) {
        emit("\nLstr%zu:\n", i);
        emit("    .asciz \"%s\"\n", literals.data[i]);
    }
}

static void literals_release(void) { release_string(&literals); }

static bool arm64_emit_program(AstNode *program)
{
    if (!program || program->kind != AST_PROGRAM)
        return false;
    if (DEBUG)
        ast_print(program, 0);

    emit_header();

    for (size_t i = 0; i < program->children_len; i++) {
        AstNode *child = program->children[i];
        switch (child->kind) {
        case AST_FUNCTION:
            emit_function(child);
            break;
        default:
            erro("Unsupported AST node kind: %s\n", ast_kind_name(child->kind));
            return false;
        }
    }

    emit_literals();
    literals_release();
    return true;
}

static bool arm64_link(const char *asm_path, const char *bin_path)
{
    Cmd link = {0};
    push(&link, "clang", "-o", bin_path, asm_path);
    return run_always(&link);
}

bool arm64_compile(AstNode *program, const CompileOptions *opts)
{
    if (!program || !opts || !opts->input_path)
        return false;

    char bin_path[256] = {0};
    if (!nine_output_binary_path(opts, bin_path, sizeof(bin_path))) {
        erro("Failed to derive output name from: %s\n",
             opts->output_path ? opts->output_path : opts->input_path);
        return false;
    }

    const char *stem_source = opts->output_path ? opts->output_path : opts->input_path;
    char *base = get_filename_no_ext(stem_source);
    if (!base) {
        erro("Failed to derive output name from: %s\n", stem_source);
        return false;
    }

    char asm_path[256] = {0};

    if (opts->save_asm) {
        if (opts->output_path) {
            snprintf(asm_path, sizeof(asm_path), "%s.s", opts->output_path);
        } else {
            snprintf(asm_path, sizeof(asm_path), "./%s.s", base);
        }
    } else {
        snprintf(asm_path, sizeof(asm_path), "/tmp/%s.s", base);
    }

    free(base);

    asm_out = fopen(asm_path, "w");
    if (!asm_out) {
        erro("Failed to open assembly file: %s\n", asm_path);
        return false;
    }

    bool ok = arm64_emit_program(program);
    fclose(asm_out);
    asm_out = NULL;

    if (!ok)
        return false;

    info("Wrote assembly: %s\n", asm_path);

    if (!arm64_link(asm_path, bin_path)) {
        erro("Failed to link: %s\n", bin_path);
        return false;
    }

    info("Linked binary: %s\n", bin_path);

    return true;
}
