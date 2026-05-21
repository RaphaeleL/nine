/*
 * nine — x86_64.h
 *
 * x86_64 code generator interface (stub).
 *
 * Copyright (c) 2026 Raphaele Salvatore Licciardo
 * SPDX-License-Identifier: MIT
 */

#ifndef X86_64_H
#define X86_64_H

#define QOL_STRIP_PREFIX
#include "../../libs/build.h"
#include "./parser.h"

void x86_64_compile(AstNode *program);

#endif // X86_64_H
