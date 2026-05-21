/*
 * nine — test_helloworld.h
 *
 * Golden tests for examples/000_helloworld.9: expected AST and ARM64 assembly.
 *
 * Copyright (c) 2026 Raphaele Salvatore Licciardo
 * SPDX-License-Identifier: MIT
 */

#include "../src/include/compiler.h"
#include "test_common.h"

#define HELLO_WORLD_SOURCE "examples/000_helloworld.9"
#define HELLO_WORLD_ASM_OUT "out/test_helloworld"

static const char *const EXPECTED_HELLO_WORLD_AST =
    "PROGRAM\n"
    "  FUNCTION foo()\n"
    "    BLOCK\n"
    "      CALL print(hello @)\n"
    "        STRING world\n"
    "  FUNCTION main()\n"
    "    BLOCK\n"
    "      CALL print(Hello, World!)\n"
    "      INVOKE foo";

static const char *const EXPECTED_HELLO_WORLD_ASM =
    ".global _main\n"
    ".align 2\n"
    "\n"
    "_foo:\n"
    "    stp x29, x30, [sp, #-16]!\n"
    "    mov x29, sp\n"
    "    adrp x0, Lstr0@PAGE\n"
    "    add x0, x0, Lstr0@PAGEOFF\n"
    "    bl _puts\n"
    "    mov x0, #0\n"
    "    ldp x29, x30, [sp], #16\n"
    "    ret\n"
    "\n"
    "_main:\n"
    "    stp x29, x30, [sp, #-16]!\n"
    "    mov x29, sp\n"
    "    adrp x0, Lstr1@PAGE\n"
    "    add x0, x0, Lstr1@PAGEOFF\n"
    "    bl _puts\n"
    "    bl _foo\n"
    "    mov x0, #0\n"
    "    ldp x29, x30, [sp], #16\n"
    "    ret\n"
    "\n"
    "Lstr0:\n"
    "    .asciz \"hello world\"\n"
    "\n"
    "Lstr1:\n"
    "    .asciz \"Hello, World!\"\n";

QOL_TEST(test_hello_world_ast) {
    nine_unittest_init();

    AstNode *program = NULL;
    QOL_TEST_TRUTHY(nine_build_source(HELLO_WORLD_SOURCE, &program), "parse hello world example");

    String ast_lines = {0};
    ast_format(program, 0, &ast_lines);
    char *ast_text = nine_join_lines(&ast_lines);
    release_string(&ast_lines);
    ast_release(program);

    QOL_TEST_TRUTHY(ast_text, "format hello world AST");
    QOL_TEST_STREQ(ast_text, EXPECTED_HELLO_WORLD_AST, "hello world AST");
    free(ast_text);
}

QOL_TEST(test_hello_world_asm) {
    nine_unittest_init();

    AstNode *program = NULL;
    QOL_TEST_TRUTHY(nine_build_source(HELLO_WORLD_SOURCE, &program), "parse hello world example");

    CompileOptions opts = {
        .input_path = HELLO_WORLD_SOURCE,
        .output_path = HELLO_WORLD_ASM_OUT,
        .save_asm = true,
        .run = false,
    };

    QOL_TEST_TRUTHY(compile(program, &opts), "compile hello world to assembly");
    ast_release(program);

    char asm_path[256] = {0};
    snprintf(asm_path, sizeof(asm_path), "%s.s", HELLO_WORLD_ASM_OUT);

    char *asm_text = nine_read_text_file(asm_path);
    QOL_TEST_TRUTHY(asm_text, "read generated assembly");
    QOL_TEST_STREQ(asm_text, EXPECTED_HELLO_WORLD_ASM, "hello world assembly");
    free(asm_text);
}
