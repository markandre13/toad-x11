#
# Autoconf file for TOAD
#
# Copyright (C) 2002-2004 by Mark-André Hopf <mhopf@mark13.org>
#

AC_INIT(libtoad, 0.64.0, toad-bug@mark13.org)

AC_CONFIG_SRCDIR([src/toadbase.cc])
AC_CONFIG_HEADER([src/config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for libraries.
AC_PATH_XTRA

# libjpeg (optional)
#^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
AC_CHECK_LIB([jpeg], [jpeg_start_decompress],,,$LIBS)

# X11 libs
#^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
XLIBS="${X_LIBS} ${X_PRE_LIBS} ${X_EXTRA_LIBS}"

# AC_CHECK_LIB([MesaGL], [main])
# AC_CHECK_LIB([MesaGLU], [main])
AC_CHECK_LIB([X11], [XOpenDisplay],,AC_MSG_ERROR([Could not link with X11]), $XLIBS)
# AC_CHECK_LIB([Xext], [main])
# AC_CHECK_LIB([Xmu], [XmuClientWindow],,AC_MSG_ERROR([Could not link with Xmu]),$XLIBS)
AC_CHECK_LIB([m], [main])

# pkg-config (required)
#^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
AC_ARG_WITH(pkg-config, [  --with-pkg-config=PROG  Use pkg-config configuration utility PROG], freetype_config=$withval, pkg_config=yes)
if test "$pkg_config" = "yes"; then
  AC_PATH_PROG(pkg_config,pkg-config,no)
  if test "$pkg_config" = "no"; then
    AC_MSG_ERROR([You must have pkg-config installed; see http://www.freetype.org/])
  fi
else
  pkg_config="$pkg_config"
fi

# fontconfig (required)
#^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
AC_MSG_CHECKING([fontconfig])
pkg-config --atleast-version=2.2.0 fontconfig
if test $? -ne 0 ; then
  AC_MSG_ERROR([You must have fontconfig 2.2.0 or higher; see http://www.freedesktop.org/])
fi
AC_MSG_RESULT([yes])
CXXFLAGS="$CXXFLAGS `$pkg_config --cflags fontconfig`"
LIBS="$LIBS `$pkg_config --libs fontconfig`"

# freetype (required)
#^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
AC_ARG_WITH(freetype-config, [  --with-freetype-config=PROG Use FreeType configuration program PROG], freetype_config=$withval, freetype_config=yes)
if test "$freetype_config" = "yes"; then
  AC_PATH_PROG(ft_config,freetype-config,no)
  if test "$ft_config" = "no"; then
    AC_MSG_ERROR([You must have freetype installed; see http://www.freetype.org/])
  fi
else
  ft_config="$freetype_config"
fi

CXXFLAGS="$CXXFLAGS `$ft_config --cflags`"
LIBS="$LIBS `$ft_config --libs`"
AC_CHECK_LIB([Xft], [XftFontOpen],,,$XLIBS)

# libpng (required)
#^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
AC_CHECK_LIB([z], [zlibVersion],,
  AC_MSG_ERROR(Could not link with libz),$XLIBS)
AC_CHECK_LIB([png], [png_create_read_struct],, 
  AC_MSG_ERROR(Could not link with libpng),$XLIBS)


# libXutf8 (required)
#^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
dnl Test to see if libXutf8 is on this machine:
XUTFLIB=
AC_CHECK_LIB(
  [Xutf8], [XUtf8DrawString],,
  AC_MSG_ERROR([Could not link with Xutf8; see http://www.oksid.ch/xutf8/]),
  $XLIBS)

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
# AC_CHECK_HEADERS([arpa/inet.h errno.h fcntl.h limits.h malloc.h netdb.h netinet/in.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_TYPE_SIGNAL
AC_FUNC_STAT  
# AC_CHECK_FUNCS([floor getcwd gethostbyname gettimeofday memset select setlocale socket sqrt strcasecmp strchr strerror strncasecmp])

AC_MSG_CHECKING([absolute source path])
cwd=`pwd`
cd $srcdir
toaddir=`pwd`
cd $cwd
AC_SUBST(toaddir)
AC_MSG_RESULT([$toaddir])



AC_CONFIG_FILES([Makefile src/Makefile conf/MakefilePrg testsuite/Makefile conf/toad-config toad.pc])

AC_OUTPUT
