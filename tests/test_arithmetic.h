/*
 * nine — test_arithmetic.h
 *
 * Golden tests for examples/001_arithmetic.9: expected AST and ARM64 assembly.
 *
 * Copyright (c) 2026 Raphaele Salvatore Licciardo
 * SPDX-License-Identifier: MIT
 */

#include "../src/include/compiler.h"
#include "test_common.h"

#define ARITHMETIC_SOURCE "examples/001_arithmetic.9"
#define ARITHMETIC_ASM_OUT "out/test_arithmetic"

static const char *const EXPECTED_ARITHMETIC_AST =
    "PROGRAM\n"
    "  FUNCTION main()\n"
    "    BLOCK\n"
    "      CALL print(1+1=@)\n"
    "        BINARY op=0\n"
    "          NUMBER 1\n"
    "          NUMBER 1\n"
    "      CALL print(3-1=@)\n"
    "        BINARY op=1\n"
    "          NUMBER 3\n"
    "          NUMBER 1\n"
    "      CALL print(2*2=@)\n"
    "        BINARY op=2\n"
    "          NUMBER 2\n"
    "          NUMBER 2\n"
    "      CALL print(10/2=@)\n"
    "        BINARY op=3\n"
    "          NUMBER 10\n"
    "          NUMBER 2\n"
    "      CALL print(10/3=@)\n"
    "        BINARY op=3\n"
    "          NUMBER 10\n"
    "          NUMBER 3\n"
    "      CALL print(10//3=@)\n"
    "        BINARY op=4\n"
    "          NUMBER 10\n"
    "          NUMBER 3\n"
    "      CALL print(6%2=@)\n"
    "        BINARY op=5\n"
    "          NUMBER 6\n"
    "          NUMBER 2";

static const char *const EXPECTED_ARITHMETIC_ASM =
    ".global _main\n"
    ".align 2\n"
    "\n"
    "_main:\n"
    "    stp x29, x30, [sp, #-16]!\n"
    "    mov x29, sp\n"
    "    adrp x0, Lstr0@PAGE\n"
    "    add x0, x0, Lstr0@PAGEOFF\n"
    "    bl _puts\n"
    "    adrp x0, Lstr1@PAGE\n"
    "    add x0, x0, Lstr1@PAGEOFF\n"
    "    bl _puts\n"
    "    adrp x0, Lstr2@PAGE\n"
    "    add x0, x0, Lstr2@PAGEOFF\n"
    "    bl _puts\n"
    "    adrp x0, Lstr3@PAGE\n"
    "    add x0, x0, Lstr3@PAGEOFF\n"
    "    bl _puts\n"
    "    adrp x0, Lstr4@PAGE\n"
    "    add x0, x0, Lstr4@PAGEOFF\n"
    "    bl _puts\n"
    "    adrp x0, Lstr5@PAGE\n"
    "    add x0, x0, Lstr5@PAGEOFF\n"
    "    bl _puts\n"
    "    adrp x0, Lstr6@PAGE\n"
    "    add x0, x0, Lstr6@PAGEOFF\n"
    "    bl _puts\n"
    "    mov x0, #0\n"
    "    ldp x29, x30, [sp], #16\n"
    "    ret\n"
    "\n"
    "Lstr0:\n"
    "    .asciz \"1+1=2\"\n"
    "\n"
    "Lstr1:\n"
    "    .asciz \"3-1=2\"\n"
    "\n"
    "Lstr2:\n"
    "    .asciz \"2*2=4\"\n"
    "\n"
    "Lstr3:\n"
    "    .asciz \"10/2=5\"\n"
    "\n"
    "Lstr4:\n"
    "    .asciz \"10/3=3.33333\"\n"
    "\n"
    "Lstr5:\n"
    "    .asciz \"10//3=3\"\n"
    "\n"
    "Lstr6:\n"
    "    .asciz \"6%2=0\"\n";

QOL_TEST(test_arithmetic_ast) {
    nine_unittest_init();

    AstNode *program = NULL;
    QOL_TEST_TRUTHY(nine_build_source(ARITHMETIC_SOURCE, &program), "parse arithmetic example");

    String ast_lines = {0};
    ast_format(program, 0, &ast_lines);
    char *ast_text = nine_join_lines(&ast_lines);
    release_string(&ast_lines);
    ast_release(program);

    QOL_TEST_TRUTHY(ast_text, "format arithmetic AST");
    QOL_TEST_STREQ(ast_text, EXPECTED_ARITHMETIC_AST, "arithmetic AST");
    free(ast_text);
}

QOL_TEST(test_arithmetic_asm) {
    nine_unittest_init();

    AstNode *program = NULL;
    QOL_TEST_TRUTHY(nine_build_source(ARITHMETIC_SOURCE, &program), "parse arithmetic example");

    CompileOptions opts = {
        .input_path = ARITHMETIC_SOURCE,
        .output_path = ARITHMETIC_ASM_OUT,
        .save_asm = true,
        .run = false,
    };

    QOL_TEST_TRUTHY(compile(program, &opts), "compile arithmetic to assembly");
    ast_release(program);

    char asm_path[256] = {0};
    snprintf(asm_path, sizeof(asm_path), "%s.s", ARITHMETIC_ASM_OUT);

    char *asm_text = nine_read_text_file(asm_path);
    QOL_TEST_TRUTHY(asm_text, "read generated assembly");
    QOL_TEST_STREQ(asm_text, EXPECTED_ARITHMETIC_ASM, "arithmetic assembly");
    free(asm_text);
}
