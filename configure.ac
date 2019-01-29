AC_PREREQ([2.64])
AC_INIT(
    [the_silver_searcher],
    [2.2.0-wild],
    [https://github.com/aswild/the_silver_searcher/issues],
    [the_silver_searcher],
    [https://github.com/aswild/the_silver_searcher])

AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([no-define foreign subdir-objects])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_SED
AC_PROG_GREP

# pthreads check. Note that if -Werror is in CFLAGS, the pthreads checks always fail, so temporarily remove it
save_cflags="$CFLAGS"
dnl an extra set of brackets is needed because of m4 bracket quoting insanity
CFLAGS=`echo "$save_cflags" | $SED 's/-Werror[[^[:space:]]]*//g'`
AX_PTHREAD(
    [AC_CHECK_HEADERS([pthread.h])],
    [AC_MSG_WARN([No pthread support. Ag will be slower due to running single-threaded.])]
)
CFLAGS="$save_cflags"
AW_APPEND_CFLAGS([$PTHREAD_CFLAGS])
AW_APPEND_LIBS([$PTHREAD_LIBS])

AC_CANONICAL_HOST
case "$host_os" in
    *mingw*)
        AC_CHECK_LIB(shlwapi, main,, AC_MSG_ERROR(libshlwapi missing))
        ;;
    *linux*)
        AC_DEFINE([OS_LINUX], [], [Building for Linux (not OSX/cygwin/mingw)])
        ;;
esac

dnl adl_RECURSIVE_EVAL(VALUE, RESULT)
dnl =================================
dnl Interpolate the VALUE in loop until it doesn't change,
dnl and set the result to $RESULT.
dnl WARNING: It's easy to get an infinite loop with some unsane input.
dnl from http://ac-archive.sourceforge.net/adl/relpaths.html
AC_DEFUN([adl_RECURSIVE_EVAL],
[_lcl_receval="$1"
$2=`(test "x$prefix" = xNONE && prefix="$ac_default_prefix"
     test "x$exec_prefix" = xNONE && exec_prefix="${prefix}"
     _lcl_receval_old=''
     while test "[$]_lcl_receval_old" != "[$]_lcl_receval"; do
       _lcl_receval_old="[$]_lcl_receval"
       eval _lcl_receval="\"[$]_lcl_receval\""
     done
     echo "[$]_lcl_receval")`])

# Add ${libdir}/pkgconfig to PKG_CONFIG_PATH if it exists. This helps when libraries
# were installed to /usr/local/lib, which isn't searched by pkg-config by default
adl_RECURSIVE_EVAL([${libdir}], libdir_expanded)
if test -d "${libdir_expanded}/pkgconfig"; then
    if test -n "${PKG_CONFIG_PATH}"; then
        PKG_CONFIG_PATH="${PKG_CONFIG_PATH}:${libdir_expanded}/pkgconfig"
    else
        PKG_CONFIG_PATH="${libdir_expanded}/pkgconfig"
    fi
    export PKG_CONFIG_PATH
fi

################### PCRE Check #####################
# pcre1 is no longer supported, only pcre2
PKG_CHECK_MODULES([PCRE2], [libpcre2-8], [
    AC_DEFINE([PCRE2_CODE_UNIT_WIDTH], [8], [8-byte characters for pcre2])
    AW_APPEND_CFLAGS([$PCRE2_CFLAGS])
    AW_APPEND_LIBS([$PCRE2_LIBS])
    AC_CHECK_HEADERS([pcre2.h], [], [AC_MSG_ERROR([pkg-config found libpcre2-8 but pcre2.h couldn't be found])])
])

##### ZLIB #####
AC_ARG_ENABLE([zlib],
    AS_HELP_STRING([--disable-zlib], [Disable zlib compressed search support]))
AS_IF([test "x$enable_zlib" != "xno"], [
    PKG_CHECK_MODULES([ZLIB], [zlib], [ag_found_zlib="yes"], [ag_found_zlib="no"])
    AS_IF([test "x$ag_found_zlib" = "xyes"], [
          AW_APPEND_CFLAGS([$ZLIB_CFLAGS])
          AW_APPEND_LIBS([$ZLIB_LIBS])
          AC_CHECK_HEADERS([zlib.h], [], [AC_MSG_ERROR([pkg-config found zlib but zlib.h couldn't be found])])
    ])
])
AS_IF([test "x$ag_found_zlib" = "xyes"],
      [AC_DEFINE([USE_ZLIB], [], [Enable zlib/gzip decompression])],
      [AS_IF([test "x$enable_zlib" = "xyes"], [AC_MSG_ERROR([--enable-zlib was specified but zlib couldn't be found])])
])

##### LZMA #####
AC_ARG_ENABLE([lzma],
    AS_HELP_STRING([--disable-lzma], [Disable lzma compressed search support]))
AS_IF([test "x$enable_lzma" != "xno"], [
    PKG_CHECK_MODULES([LZMA], [liblzma], [ag_found_lzma="yes"], [ag_found_lzma="no"])
    AS_IF([test "x$ag_found_lzma" = "xyes"], [
          AW_APPEND_CFLAGS([$LZMA_CFLAGS])
          AW_APPEND_LIBS([$LZMA_LIBS])
          AC_CHECK_HEADERS([lzma.h], [], [AC_MSG_ERROR([pkg-config found lzma but lzma.h couldn't be found])])
    ])
])
AS_IF([test "x$ag_found_lzma" = "xyes"],
      [AC_DEFINE([USE_LZMA], [], [Enable lzma/gzip decompression])],
      [AS_IF([test "x$enable_lzma" = "xyes"], [AC_MSG_ERROR([--enable-lzma was specified but lzma couldn't be found])])
])

AC_CHECK_DECLS([CPU_ZERO, CPU_SET], [AC_DEFINE([USE_CPU_SET], [], [Use CPU_SET macros])] , [], [#include <sched.h>])
AC_CHECK_HEADERS([sys/cpuset.h err.h])

AC_CHECK_DECL([_WIN32], ag_win32_build="yes")
AM_CONDITIONAL([WIN32_BUILD], [test "x$ag_win32_build" = "xyes"])

AC_CHECK_MEMBER([struct dirent.d_type], [AC_DEFINE([HAVE_DIRENT_DTYPE], [], [Have dirent struct member d_type])], [], [[#include <dirent.h>]])
AC_CHECK_MEMBER([struct dirent.d_namlen], [AC_DEFINE([HAVE_DIRENT_DNAMLEN], [], [Have dirent struct member d_namlen])], [], [[#include <dirent.h>]])

AC_CHECK_FUNCS(fgetln getline realpath strlcpy strndup vasprintf madvise posix_fadvise pthread_setaffinity_np pledge)

# Only build zfile.c if we need it
AC_ARG_ENABLE([fopencookie], [AS_HELP_STRING([--enable-fopencookie], [Search compressed files as streams])])
AS_IF([test "$enable_fopencookie" = "yes"], [AC_CHECK_FUNCS(fopencookie)])
AM_CONDITIONAL([USE_FOPENCOOKIE], [test "$enable_fopencookie" = "yes" && test "$ac_cv_func_fopencookie" = "yes" &&
                                   (test "$ac_cv_header_zlib_h" = "yes" || test "$ac_cv_header_lzma_h" = "yes")])
AM_COND_IF([USE_FOPENCOOKIE], [AC_DEFINE([USE_FOPENCOOKIE], [], [Use fopencookie streaming in zfile.c])])

AC_CHECK_PROGS([CRAM], [cram cram3 cram2], [no])
AM_CONDITIONAL([HAVE_CRAM], [test x$CRAM != xno])
AM_COND_IF([HAVE_CRAM], [ag_test_suite=enabled],
           [ag_test_suite=disabled
            AC_MSG_WARN([cram not found, test suite is disabled])])

# List the tests during configure time, because we can't use wildcards in Makefile.am.
# Note these must be relative to ${srcdir} but not contain ${srcdir} or else the log/trs
# files will get created in ${srcdir} instead of ${builddir}.
CRAM_TESTS="`(cd ${srcdir}; ls -1 tests/*.t) | tr '\n' ' '`"
AC_SUBST([CRAM_TESTS])

AC_CHECK_PROGS(
    [CLANG_FORMAT],
    [clang-format clang-format-6.0 clang-format-5.0 clang-format-4.0 clang-format-3.8],
    [no]
)
AM_CONDITIONAL([HAVE_CLANG_FORMAT], [test x$CLANG_FORMAT != xno])
AM_COND_IF(
    [HAVE_CLANG_FORMAT], [ag_clang_format=enabled],
    [ag_clang_format=disabled
    AC_MSG_WARN([clang-format not found. 'make test' will not detect improperly-formatted files.])]
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
        AS_IF([test -e "${srcdir}/.git" && git -C "${srcdir}" status --porcelain >/dev/null 2>&1], [
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
    dnl Only do this if version.c.in exists. If it doesn't assume we're building from a
    dnl release tarball and version.c already exists
    AS_IF([test -f "${srcdir}/src/version.c.in"], [
        AC_MSG_NOTICE([Creating version.c])
        rm -f ${srcdir}/src/version.c
        mkdir -p src
        $SED 's/@AG_VERSION@/PACKAGE_VERSION/' ${srcdir}/src/version.c.in >src/version.c
    ])
])

AW_STRIP_FLAGS([CFLAGS])
AW_STRIP_FLAGS([LDFLAGS])
AW_STRIP_FLAGS([LIBS])

AC_CONFIG_FILES([Makefile the_silver_searcher.spec])
AC_CONFIG_HEADERS([src/config.h])
AC_OUTPUT

# pretty-print options to the console
ag_pcre_string="pcre2"
AM_COND_IF([USE_GITVERSION],
           dnl note the git version command should match Makefile.am
           [ag_version=`git -C "${srcdir}" describe --dirty=+ --always --tags`],
           [ag_version="$VERSION"])

cat <<EOF

The Silver Searcher $ag_version configuration summary:

    Install prefix          ${prefix}
    C compiler              ${CC}
    C preprocessor flags    ${CPPFLAGS}
    C compiler flags        ${CFLAGS}
    Linker flags            ${LDFLAGS}
    Linker libraries        ${LIBS}

    PCRE version            ${ag_pcre_string}
    zlib decompression      ${ag_found_zlib}
    lzma decompression      ${ag_found_lzma}

    Test suite              ${ag_test_suite}
    Code format check       ${ag_clang_format}

EOF
