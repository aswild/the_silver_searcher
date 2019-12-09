# aw_flags.m4
#
# Autoconf macros to split CFLAGS/CPPFLAGS, CFLAGS/CPPFLAGS, or LDFLAGS/LIBS.
#
# For CFLAGS/CPPFLAGS (or CXXFLAGS/CPPFLAGS), all words which begin with -I or -D
# are considered preprocessor flags, everything else goes into CFLAGS or CXXFLAGS.
#
# Likewise, words which begin with -l are considered LIBS and everything else is
# LDFLAGS. Note this will not work properly when the ordering of libs and other flags,
# matters, as sometimes occurs with -Wl,--as-needed or -Wl,--whole-archive
#
# Copyright (c) 2018 Allen Wild <allenwild93@gmail.com>
# SPDX-License-Identifier: Apache-2.0

# AW_APPEND_CFLAGS(flags...)
# ------------------------------------------------
# For each flag, append it to CPPFLAGS if it begins
# with -D, -U, or -I. Otherwise append it to CFLAGS.
# AX_APPEND_FLAG is used to skip duplicates
AC_DEFUN([AW_APPEND_CFLAGS], [
    for flag in $1; do
        AS_CASE([$flag], [-I*|-D*|-U*], [AX_APPEND_FLAG([$flag], [CPPFLAGS])],
                [AX_APPEND_FLAG([$flag], [CFLAGS])])
    done
])dnl

# AW_APPEND_LIBS(flags...)
# ------------------------------------------------
# For each flag, append it to LIBS if it begins
# with -l. Otherwise append it to LDFLAGS.
# AX_APPEND_FLAG is used to skip duplicates
AC_DEFUN([AW_APPEND_LIBS], [
    for flag in $1; do
        AS_CASE([$flag], [-l*], [AX_APPEND_FLAG([$flag], [LIBS])],
                [AX_APPEND_FLAG([$flag], [LDFLAGS])])
    done
])dnl

# AW_ADD_CFLAGS_VERDEP(FLAG)
# ------------------------------------------------
# Check whether FLAG is supported by the compiler using
# AX_CHECK_COMPILE_FLAG, and add to CFLAGS_VERDEP if so.
AC_DEFUN_ONCE([_AW_SUBST_CFLAGS_VERDEP], [
    AC_SUBST([CFLAGS_VERDEP])
    AM_SUBST_NOTMAKE([CFLAGS_VERDEP])
])dnl
AC_DEFUN([AW_ADD_CFLAGS_VERDEP], [
    AX_CHECK_COMPILE_FLAG([$1], [AX_APPEND_FLAG([$1], [CFLAGS_VERDEP])])
    _AW_SUBST_CFLAGS_VERDEP
])dnl

# AW_STRIP_FLAGS(variable-name)
# ------------------------------------------------
# Strip leading/trailing spaces from the given variable
# and squash repeated spaces into a single space.
# Similar to $(strip ...) in a GNU Makefile
AC_DEFUN([AW_STRIP_FLAGS], [
    AC_REQUIRE([AC_PROG_SED])dnl
    AS_VAR_PUSHDEF([FLAGS], [$1])dnl
    AS_VAR_SET(FLAGS, [`AS_ECHO_N("[AS_VAR_GET(FLAGS)]") | $SED 's/^ *//;s/ *$//;s/ \+/ /g'`])
    AS_VAR_POPDEF([FLAGS])dnl
])dnl
