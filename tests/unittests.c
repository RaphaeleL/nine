/*
 * nine — unittests.c
 *
 * Unit test runner: registers and executes QOL tests from test headers.
 *
 * Copyright (c) 2026 Raphaele Salvatore Licciardo
 * SPDX-License-Identifier: MIT
 */

#define QOL_IMPLEMENTATION
#define QOL_STRIP_PREFIX
#include "../libs/build.h"

#include "test_arithmetic.h"
#include "test_helloworld.h"

int main() {
    init_logger(.only=LOG_HINT, .only_set=true, .time=true, .color=true);
    return test_run_all();
}

