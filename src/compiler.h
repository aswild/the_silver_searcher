/*
 * compiler-specific attributes and macros
 */
#ifndef COMPILER_H
#define COMPILER_H

#include "config.h"

// Default compiler attributes, applicable to gcc and clang
#if defined(__GNUC__) || defined(__clang__)
// clang-format off
#define ATTR_FORMAT_PRINTF(a, b)    __attribute__((format(printf, a, b)))
#define ATTR_NORETURN               __attribute__((noreturn))
#define STATIC_ALWAYS_INLINE        static inline __attribute__((always_inline))
// clang-format on
#else
#define ATTR_FORMAT_PRINTF(a, b)
#define ATTR_NORETURN
#define STATIC_ALWAYS_INLINE
#endif // defined(__GNUC__) || defined(__clang__)

#if defined(__MINGW32__)
// Override printf format attribute for mingw to support more than C89 printf
// Also check and make sure that we're using the mingw ANSI stdio libraries
#if !(defined(__USE_MINGW_ANSI_STDIO) && (__USE_MINGW_ANSI_STDIO == 1))
#error "On MinGW, ag must be built with __USE_MINGW_ANSI_STDIO=1"
#endif
#include <stdio.h> // for __MINGW_PRINTF_FORMAT
#undef  ATTR_FORMAT_PRINTF
#define ATTR_FORMAT_PRINTF(a, b) __attribute__((format(__MINGW_PRINTF_FORMAT, a, b)))
#endif

// disable ubsan's alignment checks for hash_strnstr which intentionally
// does unaligned access
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#if defined(__clang__) || (defined(__GNUC__) && GCC_VERSION >= 80100)
// clang and GCC 8.1 support no_sanitize("alignment")
#define ATTR_NO_SANITIZE_ALIGNMENT __attribute__((no_sanitize("alignment")))
#elif defined(__GNUC__) && GCC_VERSION >= 40900
// gcc 4.9 supports ubsan and no_sanitize_undefined
#define ATTR_NO_SANITIZE_ALIGNMENT __attribute__((no_sanitize_undefined))
#else
// older versions have no ubsan
#define NO_SANITIZE_ALIGNMENT
#endif
#undef GCC_VERSION

#endif // COMPILER_H
