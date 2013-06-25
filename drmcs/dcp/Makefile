# Project: libdcp

CXX  = g++
CC   = gcc
TAR  = tar
SRC  = platform.h version.h Cfg.cpp Cfg.h crcbytevector.h DcpIn.cpp DcpIn.h DcpOut.cpp DcpOut.h DcpUtil.cpp DcpUtil.h Makefile PftIn.cpp PftIn.h PftOut.cpp PftOut.h Reassemble.cpp Reassemble.h sockets.cpp sockets.h timestamp.cpp timestamp.h
OBJ  = Cfg.o DcpIn.o DcpOut.o DcpUtil.o PftIn.o PftOut.o Reassemble.o sockets.o timestamp.o
LIBS =   -L../mdigen
INCS =  
CXXINCS =  -I../common -I../sdixml -I/usr/include/libxml2 -I../RSLib
STATIC  = libdcp.a
BIN = test dcpjoin
CXXFLAGS = $(CXXINCS)  -Wall -g3 -Wshadow -Weffc++ -Wsign-promo -Wcomment 
CFLAGS = $(INCS)   -Wall -g3
RM = rm -f

.PHONY: all all-before all-after clean clean-custom

all: all-before $(STATIC) $(BIN) all-after


clean: clean-custom
	${RM} $(OBJ) $(BIN) $(STATIC)

$(STATIC): $(OBJ)
	ar r $(STATIC) $(OBJ)
	ranlib $(STATIC)
test: $(OBJ) main.cpp
	$(CXX) $(LIBS) $(CXXFLAGS) main.cpp -o test -L. -ldcp -lxml2 -lsdixml -lcommon
dcpjoin: $(OBJ) dcpjoin.cpp
	$(CXX) $(LIBS) $(CXXFLAGS) dcpjoin.cpp -o dcpjoin -L. -lrt -lpcap -ldcp -lxml2 -lsdixml -lcommon -lrs

tar: $(SRC) version.cpp
	$(TAR) zcf libdcp.tar.gz $(SRC) version.cpp Makefile.linux
version.cpp: $(SRC) Makefile.linux
	echo '#include "version.h"' > version.cpp
	echo -n 'const char *libdcp::svn_version() { return "' >> version.cpp
	svnversion -n . >> version.cpp
	echo '"; }' >> version.cpp
