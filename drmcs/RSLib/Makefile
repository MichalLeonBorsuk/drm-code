# Project: librs
# Makefile created by Dev-C++ 4.9.9.2
VERSION=1.0.0

CXX  = g++
CC   = gcc
TAR  = tar
OBJ  = FiniteField.o FiniteFieldElement.o FiniteFieldPolynomial.o GaloisField.o GaloisFieldElement.o GaloisFieldElementNonzero.o GaloisFieldElementZero.o GaloisFieldGenerator.o RSAlgebra.o RSCodePFT.o WorkingFiniteFieldElement.o $(RES)
LINKOBJ  = FiniteField.o FiniteFieldElement.o FiniteFieldPolynomial.o GaloisField.o GaloisFieldElement.o GaloisFieldElementNonzero.o GaloisFieldElementZero.o GaloisFieldGenerator.o RSAlgebra.o RSCodePFT.o WorkingFiniteFieldElement.o $(RES)
SRC  = FiniteField.cpp FiniteField.h FiniteFieldElement.cpp FiniteFieldElement.h FiniteFieldPolynomial.cpp FiniteFieldPolynomial.h GaloisField.cpp GaloisField.h GaloisFieldElement.cpp GaloisFieldElement.h GaloisFieldElementNonzero.cpp GaloisFieldElementNonzero.h GaloisFieldElementZero.cpp GaloisFieldElementZero.h GaloisFieldGenerator.cpp GaloisFieldGenerator.h RSAlgebra.cpp RSAlgebra.h RSCodePFT.cpp RSCodePFT.h WorkingFiniteFieldElement.cpp WorkingFiniteFieldElement.h
LIBS =   
INCS =  
CXXINCS =  
BIN  = librs.a
CFLAGS = $(INCS)  
CXXFLAGS = $(CXXINCS)  -Werror -Wall -g3 -Wshadow -Weffc++ -Wsign-promo -Wcomment -std=c++11 
RM = rm -f

.PHONY: all all-before all-after clean clean-custom

all: all-before librs.a all-after


clean: clean-custom
	${RM} $(OBJ) $(BIN)

$(BIN): $(LINKOBJ)
	ar r $(BIN) $(LINKOBJ)
	ranlib $(BIN)

rslibtest: RSLib.cpp rs.c librs.a
	$(COMPILE.cpp) RSLib.cpp rs.c -o rslibtest -L. -lrs

tar: $(SRC)
	$(TAR) zcf librs-$(VERSION).tar.gz $(SRC) Makefile.linux

install: $(BIN)
	mkdir -p $(DESTDIR)/usr/lib
	cp $(BIN) $(DESTDIR)/usr/lib
	mkdir -p $(DESTDIR)/usr/include
	cp RSCodePFT.h $(DESTDIR)/usr/include
