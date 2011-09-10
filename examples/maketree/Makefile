PRGFILE		= maketree
SRCS		= maketree.cc glwindow.cc math/matrix4.cc math/vector3.cc

CXXFLAGS	= -I/usr/include/libxml2
LIBS		= -lGL -lm -lxml2 -O3
INCDIRS		=
LIBDIRS		=

include ../../conf/MakefilePrg
# DO NOT DELETE

maketree.o: glwindow.hh math/matrix4.h math/vector3.h
glwindow.o: glwindow.hh
math/matrix4.o: math/matrix4.h math/vector3.h
math/vector3.o: math/vector3.h math/matrix4.h
