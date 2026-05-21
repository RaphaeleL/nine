#define QOL_STRIP_PREFIX
#include "../libs/build.h"

#include <string.h>

#include "./include/parser.h"
#include "./include/stats.h"

const char *ast_kind_name(AstKind kind) {
    switch (kind) {
    case AST_PROGRAM:  return "PROGRAM";
    case AST_FUNCTION: return "FUNCTION";
    case AST_BLOCK:    return "BLOCK";
    case AST_CALL:     return "CALL";
    case AST_INVOKE:   return "INVOKE";
    default:           return "UNKNOWN";
    }
}

static AstNode *ast_node(AstKind kind, const char *name, const char *value) {
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

static bool ast_add_child(AstNode *parent, AstNode *child) {
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

void ast_release(AstNode *node) {
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

void ast_print(AstNode *node, size_t depth) {
    if (!node) return;

    char indent[32] = {0};
    size_t spaces = depth * 2;
    if (spaces >= sizeof(indent)) {
        spaces = sizeof(indent) - 1;
    }
    memset(indent, ' ', spaces);

    switch (node->kind) {
    case AST_FUNCTION:
        hint("%s%s %s()\n", indent, ast_kind_name(node->kind), node->name ? node->name : "");
        break;
    case AST_CALL:
        hint("%s%s %s(%s)\n", indent, ast_kind_name(node->kind), node->name ? node->name : "", node->value ? node->value : "");
        break;
    case AST_INVOKE:
        hint("%s%s %s\n", indent, ast_kind_name(node->kind), node->name ? node->name : "");
        break;
    default:
        hint("%s%s\n", indent, ast_kind_name(node->kind));
        break;
    }

    for (size_t i = 0; i < node->children_len; i++) {
        ast_print(node->children[i], depth + 1);
    }
}

typedef struct {
    TokenList *tokens;
    size_t pos;
} Parser;

static Token *parser_peek(Parser *p) {
    if (!p || !p->tokens || p->pos >= p->tokens->len) {
        return NULL;
    }
    return &p->tokens->data[p->pos];
}

static Token *parser_advance(Parser *p) {
    Token *current = parser_peek(p);
    if (current && current->kind != TOKEN_EOF) {
        p->pos++;
    }
    return current;
}

static bool parser_expect(Parser *p, TokenKind kind, const char *what) {
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

static bool parse_call(Parser *p, AstNode **out) {
    Token *name = parser_peek(p);
    if (!name || name->kind != TOKEN_IDENT) {
        return false;
    }
    parser_advance(p);

    if (!parser_expect(p, TOKEN_LPAREN, "'('")) {
        return false;
    }

    Token *arg = parser_peek(p);
    const char *arg_text = NULL;
    if (arg && arg->kind == TOKEN_STRING) {
        arg_text = arg->text;
        parser_advance(p);
    }

    if (!parser_expect(p, TOKEN_RPAREN, "')'")) {
        return false;
    }

    *out = ast_node(AST_CALL, name->text, arg_text);
    if (*out) {
        nine_stats_inc_function_calls();
    }
    return *out != NULL;
}

static bool parse_invoke(Parser *p, AstNode **out) {
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

static bool parse_statement(Parser *p, AstNode **out) {
    Token *token = parser_peek(p);
    if (token && token->kind == TOKEN_CALL) {
        return parse_invoke(p, out);
    }
    return parse_call(p, out);
}

static bool parse_block(Parser *p, AstNode **out) {
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

static bool parse_function(Parser *p, AstNode **out) {
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

bool parse(TokenList *tokens, AstNode **program) {
    if (!tokens || !program) {
        return false;
    }

    Parser p = { .tokens = tokens, .pos = 0 };
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
