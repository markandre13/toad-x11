############################################################################
#                                                                          #
# TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System    #
# Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.de>             #
#                                                                          #
############################################################################

all: spreadsheet

PRGFILE	= spreadsheet
SRCS		= spreadsheet.cc calc.y.cc
CLEAN   = calc.l.cc calc.y.cc

calc.l.cc: calc.l
	flex -ocalc.l.cc calc.l  

calc.y.cc: calc.y
	bison --yacc -ocalc.y.cc calc.y
  
calc.y.cc: calc.l.cc

include ../../conf/MakefilePrg

# DO NOT DELETE
