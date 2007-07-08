#
# toadbone - a X11 cd player with the TOAD toolkit
#
# Copyleft (C) 1998,99 by Mark-André Hopf
#
#-----------------------------------------------------------------------------

PRGFILE	    = toadbone

SRCS        = toadbone.cc \
              resource.cc \
	      sha.cc base64.cc

RESOURCE    = $(wildcard resource/*)

LIBS 	    =
LIBDIRS	    =
INCDIRS	    =

CLEAN       = resource.cc

include ../../conf/MakefilePrg

resource.cc: Makefile $(RESOURCE)
	../../bin/toadbin2c $(RESOURCE) > resource.cc
