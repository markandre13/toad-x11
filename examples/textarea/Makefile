#
# TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
# Copyright (C) 1996-2004 by Mark-André Hopf <mhopf@mark13.de>
#

PRGFILE		= toe
SRCS		  = hacktool.cc resource.cc
RESOURCE	= hacktool.png order.atv preferences.atv
# LIBS			= -lefence

CLEAN = resource.cc

include ../../conf/MakefilePrg

resource.cc: Makefile $(RESOURCE)
	../../bin/toadbin2c $(RESOURCE) > resource.cc

# DO NOT DELETE
