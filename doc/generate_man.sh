#!/bin/sh

# ronn is used to turn the markdown into a manpage.
# Get ronn at https://github.com/rtomayko/ronn
# Alternately, since ronn is a Ruby gem, you can just
# `gem install ronn`

sed -e 's/\\0/\\\\0/g' ag.1.md | ronn --pipe --roff --warnings >ag.1
