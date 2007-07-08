############################################################################
#                                                                          #
# TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System    #
# Copyright (C) 1996-2002 by Mark-André Hopf <mhopf@mark13.de>             #
#                                                                          #
############################################################################

PRGFILE		= stackstrace
SRCS		= stacktrace.cc
LIBS		=

include ../../conf/MakefilePrg

mcheck:
	MALLOC_CHECK_=2 MALLOC_TRACE=malloc.log ./$(PRGFILE)
	mtrace $(PRGFILE) malloc.log
