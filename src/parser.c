/*
 * nine — parser.c
 *
 * Parser: builds an AST from tokens (functions, print calls, call invocations).
 *
 * Copyright (c) 2026 Raphaele Salvatore Licciardo
 * SPDX-License-Identifier: MIT
 */

#define QOL_STRIP_PREFIX
#include "./include/parser.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "../libs/build.h"
#include "./include/stats.h"

const char *ast_kind_name(AstKind kind)
{
    switch (kind) {
    case AST_PROGRAM:
        return "PROGRAM";
    case AST_FUNCTION:
        return "FUNCTION";
    case AST_BLOCK:
        return "BLOCK";
    case AST_CALL:
        return "CALL";
    case AST_INVOKE:
        return "INVOKE";
    case AST_NUMBER:
        return "NUMBER";
    case AST_STRING:
        return "STRING";
    case AST_BINARY:
        return "BINARY";
    default:
        return "UNKNOWN";
    }
}

static AstNode *ast_node(AstKind kind, const char *name, const char *value)
{
    AstNode *node = calloc(1, sizeof(AstNode));
    if (!node) {
        return NULL;
    }

    node->kind = kind;
    if (name) {
        node->name = strdup(name);
    }
    if (value) {
        node->value = strdup(value);
    }

    return node;
}

static bool ast_add_child(AstNode *parent, AstNode *child)
{
    if (!parent || !child) {
        return false;
    }

    AstNode **grown = realloc(parent->children, (parent->children_len + 1) * sizeof(AstNode *));
    if (!grown) {
        return false;
    }

    parent->children = grown;
    parent->children[parent->children_len++] = child;
    return true;
}

void ast_release(AstNode *node)
{
    if (!node) {
        return;
    }

    for (size_t i = 0; i < node->children_len; i++) {
        ast_release(node->children[i]);
    }

    free(node->children);
    free(node->name);
    free(node->value);
    free(node);
}

static void ast_format_line(String *out, const char *fmt, ...)
{
    if (!out || !fmt) {
        return;
    }

    char line[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);

    char *copy = strdup(line);
    if (copy) {
        push(out, copy);
    }
}

void ast_format(AstNode *node, size_t depth, String *out)
{
    if (!node || !out) {
        return;
    }

    char indent[32] = {0};
    size_t spaces = depth * 2;
    if (spaces >= sizeof(indent)) {
        spaces = sizeof(indent) - 1;
    }
    memset(indent, ' ', spaces);

    switch (node->kind) {
    case AST_FUNCTION:
        ast_format_line(out, "%s%s %s()", indent, ast_kind_name(node->kind),
                        node->name ? node->name : "");
        break;
    case AST_CALL:
        if (node->value) {
            ast_format_line(out, "%s%s %s(%s)", indent, ast_kind_name(node->kind),
                            node->name ? node->name : "", node->value);
        } else if (node->children_len > 0) {
            ast_format_line(out, "%s%s %s(<expr>)", indent, ast_kind_name(node->kind),
                            node->name ? node->name : "");
        } else {
            ast_format_line(out, "%s%s %s()", indent, ast_kind_name(node->kind),
                            node->name ? node->name : "");
        }
        break;
    case AST_NUMBER:
    case AST_STRING:
        ast_format_line(out, "%s%s %s", indent, ast_kind_name(node->kind),
                        node->value ? node->value : "");
        break;
    case AST_BINARY:
        ast_format_line(out, "%s%s op=%d", indent, ast_kind_name(node->kind), (int)node->op);
        break;
    case AST_INVOKE:
        ast_format_line(out, "%s%s %s", indent, ast_kind_name(node->kind),
                        node->name ? node->name : "");
        break;
    default:
        ast_format_line(out, "%s%s", indent, ast_kind_name(node->kind));
        break;
    }

    for (size_t i = 0; i < node->children_len; i++) {
        ast_format(node->children[i], depth + 1, out);
    }
}

void ast_print(AstNode *node, size_t depth)
{
    String lines = {0};
    ast_format(node, depth, &lines);
    for (size_t i = 0; i < lines.len; i++) {
        hint("%s\n", lines.data[i]);
    }
    release_string(&lines);
}

typedef struct {
    TokenList *tokens;
    size_t pos;
} Parser;

static Token *parser_peek(Parser *p)
{
    if (!p || !p->tokens || p->pos >= p->tokens->len) {
        return NULL;
    }
    return &p->tokens->data[p->pos];
}

static Token *parser_advance(Parser *p)
{
    Token *current = parser_peek(p);
    if (current && current->kind != TOKEN_EOF) {
        p->pos++;
    }
    return current;
}

