VER=0.99.2
SRCPREFIX = /usr/src/ethereal_plugins/trunk
vpath %.h $(SRCPREFIX)/dcp
vpath %.c $(SRCPREFIX)/dcp:$(SRCPREFIX)/mot $(SRCPREFIX)/mdi
LIBS=-ldl `pkg-config glib-2.0 --libs`
CFLAGS+=-I/usr/include/wireshark -DHAVE_CONFIG_H -fpic -Wall -I$(SRCPREFIX)/include
CFLAGS+=`pkg-config glib-2.0 --cflags`
MODULES=edcp.so mdi.so
all : $(MODULES)
crctest : t.o drmcrc.o
	gcc -o crctest -lc $(LIBS) t.o drmcrc.o
mdi.so : crcdrm.o packet-drm-fac.o packet-drm-msc-audio.o packet-drm-msc-data.o packet-drm-msc-text.o packet-drm-msc.o packet-drm-sdc.o packet-drm-mdi.o packet-drm-rsci.o
	gcc -shared -Wl,-soname,$@ -Wl,-export-dynamic  -Wl,--discard-none -o $@ -lc $(LIBS) $^
edcp.so : packet-dcp-etsi.o crcdrm.o rs.o
	gcc -shared -Wl,-soname,$@ -Wl,-export-dynamic  -Wl,--discard-none -o $@ -lc $(LIBS) $^
mot.so : packet-mot.o
	gcc -shared -Wl,-soname,msc.so -Wl,-export-dynamic  -Wl,--discard-none -o mot.so -lc packet-mot.o $(LIBS)
install : $(MODULES)
	cp $^ /usr/lib/wireshark/plugins/$(ETH_VER)
