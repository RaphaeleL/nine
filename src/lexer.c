/*
 * nine — lexer.c
 *
 * Lexical analyzer: turns preprocessed source lines into a token stream.
 *
 * Copyright (c) 2026 Raphaele Salvatore Licciardo
 * SPDX-License-Identifier: MIT
 */

#define QOL_STRIP_PREFIX
#include "./include/lexer.h"

#include <ctype.h>
#include <string.h>

#include "../libs/build.h"

const char *token_kind_name(TokenKind kind)
{
    switch (kind) {
    case TOKEN_EOF:
        return "EOF";
    case TOKEN_DEFINE:
        return "DEFINE";
    case TOKEN_CALL:
        return "CALL";
    case TOKEN_IDENT:
        return "IDENT";
    case TOKEN_STRING:
        return "STRING";
    case TOKEN_LPAREN:
        return "LPAREN";
    case TOKEN_RPAREN:
        return "RPAREN";
    case TOKEN_NUMBER:
        return "NUMBER";
    case TOKEN_START:
        return "START";
    case TOKEN_END:
        return "END";
    case TOKEN_PLUS:
        return "PLUS";
    case TOKEN_MINUS:
        return "MINUS";
    case TOKEN_STAR:
        return "STAR";
    case TOKEN_SLASH:
        return "SLASH";
    case TOKEN_SLASHSLASH:
        return "SLASHSLASH";
    case TOKEN_PERCENT:
        return "PERCENT";
    case TOKEN_COMMA:
        return "COMMA";
    default:
        return "UNKNOWN";
    }
}

void token_list_release(TokenList *tokens)
{
    if (!tokens || !tokens->data) {
        return;
    }

    for (size_t i = 0; i < tokens->len; i++) {
        free(tokens->data[i].text);
    }
    release(tokens);
}

static void push_token(TokenList *tokens, TokenKind kind, const char *text, size_t line)
{
    Token token = {
        .kind = kind,
        .text = text ? strdup(text) : NULL,
        .line = line,
    };
    push(tokens, token);
}

static bool lex_identifier(const char *line, size_t *i, TokenList *tokens, size_t line_no)
{
    size_t start = *i;
    while (line[*i] && (isalnum((unsigned char)line[*i]) || line[*i] == '_')) {
        (*i)++;
    }

    size_t len = *i - start;
    char *text = strndup(line + start, len);
    if (!text) {
        return false;
    }

    if (strcmp(text, "define") == 0) {
        free(text);
        push_token(tokens, TOKEN_DEFINE, NULL, line_no);
    } else if (strcmp(text, "call") == 0) {
        free(text);
        push_token(tokens, TOKEN_CALL, NULL, line_no);
    } else if (strcmp(text, "start") == 0) {
        free(text);
        push_token(tokens, TOKEN_START, NULL, line_no);
    } else if (strcmp(text, "end") == 0) {
        free(text);
        push_token(tokens, TOKEN_END, NULL, line_no);
    } else {
        push_token(tokens, TOKEN_IDENT, text, line_no);
        free(text);
    }

    return true;
}

static bool lex_number(const char *line, size_t *i, TokenList *tokens, size_t line_no)
{
    size_t start = *i;
    while (line[*i] && isdigit((unsigned char)line[*i])) {
        (*i)++;
    }

    char *text = strndup(line + start, *i - start);
    if (!text) {
        return false;
    }

    push_token(tokens, TOKEN_NUMBER, text, line_no);
    free(text);
    return true;
}

static bool lex_string(const char *line, size_t *i, TokenList *tokens, size_t line_no)
{
    (*i)++;
    size_t start = *i;

    while (line[*i] && line[*i] != '"') {
        (*i)++;
    }

    if (line[*i] != '"') {
        erro("Unterminated string on line %zu\n", line_no);
        return false;
    }

    char *text = strndup(line + start, *i - start);
    if (!text) {
        return false;
    }

    push_token(tokens, TOKEN_STRING, text, line_no);
    free(text);
    (*i)++;
    return true;
}

static bool lex_line(const char *line, size_t line_no, TokenList *tokens)
{
    if (!line) {
        return true;
    }

    size_t i = 0;
    while (line[i]) {
        char c = line[i];

        if (isspace((unsigned char)c)) {
            i++;
            continue;
        }

        switch (c) {
        case '(':
            push_token(tokens, TOKEN_LPAREN, NULL, line_no);
            i++;
            break;
        case ')':
            push_token(tokens, TOKEN_RPAREN, NULL, line_no);
            i++;
            break;
        case '+':
            push_token(tokens, TOKEN_PLUS, NULL, line_no);
            i++;
            break;
        case '-':
            push_token(tokens, TOKEN_MINUS, NULL, line_no);
            i++;
            break;
        case '*':
            push_token(tokens, TOKEN_STAR, NULL, line_no);
            i++;
            break;
        case '/':
            if (line[i + 1] == '/') {
                push_token(tokens, TOKEN_SLASHSLASH, NULL, line_no);
                i += 2;
            } else {
                push_token(tokens, TOKEN_SLASH, NULL, line_no);
                i++;
            }
            break;
        case '%':
            push_token(tokens, TOKEN_PERCENT, NULL, line_no);
            i++;
            break;
        case ',':
            push_token(tokens, TOKEN_COMMA, NULL, line_no);
            i++;
            break;
        case '"':
            if (!lex_string(line, &i, tokens, line_no))
                return false;
            break;
        default:
            if (isdigit((unsigned char)c)) {
                if (!lex_number(line, &i, tokens, line_no)) {
                    return false;
                }
            } else if (isalpha((unsigned char)c) || c == '_') {
                if (!lex_identifier(line, &i, tokens, line_no)) {
                    return false;
                }
            } else {
                erro("Unexpected character '%c' on line %zu\n", c, line_no);
                return false;
            }
            break;
        }
    }

    return true;
}

bool lex(String *source, TokenList *tokens)
{
    if (!source || !tokens) {
        return false;
    }

    tokens->data = NULL;
    tokens->len = 0;
    tokens->cap = 0;

    for (size_t n = 0; n < source->len; n++) {
        if (!lex_line(source->data[n], n + 1, tokens)) {
            token_list_release(tokens);
            return false;
        }
    }

    push_token(tokens, TOKEN_EOF, NULL, source->len + 1);
    return true;
}