static bool parser_expect(Parser *p, TokenKind kind, const char *what)
{
    Token *token = parser_peek(p);
    if (!token || token->kind != kind) {
        size_t line = token ? token->line : 0;
        erro("Expected %s at line %zu, got %s\n", what, line,
             token ? token_kind_name(token->kind) : "EOF");
        return false;
    }
    parser_advance(p);
    return true;
}

static bool parse_number(Parser *p, AstNode **out);
static bool parse_factor(Parser *p, AstNode **out);
static bool parse_term(Parser *p, AstNode **out);
static bool parse_expr(Parser *p, AstNode **out);

static AstNode *ast_binary(BinOp op, AstNode *lhs, AstNode *rhs)
{
    AstNode *node = ast_node(AST_BINARY, NULL, NULL);
    if (!node) {
        ast_release(lhs);
        ast_release(rhs);
        return NULL;
    }

    node->op = op;
    if (!ast_add_child(node, lhs) || !ast_add_child(node, rhs)) {
        ast_release(node);
        return NULL;
    }
    return node;
}

static bool parse_number(Parser *p, AstNode **out)
{
    Token *token = parser_peek(p);
    if (!token || token->kind != TOKEN_NUMBER) {
        return false;
    }
    parser_advance(p);

    *out = ast_node(AST_NUMBER, NULL, token->text);
    return *out != NULL;
}

static bool parse_factor(Parser *p, AstNode **out)
{
    Token *token = parser_peek(p);
    if (!token) {
        return false;
    }

    if (token->kind == TOKEN_NUMBER) {
        return parse_number(p, out);
    }

    if (token->kind == TOKEN_LPAREN) {
        parser_advance(p);
        if (!parse_expr(p, out)) {
            return false;
        }
        return parser_expect(p, TOKEN_RPAREN, "')'");
    }

    return false;
}

static bool parse_term(Parser *p, AstNode **out)
{
    if (!parse_factor(p, out)) {
        return false;
    }

    for (;;) {
        Token *op = parser_peek(p);
        BinOp binop;
        switch (op ? op->kind : TOKEN_EOF) {
        case TOKEN_STAR:
            binop = BINOP_MUL;
            break;
        case TOKEN_SLASH:
            binop = BINOP_DIV;
            break;
        case TOKEN_SLASHSLASH:
            binop = BINOP_IDIV;
            break;
        case TOKEN_PERCENT:
            binop = BINOP_MOD;
            break;
        default:
            return true;
        }

        parser_advance(p);

        AstNode *rhs = NULL;
        if (!parse_factor(p, &rhs)) {
            ast_release(*out);
            *out = NULL;
            return false;
        }

        AstNode *combined = ast_binary(binop, *out, rhs);
        if (!combined) {
            *out = NULL;
            return false;
        }
        *out = combined;
    }
}

static bool parse_expr(Parser *p, AstNode **out)
{
    if (!parse_term(p, out)) {
        return false;
    }

    for (;;) {
        Token *op = parser_peek(p);
        BinOp binop;
        switch (op ? op->kind : TOKEN_EOF) {
        case TOKEN_PLUS:
            binop = BINOP_ADD;
            break;
        case TOKEN_MINUS:
            binop = BINOP_SUB;
            break;
        default:
            return true;
        }

        parser_advance(p);

        AstNode *rhs = NULL;
        if (!parse_term(p, &rhs)) {
            ast_release(*out);
            *out = NULL;
            return false;
        }

        AstNode *combined = ast_binary(binop, *out, rhs);
        if (!combined) {
            *out = NULL;
            return false;
        }
        *out = combined;
    }
}

static bool parse_print_arg(Parser *p, AstNode **out)
{
    Token *arg = parser_peek(p);
    if (!arg) {
        return false;
    }

    if (arg->kind == TOKEN_STRING) {
        parser_advance(p);
        *out = ast_node(AST_STRING, NULL, arg->text);
        return *out != NULL;
    }

    return parse_expr(p, out);
}

static bool parse_call_args(Parser *p, AstNode *call)
{
    Token *arg = parser_peek(p);
    if (!arg || arg->kind == TOKEN_RPAREN) {
        return true;
    }

    if (arg->kind == TOKEN_STRING) {
        if (call->value) {
            erro("print takes only one format or string literal\n");
            return false;
        }
        call->value = strdup(arg->text);
        if (!call->value) {
            return false;
        }
        parser_advance(p);
    } else if (arg->kind == TOKEN_NUMBER || arg->kind == TOKEN_LPAREN) {
        AstNode *expr = NULL;
        if (!parse_expr(p, &expr)) {
            return false;
        }
        if (!ast_add_child(call, expr)) {
            ast_release(expr);
            return false;
        }
    } else {
        erro("Expected string or expression argument\n");
        return false;
    }

    while (parser_peek(p) && parser_peek(p)->kind == TOKEN_COMMA) {
        parser_advance(p);

        AstNode *arg_node = NULL;
        if (!parse_print_arg(p, &arg_node)) {
            return false;
        }
        if (!ast_add_child(call, arg_node)) {
            ast_release(arg_node);
            return false;
        }
    }

    return true;
}

