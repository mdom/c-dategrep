VERSION = 0.01

CPPFLAGS = -DVERSION=\"${VERSION}\" -D_XOPEN_SOURCE
CFLAGS = -std=c99 -pedantic -Wall -Os ${CPPFLAGS}
SRC = dategrep.c approxidate.c

PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

dategrep: dategrep.c approxidate.c
	@echo CC -o $@
	@$(CC) $(CFLAGS) -lm -o dategrep $(SRC)

install:
	@echo installing executable file to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f dategrep ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/dategrep
	@echo installing manual page to ${DESTDIR}${MANPREFIX}/man1
	@mkdir -p ${DESTDIR}${MANPREFIX}/man1
	@sed "s/VERSION/${VERSION}/g" < dategrep.1 > ${DESTDIR}${MANPREFIX}/man1/dategrep.1
	@chmod 644 ${DESTDIR}${MANPREFIX}/man1/dategrep.1

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/dategrep
	@echo removing manual page from ${DESTDIR}${MANPREFIX}/man1
	@rm -f ${DESTDIR}${MANPREFIX}/man1/dategrep.1

clean:
	@rm -f dategrep dategrep.o

.PHONY: install uninstall clean
