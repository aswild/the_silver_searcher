ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
AM_CFLAGS = -std=gnu99 -D_GNU_SOURCE \
            -Wall -Wextra -Wformat=2 -Wno-format-nonliteral -Wshadow -Wpointer-arith \
            -Wcast-qual -Wmissing-prototypes -Wno-missing-braces
if ENABLE_WERROR
AM_CFLAGS += -Werror
endif

bin_PROGRAMS = ag
ag_SOURCES = \
	src/ignore.c \
	src/ignore.h \
	src/log.c \
	src/log.h \
	src/options.c \
	src/options.h \
	src/print.c \
	src/print.h \
	src/scandir.c \
	src/scandir.h \
	src/search.c \
	src/search.h \
	src/lang.c \
	src/lang.h \
	src/util.c \
	src/util.h \
	src/decompress.c \
	src/decompress.h \
	src/uthash.h \
	src/version.c \
	src/version.h \
	src/main.c

if WIN32_BUILD
ag_SOURCES += src/print_w32.c
endif

if USE_FOPENCOOKIE
ag_SOURCES += src/zfile.c
endif

dist_man_MANS = doc/ag.1

bashcompdir = $(datadir)/bash-completion/completions
dist_bashcomp_DATA = ag.bashcomp.sh
zshcompdir = $(datadir)/zsh/site-functions
dist_zshcomp_DATA = _the_silver_searcher

EXTRA_DIST = Makefile.w32 LICENSE NOTICE the_silver_searcher.spec README.md

if USE_GITVERSION
AG_VERSION := $(shell git -C "$(srcdir)" describe --dirty=+ --always --tags)
$(shell grep "^$(AG_VERSION)" .version >/dev/null 2>&1 || echo "$(AG_VERSION)" > .version)
CLEANFILES = .version src/version.c

src/version.c: .version
	@rm -f src/version.c
	@$(SED) "s/@AG_VERSION@/\"$(AG_VERSION)\"/" $(srcdir)/src/version.c.in >src/version.c
endif

#
# TEST SUITE
#
# Always define all the tests, but some will be skipped if cram or clang-format
# is unavailable. As a slight hack, define .sh as a test extension to get format.sh
TESTS = $(CRAM_TESTS) format.sh
TEST_EXTENSIONS = .t .sh

# Use a customized test driver which prints the test command's output when a test fails.
# have to set all three of these or Automake will still try to use its built-in version
LOG_DRIVER = $(srcdir)/tests/test-driver
T_LOG_DRIVER = $(LOG_DRIVER)
SH_LOG_DRIVER = $(LOG_DRIVER)

# Environment variables exported for all tests
AM_TESTS_ENVIRONMENT  = AGPROG='$(abs_builddir)/ag'; export AGPROG;
AM_TESTS_ENVIRONMENT += CLANG_FORMAT='$(CLANG_FORMAT)'; export CLANG_FORMAT;

# Check program availability to determine how to actually run the tests.
# Exit code 77 means to skip a test
if HAVE_CRAM
T_LOG_COMPILER = $(CRAM)
AM_T_LOG_FLAGS = -v
else
T_LOG_COMPILER = $(SHELL)
AM_T_LOG_FLAGS = -c 'echo "cram is not available: skip test"; exit 77'
endif

if HAVE_CLANG_FORMAT
SH_LOG_COMPILER = $(srcdir)/format.sh
AM_SH_LOG_FLAGS = test
else
SH_LOG_COMPILER = $(SHELL)
AM_SH_LOG_FLAGS = -c 'echo "clang-format is not available: skip test"; exit 77'
endif

# backwards-compatible alias and one-off special tests
test: check

test_big: all
	$(AM_TESTS_ENVIRONMENT) $(T_LOG_COMPILER) $(AM_T_LOG_FLAGS) $(srcdir)/tests/big/*.t

test_fail: all
	$(AM_TESTS_ENVIRONMENT) $(T_LOG_COMPILER) $(AM_T_LOG_FLAGS) $(srcdir)/tests/fail/*.t

.PHONY: test test_big test_fail
