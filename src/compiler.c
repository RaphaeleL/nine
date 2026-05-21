#define QOL_STRIP_PREFIX
#include "../libs/build.h"

#include <string.h>

#include "./include/compiler.h"
#include "./include/arm64.h"
#include "./include/stats.h"
#include "./include/x86_64.h"

Target get_target(void) {
    arg_t *target = get_argument("--target");
    if (!target) {
        erro("No target provided\n");
        exit(EXIT_FAILURE);
    }
    if (strcmp(target->value, "arm64") == 0) {
        return TARGET_ARM64;
    }
    if (strcmp(target->value, "x86_64") == 0) {
        return TARGET_X86_64;
    }
    return TARGET_UNSUPPORTED;
}

const char *target_name(Target target) {
    const char *real_target = "n/a";
    switch (target) {
    case TARGET_ARM64:
        return "arm64";
    case TARGET_X86_64:
        return "x86_64";
    case TARGET_UNSUPPORTED:
        return real_target;
    }
}

bool compile(AstNode *program, const CompileOptions *opts) {
    if (!program || !opts || !opts->input_path) {
        return false;
    }

    double phase_start = nine_stats_now_ms();
    bool ok = false;
    Target target = get_target();

    switch (target) {
    case TARGET_ARM64:
        ok = arm64_compile(program, opts);
        break;
    case TARGET_X86_64:
        x86_64_compile(program);
        ok = true;
        break;
    case TARGET_UNSUPPORTED:
        erro("Unsupported target: %s\n", target_name(target));
        ok = false;
        break;
    }

    nine_stats_set_compile_ms(nine_stats_now_ms() - phase_start);
    return ok;
}
