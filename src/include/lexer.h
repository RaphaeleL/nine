/*
 * nine — lexer.h
 *
 * Token kinds, token list, and lex() for preprocessed source.
 *
 * Copyright (c) 2026 Raphaele Salvatore Licciardo
 * SPDX-License-Identifier: MIT
 */

#ifndef LEXER_H
#define LEXER_H

#define QOL_STRIP_PREFIX
#include "../../libs/build.h"

typedef enum {
    TOKEN_EOF,
    TOKEN_DEFINE,
    TOKEN_CALL,
    TOKEN_IDENT,
    TOKEN_STRING,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_START,
    TOKEN_END,
} TokenKind;

typedef struct {
    TokenKind kind;
    char *text;
    size_t line;
} Token;

typedef struct {
    Token *data;
    size_t len;
    size_t cap;
} TokenList;

const char *token_kind_name(TokenKind kind);
void token_list_release(TokenList *tokens);
bool lex(String *source, TokenList *tokens);

#endif
