CFLAGS?=-Wall
LDFLAGS?=-static
gps_simple:	gps_simple.o devtag-allinone.o
install:	gps_simple
	strip gps_simple
	mkdir -p $(DESTDIR)/$(PREFIX)/bin
	cp -p gps_simple $(DESTDIR)/$(PREFIX)/bin
clean:
	rm -f *.o gps_simple

