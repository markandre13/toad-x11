#!/bin/sh -e
#
# Script to create a working configure on Debian Sarge
#

# add additional files for the AC_PROG_LIBTOOL macro
libtoolize --force --copy   

# create configure script
autoconf
