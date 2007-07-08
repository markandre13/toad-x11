PRGFILE=html
SRCS=html.cc

CXX=`toad-config --cxx`
CXXFLAGS=`toad-config --cxxflags` -g
LD=$(CXX)
LDFLAGS=`toad-config --libs`
# LDFLAGS+=-lefence
OBJS=$(SRCS:.cc=.o)

all: $(PRGFILE)

.SUFFIXES: .cc

.cc.o:
	@echo compiling $*.cc ...
	@$(CXX) $(CXXFLAGS) $*.cc -c -o $*.o

$(PRGFILE): $(OBJS)
	@echo linking $(PRGFILE) ...
	@$(LD) $(LDFLAGS) $(OBJS) -o $(PRGFILE) $(LDFLAGS)
	@echo Ok

clean::
	@echo removing generated files in `pwd`
	@rm -f $(PRGFILE) .depend $(CLEAN)
	@find . -name "*.o" -exec rm -f {} \;
	@find . -name "*.bak" -exec rm -f {} \;
	@find . -name "*~" -exec rm -f {} \;
	@find . -name "core" -exec rm -f {} \;
	@find . -name "DEADJOE" -exec rm -f {} \;
	@find . -name "a.out" -exec rm -f {} \;

depend::
	@/usr/X11R6/bin/makedepend $(INCDIRS) -Y $(SRCS) 2> /dev/null
