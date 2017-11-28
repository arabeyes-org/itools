# $Id: Makefile.in,v 1.3 2004/12/27 16:20:02 thamer Exp $

PREFIX		= /home/xs/daten/debian/arabeyes/itl/itools/debian/itools/usr

CC		= gcc
CFLAGS		= -g -O2
LDFLAGS		= 
CPPFLAGS	= 
LIBS		= -lm  -litl

PROGRAMS	= ical idate ipraytime

all: $(PROGRAMS)

ical: ical.c
	@echo "==> Building ical..."
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIBS) ical.c -o ical

idate: idate.c
	@echo "==> Building idate..."
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIBS) idate.c -o idate

ipraytime: ipraytime.c
	@echo "==> Building ipraytime..."
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIBS) ipraytime.c -o ipraytime

static: ical.c idate.c ipraytime.c
	$(CC) $(CFLAGS) $(LDFLAGS) ical.c -o ical $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) idate.c -o idate $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) ipraytime.c -o ipraytime $(LIBS)

install: all
	@echo "==> Installing compiled binaries & manpages..."
	test -d $(PREFIX)/bin \
	|| mkdir -p $(PREFIX)/bin \
        || exit 1;
	cp ical $(PREFIX)/bin
	cp idate $(PREFIX)/bin
	cp ipraytime $(PREFIX)/bin
	cp ireminder.pl $(PREFIX)/bin
	test -d $(PREFIX)/man/man1 \
	|| mkdir -p $(PREFIX)/man/man1\
	|| exit 1;
	cp doc/ical.1 $(PREFIX)/man/man1
	cp doc/idate.1 $(PREFIX)/man/man1
	cp doc/ipraytime.1 $(PREFIX)/man/man1


debinstall: all
	@echo "==> Installing compiled binaries & manpages..."
	test -d $(PREFIX)/bin \
	|| mkdir -p $(PREFIX)/bin \
        || exit 1;
	cp ical $(PREFIX)/bin
	cp idate $(PREFIX)/bin
	cp ipraytime $(PREFIX)/bin
	cp ipraytime $(PREFIX)/bin
	cp ireminder.pl $(PREFIX)/bin


uninstall:
	@echo "==> Uninstalling ITL-tools various components..."
	rm -f $(PREFIX)/bin/ical $(PREFIX)/man/man1/ical.1
	rm -f $(PREFIX)/bin/idate $(PREFIX)/man/man1/idate.1
	rm -f $(PREFIX)/bin/ipraytime $(PREFIX)/man/man1/ipraytime.1
	rm -f $(PREFIX)/bin/ireminder.pl
clean:
	rm -f *.o *~ ical idate ipraytime

distclean: autogen.sh clean
	fakeroot $(MAKE) -k -f debian/rules clean
	./autogen.sh clean
