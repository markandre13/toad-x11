#
# Autoconf file for TOAD
#
# Copyright (C) 2002-2005 by Mark-André Hopf <mhopf@mark13.org>
#

AC_INIT(libtoad, 0.64.0, toad-bug@mark13.org)

AC_CONFIG_SRCDIR([src/toadbase.cc])
AC_CONFIG_HEADER([src/config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_LN_S
AC_PROG_MAKE_SET
  
# The libtool macro requires the file install-sh, config.sub and config.guess
# within the local directory. I've used these on my Debian Sarge:
# /usr/share/libtool/libltdl/install-sh
# /usr/share/libtool/config.sub
# /usr/share/libtool/config.guess
AC_PROG_LIBTOOL

AC_LANG(C)

# Checks for libraries.
AC_PATH_XTRA
XLIBS="${X_LIBS} ${X_EXTRA_LIBS}"

AC_CHECK_LIB(m, sqrt)
AC_CHECK_LIB(jpeg, jpeg_start_decompress)
AC_CHECK_LIB(z, zlibVersion)
AC_CHECK_LIB(png, png_create_read_struct)

# AC_CHECK_LIB([MesaGL], [main])
# AC_CHECK_LIB([MesaGLU], [main])
AC_CHECK_LIB(X11, XOpenDisplay,,AC_MSG_ERROR([Could not link with X11]), $XLIBS)

# AC_CHECK_LIB([Xext], [main])
# AC_CHECK_LIB([Xmu], [XmuClientWindow],,AC_MSG_ERROR([Could not link with Xmu]),$XLIBS)
AC_CHECK_LIB(Xutf8, XUtf8DrawString,,, $XLIBS)

PKG_CHECK_MODULES(FONTCONFIG, fontconfig)
# PKG_CHECK_MODULES(FREETYPE, freetype2)
PKG_CHECK_MODULES(XFT, xft, AC_DEFINE(HAVE_LIBXFT, 1))



#
# merge all found compiler and linker flags and remove duplicates
#

echo "LIBS=$LIBS"

CFLAGS="$CPPFLAGS $DEFS $CFLAGS $X_CFLAGS $FONTCONFIG_CFLAGS $XFT_CFLAGS"
CXXFLAGS="$CPPFLAGS $DEFS $CXXFLAGS $X_CFLAGS $FONTCONFIG_CFLAGS $XFT_CFLAGS"
LIBS="$X_LIBS $FONTCONFIG_LIBS $XFT_LIBS $X_PRE_LIBS $X_EXTRA_LIB $LIBS"

# the order of libraries is important so when removing libraries, we must
# keep the last '-l', which we do with the two loops below:
z=""
for x in $LIBS ; do
  z="$x $z"
done

y=""
for x in $z ; do
  if test `echo "$y" | grep -ce "$x" -` = 0 ; then
    y="$x $y"
  fi
done
LIBS="`echo $y`"

y=""
for x in $CXXFLAGS ; do
  if test `echo "$y" | grep -ce "$x" -` = 0 ; then
    y="$y $x"
  fi
done
CXXFLAGS="`echo $y`"

y=""
for x in $CFLAGS ; do
  if test `echo "$y" | grep -ce "$x" -` = 0 ; then
    y="$y $x"
  fi
done
CFLAGS="`echo $y`"

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
X_LIBS (X linker flags)               : $X_LIBS
X_CFLAGS (X compiler flags)           : $X_CFLAGS
X_EXTRA_LIBS (X required libs)        : $X_EXTRA_LIBS
X_PRE_LIBS (to be linked before -lX11): $X_PRE_LIBS

XFT_CFLAGS                            : $XFT_CFLAGS
XFT_LIBS                              : $XFT_LIBS
FONTCONFIG_CFLAGS                     : $FONTCONFIG_CFLAGS
FONTCONFIG_LIBS                       : $FONTCONFIG_LIBS
FREETYPE_CFLAGS                       : $FREETYPE_CFLAGS
FREETYPE_LIBS                         : $FREETYPE_LIBS

LIBTOOL                               : $LIBTOOL

EOF

AC_CONFIG_FILES([Makefile src/Makefile conf/MakefilePrg testsuite/Makefile conf/toad-config toad.pc])

AC_OUTPUT
