############################################################################
#                                                                          #
# TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System    #
# Copyright (C) 1996-2000 by Mark-André Hopf                               #
#                                                                          #
############################################################################

PRGFILE		= try001
SRCS		= try001.cc
LIBS		=



include ../../conf/MakefilePrg

mcheck:
	MALLOC_CHECK_=2 MALLOC_TRACE=malloc.log ./$(PRGFILE)
	mtrace $(PRGFILE) malloc.log
