#
# Autoconf file for Fischland
#
# Copyright (C) 2002-2005 by Mark-André Hopf <mhopf@mark13.org>
#

AC_INIT(fischland, 0.64.0, mhopf@mark13.org)

export PKG_CONFIG_PATH="${PKG_CONFIG_PATH}:/usr/local/lib/pkgconfig"

AC_CONFIG_SRCDIR([draw.cc])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_LN_S
AC_PROG_MAKE_SET
  
AC_LANG(C)

PKG_CHECK_MODULES(TOAD, toad)
PKG_CHECK_MODULES(CAIRO, cairo, AC_DEFINE(HAVE_LIBCAIRO, 1))

#
# merge all found compiler and linker flags and remove duplicates
#

CFLAGS="$CPPFLAGS $DEFS $CFLAGS $TOAD_CFLAGS $CAIRO_CFLAGS"
CXXFLAGS="$CPPFLAGS $DEFS $CXXFLAGS $TOAD_CFLAGS $CAIRO_CFLAGS"
LIBS="$TOAD_LIBS $CAIRO_LIBS"

# the order of libraries is important so when removing libraries, we must
# keep the last '-l', which we do with the two loops below:
z=""
u=""  
for x in $LIBS ; do
  if test "${x#-L}" != "$x" ; then
    if test `echo "$u" | grep -ce "$x" -` = 0 ; then
      u="$u $x"
    fi
  else
    z="$x $z"
  fi
done

y=""
for x in $z ; do
  if test `echo "$y" | grep -ce "$x" -` = 0 ; then
    y="$x $y"
  fi
done
LIBS="$u $y"

y=""
for x in $CXXFLAGS ; do
  if test `echo "$y" | grep -ce "$x" -` = 0 ; then
    y="$y $x"
  fi
done
CXXFLAGS="$y"

y=""
for x in $CFLAGS ; do
  if test `echo "$y" | grep -ce "$x" -` = 0 ; then
    y="$y $x"
  fi
done
CFLAGS="$y"

cat<<EOF

configure results:

SHELL                                 : $SHELL

DEFS                                  : $DEFS
CPPFLAGS                              : $CPPFLAGS

CC                                    : $CC
CFLAGS                                : $CFLAGS

CXX                                   : $CXX
CXXFLAGS                              : $CXXFLAGS

LDFLAGS: $LDFLAGS
LIBS                                  : $LIBS

TOAD_CFLAGS                           : $TOAD_CFLAGS
TOAD_LIBS                             : $TOAD_LIBS
CAIRO_CFLAGS                          : $CAIRO_CFLAGS
CAIRO_LIBS                            : $CAIRO_LIBS
EOF

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
