#!/bin/sh -e
#
# Script to create a working configure on Debian Etch
#

# add additional files for the LT_INIT macro
libtoolize --force --copy   
aclocal -I ./
# create configure script
autoconf
