#ifndef COMPILER_H
#define COMPILER_H

#define QOL_STRIP_PREFIX
#include "../../libs/build.h"
#include "./parser.h"

typedef enum {
    TARGET_ARM64,
    TARGET_X86_64,
    TARGET_UNSUPPORTED,
} Target;

typedef struct {
    const char *input_path;
    const char *output_path;
    bool save_asm;
    bool run;
} CompileOptions;

bool compile(AstNode *program, const CompileOptions *opts);
Target get_target(void);
const char *target_name(Target target);

#endif
