PREFIX ?= /usr/local
TARGET := liblinklist.so

all: lib src

lib:
	$(MAKE) -C lib

src: lib
	$(MAKE) -C src

install: all
	install -d $(DESTDIR)$(PREFIX)/lib
	install -m 0755 lib/$(TARGET) $(DESTDIR)$(PREFIX)/lib/
	install -d $(DESTDIR)$(PREFIX)/include
	install -m 0755 lib/linklist.h $(DESTDIR)$(PREFIX)/include
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 0755 src/addressbook $(DESTDIR)$(PREFIX)/bin
	ldconfig

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/lib/$(TARGET)
	rm -f $(DESTDIR)$(PREFIX)/bin/addressbook
	ldconfig

clean:
	$(MAKE) -C lib clean
	$(MAKE) -C src clean

.PHONY: all clean install uninstalln