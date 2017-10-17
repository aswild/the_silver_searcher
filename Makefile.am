ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

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
	src/pcre_api.c \
	src/pcre_api.h \
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

bashcompdir = $(pkgdatadir)/completions
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

test: ag
	AGPROG=$(PWD)/ag cram -v $(srcdir)/tests/*.t
if HAS_CLANG_FORMAT
	CLANG_FORMAT=$(CLANG_FORMAT) $(srcdir)/format.sh test
else
	@echo "clang-format is not available. Skipped clang-format test."
endif

test_big: ag
	cram -v tests/big/*.t

test_fail: ag
	cram -v tests/fail/*.t

.PHONY : all clean test test_big test_fail
