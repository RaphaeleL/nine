/*
 * nine — preprocessor.h
 *
 * Preprocess() entry: returns directive hash map, mutates source in place.
 *
 * Copyright (c) 2026 Raphaele Salvatore Licciardo
 * SPDX-License-Identifier: MIT
 */

#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#define QOL_STRIP_PREFIX
#include "../../libs/build.h"

HashMap *preprocess(String *source);

#endif
