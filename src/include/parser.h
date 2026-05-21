/*
 * nine — parser.h
 *
 * AST node types, tree utilities, and parse() from a token stream.
 *
 * Copyright (c) 2026 Raphaele Salvatore Licciardo
 * SPDX-License-Identifier: MIT
 */

#ifndef PARSER_H
#define PARSER_H

#define QOL_STRIP_PREFIX
#include "../../libs/build.h"
#include "./lexer.h"

typedef enum {
    AST_PROGRAM,
    AST_FUNCTION,
    AST_BLOCK,
    AST_CALL,
    AST_INVOKE,
} AstKind;

typedef struct AstNode {
    AstKind kind;
    char *name;
    char *value;
    struct AstNode **children;
    size_t children_len;
} AstNode;

const char *ast_kind_name(AstKind kind);
void ast_release(AstNode *node);
void ast_print(AstNode *node, size_t depth);
bool parse(TokenList *tokens, AstNode **program);

#endif
