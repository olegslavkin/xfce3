#!/bin/sh

aclocal -I ac --verbose \
  && autoheader \
  && automake --add-missing --copy --include-deps --foreign --gnu --verbose \
  && autoconf \
  && ./configure $@
