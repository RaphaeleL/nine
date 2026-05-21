#ifndef ARM64_H
#define ARM64_H

#define QOL_STRIP_PREFIX
#include "../../libs/build.h"

#include "./compiler.h"
#include "./parser.h"

bool arm64_compile(AstNode *program, const CompileOptions *opts);

#endif // ARM64_H
