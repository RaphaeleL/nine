#include <stdio.h>

#include "../src/include/compiler.h"
#include "../src/include/lexer.h"
#include "../src/include/parser.h"
#include "../src/include/preprocessor.h"

#define HELLO_WORLD_SOURCE "examples/000_helloworld.9"
#define HELLO_WORLD_ASM_OUT "out/test_helloworld"

static const char *const EXPECTED_HELLO_WORLD_AST =
    "PROGRAM\n"
    "  FUNCTION foo()\n"
    "    BLOCK\n"
    "      CALL print(hello)\n"
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
    "    .asciz \"hello\"\n"
    "\n"
    "Lstr1:\n"
    "    .asciz \"Hello, World!\"\n";

static void nine_unittest_init(void)
{
    static bool ready = false;
    if (ready) {
        return;
    }

    add_argument("--target", "arm64", "Set target architecture");
    add_argument("--output", NULL, "Output binary path");
    add_argument("--save-asm", NULL, "Save assembly next to output path");

    char *argv[] = {"unittests", NULL};
    init_argparser(1, argv);
    ready = true;
}

static bool nine_build_hello_world(AstNode **program_out)
{
    String input_file = {0};
    TokenList tokens = {0};
    HashMap *directives = NULL;
    AstNode *program = NULL;

    if (!read_file(HELLO_WORLD_SOURCE, &input_file)) {
        return false;
    }

    directives = preprocess(&input_file);
    if (!directives) {
        release_string(&input_file);
        return false;
    }

    if (!lex(&input_file, &tokens)) {
        hm_release(directives);
        release_string(&input_file);
        return false;
    }

    if (!parse(&tokens, &program)) {
        token_list_release(&tokens);
        hm_release(directives);
        release_string(&input_file);
        return false;
    }

    token_list_release(&tokens);
    hm_release(directives);
    release_string(&input_file);

    *program_out = program;
    return true;
}


static char *nine_join_lines(const String *lines)
{
    if (!lines || lines->len == 0) {
        return strdup("");
    }

    size_t total = 1;
    for (size_t i = 0; i < lines->len; i++) {
        total += strlen(lines->data[i]) + 1;
    }

    char *joined = calloc(total, 1);
    if (!joined) {
        return NULL;
    }

    for (size_t i = 0; i < lines->len; i++) {
        if (i > 0) {
            strcat(joined, "\n");
        }
        strcat(joined, lines->data[i]);
    }

    return joined;
}

static char *nine_read_text_file(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        return NULL;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }

    long size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        return NULL;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return NULL;
    }

    char *text = calloc((size_t)size + 1, 1);
    if (!text) {
        fclose(fp);
        return NULL;
    }

    size_t read = fread(text, 1, (size_t)size, fp);
    fclose(fp);
    text[read] = '\0';
    return text;
}

QOL_TEST(test_hello_world_ast) {
    nine_unittest_init();

    AstNode *program = NULL;
    QOL_TEST_TRUTHY(nine_build_hello_world(&program), "parse hello world example");

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
    QOL_TEST_TRUTHY(nine_build_hello_world(&program), "parse hello world example");

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
