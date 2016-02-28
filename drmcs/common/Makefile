# Project: libcommon

VERSION = 1.0.0
CXX  = g++
CC   = gcc
TAR  = tar
SRC  = bitvector.cpp bytevector.cpp Cfg.cpp Crc16.cpp persist.cpp timestamp.cpp
HDR = bitvector.h bytevector.h bytev.h Cfg.h Crc16.h crcbytevector.h persist.h timestamp.h

OBJ  = bitvector.o bytevector.o Cfg.o Crc16.o persist.o timestamp.o

LIBS =   
INCS =  
CXXINCS = -I/usr/include/libxml2
STATIC  = libcommon.a
CXXFLAGS = $(CXXINCS)  -Wall -g3 -Wshadow -Weffc++ -Wsign-promo -Wcomment -std=c++11 
CFLAGS = $(INCS)   -Wall -g3
RM = rm -f

.PHONY: all all-before all-after clean clean-custom

all: all-before $(STATIC) all-after

clean: clean-custom
	${RM} $(OBJ) $(STATIC)

$(STATIC): $(OBJ)
	ar r $(STATIC) $(OBJ)
	ranlib $(STATIC)

install: $(STATIC)
	mkdir -p $(DESTDIR)/usr/lib
	cp $(STATIC) $(DESTDIR)/usr/lib
	mkdir -p $(DESTDIR)/usr/include/common
	cp $(HDR) $(DESTDIR)/usr/include/common

