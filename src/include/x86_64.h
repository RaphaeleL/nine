#ifndef X86_64_H
#define X86_64_H

#define QOL_STRIP_PREFIX
#include "../../libs/build.h"

#include "./parser.h"

void x86_64_compile(AstNode *program);

#endif // X86_64_H