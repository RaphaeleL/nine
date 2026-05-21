/*
 * nine — x86_64.c
 *
 * x86_64 backend stub (not yet emitting code).
 *
 * Copyright (c) 2026 Raphaele Salvatore Licciardo
 * SPDX-License-Identifier: MIT
 */

#include "./include/x86_64.h"

void x86_64_compile(AstNode *program)
{
    if (!program)
        return;

    ast_print(program, 0);
}
