PREFIX ::= /usr/local
CC ::= gcc
CFLAGS ::= -g -Wpedantic 

.PHONY: compile
compile:
	mkdir -p ./out
	$(CC) -std=gnu2x $(CFLAGS) -I./third_party/raylib/include -o ./out/launcher ./launcher.c -L./third_party/raylib/lib -lraylib -lm
	$(CC) -std=gnu11 $(CFLAGS) -o ./out/keymon ./keymon.c -lsystemd

.PHONY: install
install:
	install -D ./third_party/raylib/lib/libraylib.so $(PREFIX)/lib/libraylib.so
	ln -fs libraylib.so $(PREFIX)/lib/libraylib.so.500
	ldconfig
	install -D ./out/launcher $(PREFIX)/bin/launcher
	install -D ./out/keymon $(PREFIX)/bin/keymon
	install -D ./keymon.service $(PREFIX)/lib/systemd/system/keymon.service
	systemctl disable keymon.service
	install -D ./keymon.sh /etc/profile.d/keymon.sh

.PHONY: uninstall
uninstall:
	rm -f $(PREFIX)/lib/libraylib.so
	rm -f $(PREFIX)/lib/libraylib.so.500
	ldconfig
	rm -f $(PREFIX)/bin/launcher
	rm -f $(PREFIX)/bin/keymon
	rm -f $(PREFIX)/lib/systemd/system/keymon.service
# systemctl disable --now keymon.service
	rm -f /etc/profile.d/keymon.sh
