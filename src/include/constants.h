/*
 * nine — constants.h
 *
 * Version macros, debug/stats toggles, and stats warning thresholds.
 *
 * Copyright (c) 2026 Raphaele Salvatore Licciardo
 * SPDX-License-Identifier: MIT
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

#define QOL_STRIP_PREFIX
#include "../../libs/build.h"

#define MAJOR_VERSION 0
#define MINOR_VERSION 0
#define PATCH_VERSION 1

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define VERSION "v" TOSTRING(MAJOR_VERSION) "." TOSTRING(MINOR_VERSION) "." TOSTRING(PATCH_VERSION)

#define DEBUG 0
#define STATS 1

#if STATS
#define STATS_PHASE_WARN_MS 50.0
#define STATS_COMPILE_WARN_MS 250.0
#define STATS_TOTAL_WARN_MS 250.0
#endif

#endif
