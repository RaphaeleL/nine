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
    AST_NUMBER,
    AST_STRING,
    AST_BINARY,
} AstKind;

typedef enum {
    BINOP_ADD,
    BINOP_SUB,
    BINOP_MUL,
    BINOP_DIV,
    BINOP_IDIV,
    BINOP_MOD,
} BinOp;

typedef struct {
    bool is_float;
    long long i;
    double f;
} ExprValue;

typedef struct AstNode {
    AstKind kind;
    char *name;
    char *value;
    BinOp op;
    struct AstNode **children;
    size_t children_len;
} AstNode;

const char *ast_kind_name(AstKind kind);
void ast_release(AstNode *node);
void ast_print(AstNode *node, size_t depth);
void ast_format(AstNode *node, size_t depth, String *out);
bool parse(TokenList *tokens, AstNode **program);
bool expr_eval_const(AstNode *node, ExprValue *out);
char *expr_value_to_string(const ExprValue *value);
char *print_format_apply(const char *fmt, AstNode **args, size_t nargs);

#endif
