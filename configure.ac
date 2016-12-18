AC_INIT(
    [the_silver_searcher],
    [1.0.2-wild],
    [https://github.com/ggreer/the_silver_searcher/issues],
    [the_silver_searcher],
    [https://github.com/ggreer/the_silver_searcher])

AM_INIT_AUTOMAKE([no-define foreign subdir-objects])

AC_PROG_CC
AM_PROG_CC_C_O
AC_PREREQ([2.59])
AC_PROG_SED

m4_ifdef(
    [AM_SILENT_RULES],
    [AM_SILENT_RULES([yes])])

m4_include([m4/ax_pthread.m4])
AX_PTHREAD(
    [AC_CHECK_HEADERS([pthread.h])],
    [AC_MSG_WARN([No pthread support. Ag will be slower due to running single-threaded.])]
)

# Run CFLAGS="-pg" ./configure if you want debug symbols
CFLAGS="$CFLAGS $PTHREAD_CFLAGS -Wall -Wextra -Wformat=2 -Wno-format-nonliteral -Wshadow"
CFLAGS="$CFLAGS -Wpointer-arith -Wcast-qual -Wmissing-prototypes -Wno-missing-braces -std=gnu89 -D_GNU_SOURCE -O2"
LDFLAGS="$LDFLAGS"

case $host in
*mingw*)
    AC_CHECK_LIB(shlwapi, main,, AC_MSG_ERROR(libshlwapi missing))
esac

LIBS="$PTHREAD_LIBS $LIBS"

AC_ARG_WITH([pcre2],
    [AS_HELP_STRING([--with-pcre2], [Enable experimental support for libpcre2])])

AS_IF([test "x$with_pcre2" = "xyes"], [
      PKG_CHECK_MODULES([PCRE2], [libpcre2-8], [
                        AC_DEFINE([HAVE_PCRE2], [], [Use libpcre2])
                        AC_DEFINE([PCRE2_CODE_UNIT_WIDTH], [8], [8-byte characters for libpcre2])
                        AC_DEFINE([USE_PCRE_JIT], [], [Use PCRE JIT])
                        CFLAGS="$CFLAGS $PCRE2_CFLAGS"
                        ])
      ], [
      PKG_CHECK_MODULES([PCRE], [libpcre], [
                        AC_DEFINE([HAVE_PCRE], [], [Use libpcre])
                        CFLAGS="$CFLAGS $PCRE_CFLAGS"
                        AC_CHECK_DECL([PCRE_CONFIG_JIT], [AC_DEFINE([USE_PCRE_JIT], [], [Use PCRE JIT])],
                                      [], [#include <pcre.h>])
                        ])
])
AM_CONDITIONAL([HAVE_PCRE2], [test "x$with_pcre2" = "xyes"])

AC_ARG_ENABLE([zlib],
    AS_HELP_STRING([--disable-zlib], [Disable zlib compressed search support]))

AS_IF([test "x$enable_zlib" != "xno"], [
    AC_CHECK_HEADERS([zlib.h])
    AC_SEARCH_LIBS([inflate], [zlib, z])
])

AC_ARG_ENABLE([lzma],
    AS_HELP_STRING([--disable-lzma], [Disable lzma compressed search support]))

AS_IF([test "x$enable_lzma" != "xno"], [
    AC_CHECK_HEADERS([lzma.h])
    PKG_CHECK_MODULES([LZMA], [liblzma])
])

AC_CHECK_DECL([CPU_ZERO, CPU_SET], [AC_DEFINE([USE_CPU_SET], [], [Use CPU_SET macros])] , [], [#include <sched.h>])

AC_CHECK_MEMBER([struct dirent.d_type], [AC_DEFINE([HAVE_DIRENT_DTYPE], [], [Have dirent struct member d_type])], [], [[#include <dirent.h>]])
AC_CHECK_MEMBER([struct dirent.d_namlen], [AC_DEFINE([HAVE_DIRENT_DNAMLEN], [], [Have dirent struct member d_namlen])], [], [[#include <dirent.h>]])

AC_CHECK_FUNCS(fgetln getline realpath strlcpy strndup vasprintf madvise posix_fadvise pthread_setaffinity_np pledge)

AC_CONFIG_FILES([Makefile the_silver_searcher.spec])
AC_CONFIG_HEADERS([src/config.h])

AC_CHECK_PROGS(
    [CLANG_FORMAT],
    [clang-format-3.8 clang-format-3.7 clang-format-3.6 clang-format],
    [no]
)
AM_CONDITIONAL([HAS_CLANG_FORMAT], [test x$CLANG_FORMAT != xno])
AM_COND_IF(
    [HAS_CLANG_FORMAT],
    [AC_MSG_NOTICE([clang-format found. 'make test' will detect improperly-formatted files.])],
    [AC_MSG_WARN([clang-format not found. 'make test' will not detect improperly-formatted files.])]
)

# Determine if we're building in a git repo, and if so enable dynamic versioning.
# If this isn't a git repo, assume this is building from a release tarball and use
# PACKAGE_VERSION. Disable if --disable-gitversion is passed.
AC_ARG_ENABLE([gitversion],
              AS_HELP_STRING([--disable-gitversion], [Disable dynamic git version and use PACKAGE_VERSION instead]))

ag_use_gitversion="no"
AS_IF([test "x$enable_gitversion" != "xno"], [
    AC_CHECK_PROG([ag_has_git], [git], [yes])
    AS_IF([test "x$ag_has_git" = "xyes"], [
        AC_MSG_CHECKING([Whether this is a git repo])
        AS_IF([test -d "./.git" && git status --porcelain >/dev/null 2>&1], [
            AC_MSG_RESULT([yes])
            ag_use_gitversion="yes"
        ], [
            AC_MSG_RESULT([no])
        ])
    ])
])
AM_CONDITIONAL([USE_GITVERSION], [test "x$ag_use_gitversion" = "xyes"])
AM_COND_IF([USE_GITVERSION], [], [
    dnl Logic to use USE_GITVERSION is handled in Makefile.in. If not using gitversion
    dnl then use sed to create version.c during configure
    AS_IF([test -f "src/version.c.in"], [
        AC_MSG_NOTICE([Creating version.c])
        rm -f src/version.c
        $SED 's/@AG_VERSION@/PACKAGE_VERSION/' src/version.c.in >src/version.c
    ])
])

AC_OUTPUT
