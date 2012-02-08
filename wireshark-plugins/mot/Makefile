LIBS=-ldl -lc `pkg-config glib-2.0 --libs`
CFLAGS+=-DHAVE_CONFIG_H -fpic -Wall -g 
CFLAGS+=`pkg-config glib-2.0 --cflags` -I/usr/src/wireshark
OBJS=packet-mot.o
mot.so : $(OBJS)
	$(CC) -shared -Wl,-soname,$@ -Wl,-export-dynamic  -Wl,--discard-none -o $@ $(LIBS) $^
clean:
	rm mot.so packet-mot.o
