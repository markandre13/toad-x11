# Autoconf file for TOAD
#
# Copyright (C) 2002, 2003 by Mark-André Hopf <mhopf@mark13.de>
#

AC_INIT(libtoad, 0.64.0, toad-bug@mark13.de)

AC_CONFIG_SRCDIR([src/toadbase.cc])
# AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for libraries.
AC_PATH_XTRA
# AC_CHECK_LIB([MesaGL], [main])
# AC_CHECK_LIB([MesaGLU], [main])
AC_CHECK_LIB([Xext], [main])
AC_CHECK_LIB([m], [main])

# libpng (required)
#^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
AC_CHECK_LIB([z], [zlibVersion],,
  AC_MSG_ERROR(Could not link with libz))
AC_CHECK_LIB([png], [png_create_read_struct],, 
  AC_MSG_ERROR(Could not linkwith libpng))

# libjpeg (optional)
#^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
AC_CHECK_LIB([jpeg], [jpeg_start_decompress])

# Checks for header files.
AC_PATH_X
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h errno.h fcntl.h limits.h malloc.h netdb.h netinet/in.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h termios.h unistd.h])

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
AC_CHECK_FUNCS([floor getcwd gethostbyname gettimeofday memset select setlocale socket sqrt strcasecmp strchr strerror strncasecmp])

cwd=`pwd`
cd $srcdir
toaddir=`pwd`
cd $cwd
AC_SUBST(toaddir)

AC_CONFIG_FILES([Makefile src/Makefile conf/MakefilePrg testsuite/Makefile conf/toad-config])

AC_OUTPUT
