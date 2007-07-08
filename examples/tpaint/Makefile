############################################################################
#                                                                          #
# TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System    #
# Copyright (C) 1996-2000 by Mark-André Hopf                               #
#                                                                          #
############################################################################

PRGFILE		= tpaint
# CFLAGS		= -DTOAD_OLD_CONNECT

# LIBS		= -lefence

SRCS		= tpaint.cc BitmapEditor.cc ColorEditor.cc Tool.cc \
		  NewBitmapDialog.cc Palette.cc ZoomDlg.cc \
                  resource.cc

RESOURCE	= tpaint.png tpaint_draw.png tpaint_fill.png tpaint_plot.png \
		  standard.pal \
		  $(wildcard resource/*)

CLEAN		= bin2c resource.cc
			
DEFINES		=

include ../../conf/MakefilePrg

resource.cc: Makefile $(RESOURCE)
	../../bin/toadbin2c $(RESOURCE) > resource.cc

mcheck:
	MALLOC_CHECK_=2 MALLOC_TRACE=malloc.log ./$(PRGFILE)
	mtrace $(PRGFILE) malloc.log

# DO NOT DELETE

tpaint.o: MainWindow.hh NewBitmapDialog.hh ZoomDlg.hh BitmapEditor.hh Tool.hh
tpaint.o: ColorEditor.hh Palette.hh ToolBar.hh
BitmapEditor.o: BitmapEditor.hh Tool.hh
ColorEditor.o: ColorEditor.hh
Tool.o: BitmapEditor.hh Tool.hh
NewBitmapDialog.o: BitmapEditor.hh NewBitmapDialog.hh
Palette.o: Palette.hh
ZoomDlg.o: BitmapEditor.hh ZoomDlg.hh
