# Project: libsdixml
# Makefile created by Dev-C++ 4.9.9.2

VERSION = 1.0.0
CXX  = g++
CC   = gcc
TAR  = tar
SRC  = ServiceComponent.cpp Announcement.cpp BitRates.c DrmMuxConfig.cpp Service.cpp RFChannel.cpp Region.cpp Schedule.cpp SDIStream.cpp afs.cpp main.cpp afs.cpp afsmuxlist.cpp afsservicelist.cpp frequencygroup.cpp servicegroup.cpp AMSSConfig.cpp

HDR = afs.h afsmuxlist.h afsservicelist.h Announcement.h BitRates.h DrmMuxConfig.h frequencygroup.h Region.h RFChannel.h Schedule.h SDIStream.h ServiceComponent.h servicegroup.h Service.h version.h AMSSConfig.h

OBJ  = afs.o afsmuxlist.o afsservicelist.o frequencygroup.o servicegroup.o afs.o BitRates.o Announcement.o DrmMuxConfig.o Service.o Region.o RFChannel.o Schedule.o ServiceComponent.o SDIStream.o version.o schema_version.o AMSSConfig.o

INCS =  
CXXINCS = -I../common -I/usr/include/libxml2
STATIC  = libsdixml.a
BIN = test
CXXFLAGS = $(CXXINCS) -Werror -Wall -g3 -Wshadow -Weffc++ -Wsign-promo -Wcomment -std=c++11
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
	$(CXX) $(LIBS) $(CXXFLAGS) main.cpp -o test -L. -L../common -lsdixml -lxml2 -lcommon

tar: $(HDR) $(SRC) version.cpp Makefile
	$(TAR) zcf ../libsdixml-$(VERSION).tar.gz $^ debian

version.cpp: $(HDR) $(SRC) Makefile
	echo '#include "version.h"' > version.cpp
# We should create version.cpp using svn revision.
	echo -n 'const char *libsdixml::svn_version() { return "' >> version.cpp
	echo -n 999 >> version.cpp
	echo '"; }' >> version.cpp

install: $(STATIC)
	mkdir -p $(DESTDIR)/usr/lib
	cp $(STATIC) $(DESTDIR)/usr/lib
	mkdir -p $(DESTDIR)/usr/include/sdixml
	cp $(HDR) $(DESTDIR)/usr/include/sdixml

