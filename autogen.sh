#!/bin/sh -e
#
# Script to create a working configure on Debian Etch
#

# add additional files for the LT_INIT macro
libtoolize --force --copy || 
  cp autogen/config.guess \
     autogen/config.sub \
     autogen/ltmain.sh .
aclocal -I ./ || cp autogen/aclocal.m4 .
# create configure script
autoconf || cp autogen/configure .
