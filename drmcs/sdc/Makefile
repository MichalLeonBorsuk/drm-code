# Project: libsdc

VERSION = 1.0.0
CXX  = g++
CC   = gcc
TAR  = tar
SRC  = sdcblock.cpp sdc.cpp SdcElement.cpp
HDR = sdcblock.h SdcElement.h sdc.h

OBJ  = sdcblock.o sdc.o SdcElement.o

LIBS =   
INCS =  
CXXINCS =  -I/usr/include/libxml2 -I../common -I../sdixml -I.
STATIC  = libsdc.a
BIN = 
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
	$(CXX) $(CXXFLAGS) main.cpp -o test -L../mdigen -L. -lsdixml -lxml2

tar: $(HDR) $(SRC) version.cpp Makefile
	$(TAR) zcf ../libsdixml-$(VERSION).tar.gz $^ debian

version.cpp: $(HDR) $(SRC) Makefile
	echo '#include "version.h"' > version.cpp
	echo -n 'const char *libsdixml::svn_version() { return "' >> version.cpp
	svnversion -n . >> version.cpp
	echo '"; }' >> version.cpp

install: $(STATIC)
	mkdir -p $(DESTDIR)/usr/lib
	cp $(STATIC) $(DESTDIR)/usr/lib
	mkdir -p $(DESTDIR)/usr/include/sdixml
	cp $(HDR) $(DESTDIR)/usr/include/sdixml