static bool parse_call(Parser *p, AstNode **out)
{
    Token *name = parser_peek(p);
    if (!name || name->kind != TOKEN_IDENT) {
        return false;
    }
    parser_advance(p);

    if (!parser_expect(p, TOKEN_LPAREN, "'('")) {
        return false;
    }

    *out = ast_node(AST_CALL, name->text, NULL);
    if (!*out) {
        return false;
    }

    if (!parse_call_args(p, *out)) {
        ast_release(*out);
        *out = NULL;
        return false;
    }

    if (!parser_expect(p, TOKEN_RPAREN, "')'")) {
        ast_release(*out);
        *out = NULL;
        return false;
    }

    if (!(*out)->value && (*out)->children_len == 0) {
        erro("print requires at least one argument\n");
        ast_release(*out);
        *out = NULL;
        return false;
    }

    nine_stats_inc_function_calls();
    return true;
}

char *expr_value_to_string(const ExprValue *value)
{
    if (!value) {
        return NULL;
    }

    char buf[64];
    if (value->is_float) {
        snprintf(buf, sizeof(buf), "%g", value->f);
    } else {
        snprintf(buf, sizeof(buf), "%lld", (long long)value->i);
    }
    return strdup(buf);
}

char *print_format_apply(const char *fmt, AstNode **args, size_t nargs)
{
    if (!fmt) {
        return NULL;
    }

    if (!strchr(fmt, '@')) {
        if (nargs > 0) {
            erro("print format has no '@' but arguments were given\n");
            return NULL;
        }
        return strdup(fmt);
    }

    char *result = calloc(1, 1);
    if (!result) {
        return NULL;
    }

    size_t arg_i = 0;
    for (const char *p = fmt; *p; p++) {
        if (*p != '@') {
            size_t len = strlen(result);
            char *grown = realloc(result, len + 2);
            if (!grown) {
                free(result);
                return NULL;
            }
            result = grown;
            result[len] = *p;
            result[len + 1] = '\0';
            continue;
        }

        if (arg_i >= nargs) {
            erro("print format has more '@' placeholders than arguments\n");
            free(result);
            return NULL;
        }

        p++;
        bool want_string = (*p == 's');
        bool want_number = (*p == 'd');
        if (want_string || want_number) {
            p++;
        }

        char *subst = NULL;
        if (want_string) {
            if (args[arg_i]->kind != AST_STRING || !args[arg_i]->value) {
                erro("print @s requires a string argument\n");
                free(result);
                return NULL;
            }
            subst = strdup(args[arg_i]->value);
        } else if (want_number) {
            ExprValue value = {0};
            if (!expr_eval_const(args[arg_i], &value)) {
                erro("print @d requires a constant expression\n");
                free(result);
                return NULL;
            }
            subst = expr_value_to_string(&value);
        } else if (args[arg_i]->kind == AST_STRING && args[arg_i]->value) {
            subst = strdup(args[arg_i]->value);
        } else {
            ExprValue value = {0};
            if (!expr_eval_const(args[arg_i], &value)) {
                erro("print format argument must be a string or constant expression\n");
                free(result);
                return NULL;
            }
            subst = expr_value_to_string(&value);
        }

        if (!subst) {
            free(result);
            return NULL;
        }

        char *grown = realloc(result, strlen(result) + strlen(subst) + 1);
        if (!grown) {
            free(subst);
            free(result);
            return NULL;
        }
        result = grown;
        strcat(result, subst);
        free(subst);
        arg_i++;
    }

    if (arg_i != nargs) {
        erro("print has more arguments than '@' placeholders in format string\n");
        free(result);
        return NULL;
    }

    return result;
}

