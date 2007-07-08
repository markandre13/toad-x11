############################################################################
#                                                                          #
# TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System    #
# Copyright (C) 1995-2004 by Mark-André Hopf <mhopf@mark13.de>             #
#                                                                          #
############################################################################

PRGFILE		= table
SRCS		= table.cc
LIBS		=

include ../../conf/MakefilePrg

mcheck:
	MALLOC_CHECK_=2 MALLOC_TRACE=malloc.log ./$(PRGFILE)
	mtrace $(PRGFILE) malloc.log
