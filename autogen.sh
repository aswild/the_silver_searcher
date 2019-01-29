#!/bin/sh

set -e
cd "$(dirname "$0")"

AC_SEARCH_OPTS=""
# For those of us with pkg-config and other tools in /usr/local
PATH=$PATH:/usr/local/bin

# This is to make life easier for people who installed pkg-config in /usr/local
# but have autoconf/make/etc in /usr/. AKA most mac users
if [ -d "/usr/local/share/aclocal" ]
then
    AC_SEARCH_OPTS="-I /usr/local/share/aclocal"
fi

if [ "$1" = "-f" ] || [ "$1" = "--force" ]; then
    force=--force
else
    force=
fi

# shellcheck disable=2086
set -x
aclocal -I m4 $AC_SEARCH_OPTS $force
autoconf $force
autoheader $force
automake --add-missing $force