bool expr_eval_const(AstNode *node, ExprValue *out)
{
    if (!node || !out) {
        return false;
    }

    memset(out, 0, sizeof(*out));

    if (node->kind == AST_NUMBER) {
        if (!node->value) {
            return false;
        }
        out->is_float = false;
        out->i = strtoll(node->value, NULL, 10);
        return true;
    }

    if (node->kind != AST_BINARY || node->children_len != 2) {
        return false;
    }

    ExprValue lhs = {0};
    ExprValue rhs = {0};
    if (!expr_eval_const(node->children[0], &lhs) || !expr_eval_const(node->children[1], &rhs)) {
        return false;
    }

    if (lhs.is_float || rhs.is_float) {
        double l = lhs.is_float ? lhs.f : (double)lhs.i;
        double r = rhs.is_float ? rhs.f : (double)rhs.i;
        double result = 0.0;

        switch (node->op) {
        case BINOP_ADD:
            result = l + r;
            break;
        case BINOP_SUB:
            result = l - r;
            break;
        case BINOP_MUL:
            result = l * r;
            break;
        case BINOP_DIV:
            result = l / r;
            break;
        case BINOP_IDIV:
            result = (double)((long long)(l / r));
            break;
        case BINOP_MOD:
            result = (double)((long long)l % (long long)r);
            break;
        }

        out->is_float = true;
        out->f = result;
        return true;
    }

    long long l = lhs.i;
    long long r = rhs.i;
    long long iresult = 0;

    switch (node->op) {
    case BINOP_ADD:
        iresult = l + r;
        break;
    case BINOP_SUB:
        iresult = l - r;
        break;
    case BINOP_MUL:
        iresult = l * r;
        break;
    case BINOP_DIV:
        out->is_float = true;
        out->f = (double)l / (double)r;
        return true;
    case BINOP_IDIV:
        if (r == 0) {
            return false;
        }
        iresult = l / r;
        break;
    case BINOP_MOD:
        if (r == 0) {
            return false;
        }
        iresult = l % r;
        break;
    }

    out->is_float = false;
    out->i = iresult;
    return true;
}

static bool parse_invoke(Parser *p, AstNode **out)
{
    if (!parser_expect(p, TOKEN_CALL, "'call'")) {
        return false;
    }

    Token *name = parser_peek(p);
    if (!name || name->kind != TOKEN_IDENT) {
        erro("Expected function name after 'call'\n");
        return false;
    }
    parser_advance(p);

    *out = ast_node(AST_INVOKE, name->text, NULL);
    if (*out) {
        nine_stats_inc_function_calls();
    }
    return *out != NULL;
}

static bool parse_statement(Parser *p, AstNode **out)
{
    Token *token = parser_peek(p);
    if (token && token->kind == TOKEN_CALL) {
        return parse_invoke(p, out);
    }
    return parse_call(p, out);
}

static bool parse_block(Parser *p, AstNode **out)
{
    *out = ast_node(AST_BLOCK, NULL, NULL);
    if (!*out) {
        return false;
    }

    while (parser_peek(p) && parser_peek(p)->kind != TOKEN_END) {
        AstNode *stmt = NULL;
        if (!parse_statement(p, &stmt)) {
            Token *token = parser_peek(p);
            erro("Expected statement at line %zu\n", token ? token->line : 0);
            ast_release(*out);
            *out = NULL;
            return false;
        }

        if (!ast_add_child(*out, stmt)) {
            ast_release(stmt);
            ast_release(*out);
            *out = NULL;
            return false;
        }
    }

    return true;
}

static bool parse_function(Parser *p, AstNode **out)
{
    if (!parser_expect(p, TOKEN_DEFINE, "'define'")) {
        return false;
    }

    Token *name = parser_peek(p);
    if (!name || name->kind != TOKEN_IDENT) {
        erro("Expected function name after 'define'\n");
        return false;
    }
    parser_advance(p);

    if (!parser_expect(p, TOKEN_LPAREN, "'('")) {
        return false;
    }
    if (!parser_expect(p, TOKEN_RPAREN, "')'")) {
        return false;
    }
    if (!parser_expect(p, TOKEN_START, "'start'")) {
        return false;
    }

    AstNode *body = NULL;
    if (!parse_block(p, &body)) {
        return false;
    }

    if (!parser_expect(p, TOKEN_END, "'end'")) {
        ast_release(body);
        return false;
    }

    *out = ast_node(AST_FUNCTION, name->text, NULL);
    if (!*out) {
        ast_release(body);
        return false;
    }

    if (!ast_add_child(*out, body)) {
        ast_release(*out);
        *out = NULL;
        return false;
    }

    return true;
}

bool parse(TokenList *tokens, AstNode **program)
{
    if (!tokens || !program) {
        return false;
    }

    Parser p = {.tokens = tokens, .pos = 0};
    *program = ast_node(AST_PROGRAM, NULL, NULL);
    if (!*program) {
        return false;
    }

    while (parser_peek(&p) && parser_peek(&p)->kind != TOKEN_EOF) {
        AstNode *function = NULL;
        if (!parse_function(&p, &function)) {
            ast_release(*program);
            *program = NULL;
            return false;
        }

        if (!ast_add_child(*program, function)) {
            ast_release(function);
            ast_release(*program);
            *program = NULL;
            return false;
        }
    }

    if (!parser_expect(&p, TOKEN_EOF, "end of file")) {
        ast_release(*program);
        *program = NULL;
        return false;
    }

    return true;
}
