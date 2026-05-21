#include "./include/x86_64.h"

void x86_64_compile(AstNode *program) {
    if (!program) return;

    ast_print(program, 0);

}