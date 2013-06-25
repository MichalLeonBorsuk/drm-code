# Project: mdigen
VERSION = 0.1.1

CPP = g++
CC  = gcc
TAR = tar
OBJ = svn_version.o mdigenerator.o TextEncode.o TextFileSCE.o PacketSCE.o TranslatingTextSCE.o TickerTextSCE.o Ticker.o wirelist.o fac.o StreamMux.o msc.o drmmux.o mdiout.o packetencoder.o CTAudioMemSCE.o PacketStreamMux.o TDMStreamMux.o FileSCE.o MonitoredFile.o JML.o DUFileSCE.o JMLSCE.o DataGroupEncoder.o PacketFileSCE.o

LIVEOBJ  = CTAudioFileSCE.o TimedTextSCE.o TcpDataSCE.o sndfileinput.o oss.o alsa.o udpTextSCE.o MOTSCE.o MotObject.o MOTEncoder.o MotDirectory.o
LIBS +=  -L. -lcommon -ldcp -lsdixml -lsdc -lrs -lstdc++ -lxml2 -lm -lrt -lz -lpcap -lfam
LIVELIBS =  -lasound -lsndfile
CXXINCS =  -I/usr/include/libxml2 -I.
CXXINCS +=  -I../sdixml
CXXINCS +=  -I../dcp
CXXINCS +=  -I../common
CXXINCS +=  -I../sdc
CXXINCS +=  -I../RSLib
CXXFLAGS = $(CXXINCS) -Wall
CFLAGS = $(INCS) -Wall 

.PHONY: all all-before all-after clean clean-custom svn_version.cpp

all: all-before mdigen all-after

clean: clean-custom
	rm -f mditestgenerator mdigen *.o

mditestgenerator: $(OBJ) main.o
	$(CPP) $(CXXFLAGS) BBCSceFactory.cpp $^ -o $@ $(LIBS)
	strip $@

mdigen: $(OBJ) $(LIVEOBJ) main.o
	$(CPP) $(CXXFLAGS) -DNOT_JUST_A_TEST_GENERATOR BBCSceFactory.cpp $^ -o $@ $(LIBS) $(LIVELIBS)
	strip $@

HDR = sound.h alsa.h sndfileinput.h oss.h ServiceComponentEncoder.h mdigenerator.h platform.h udpTextSCE.h TextEncode.h TextFileSCE.h CTAudioFileSCE.h TranslatingTextSCE.h TimedTextSCE.h BBCSceFactory.h TickerTextSCE.h Ticker.h wirelist.h fac.h StreamMux.h PacketStreamMux.h TDMStreamMux.h msc.h drmmux.h mdiout.h TcpDataSCE.h packetencoder.h PacketSCE.h PacketFileSCE.h DataGroupEncoder.h motdefs.h MOTSCE.h MotObject.h MOTEncoder.h MotDirectory.h wirelist.c FileSCE.h CTAudioMemSCE.h MonitoredFile.h JML.h DUFileSCE.h JMLSCE.h

SRC  = alsa.cpp sndfileinput.cpp oss.cpp mdigenerator.cpp udpTextSCE.cpp TextEncode.cpp TextFileSCE.cpp CTAudioFileSCE.cpp TranslatingTextSCE.cpp TimedTextSCE.cpp BBCSceFactory.cpp TickerTextSCE.cpp Ticker.cpp wirelist.c fac.cpp TDMStreamMux.cpp StreamMux.cpp msc.cpp drmmux.cpp mdiout.cpp TcpDataSCE.cpp packetencoder.cpp PacketSCE.cpp PacketFileSCE.cpp DataGroupEncoder.cpp MOTSCE.cpp MotObject.cpp MOTEncoder.cpp MotDirectory.cpp PacketStreamMux.cpp FileSCE.cpp CTAudioMemSCE.cpp MonitoredFile.cpp JML.cpp DUFileSCE.cpp JMLSCE.cpp

tar: $(HDR) $(SRC) svn_version.cpp Makefile main.cpp
	$(TAR) zcf ../mdigen-$(VERSION).tar.gz $^ debian

svn_version.cpp: $(SRC) $(HDR)
	echo -n 'const char* svn_version() { const char* SVN_Version = "' > svn_version.cpp
	echo -n 999                   >> svn_version.cpp
	echo '"; return SVN_Version; }'   >> svn_version.cpp

install: mdigen
	cp $^ $(DESTDIR)/usr/bin
