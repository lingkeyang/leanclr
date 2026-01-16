#pragma once

#include <stdint.h>

typedef uint8_t byte;
typedef int8_t sbyte;

#define LEANCLR_SUPPORT_UNALIGNED_ACCESS 1

#if UINTPTR_MAX == 0xFFFFFFFF
#define LEANCLR_ARCH_32BIT 1
#else
#define LEANCLR_ARCH_64BIT 1
#endif

#if defined(__GNUC__) || defined(__clang__)
// #define LEANCLR_USE_COMPUTED_GOTO_DISPATCHER 1
#define LEANCLR_USE_COMPUTED_GOTO_DISPATCHER 1
#else
#define LEANCLR_USE_COMPUTED_GOTO_DISPATCHER 1
#endif

#if !NDEBUG
#define LEANCLR_ENABLE_TEST_PINVOKES 1
#define LEANCLR_ENABLE_TEST_INTRINSICS 1
#define LEANCLR_ENABLE_TEST_INTERNAL_CALLS 1
#endif

#if !NDEBUG
#ifndef LEANCLR_ENABLE_FRAME_TRACE
#define LEANCLR_ENABLE_FRAME_TRACE 1
#endif
#endif

#define LEANCLR_NO_EXCEPTION noexcept