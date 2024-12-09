PREFIX ::= /usr/local
CC ::= gcc
CFLAGS ::= -g -Wpedantic

.PHONY: compile
compile:
	mkdir -p ./out
	$(CC) -std=gnu2x $(CFLAGS) $(shell pkg-config --cflags ./build/Debug/generators/raylib.pc) -o ./out/launcher ./src/launcher.c $(shell pkg-config --libs --static ./build/Debug/generators/raylib.pc)
	$(CC) -std=gnu11 $(CFLAGS) -o ./out/keymon ./src/keymon.c -lsystemd

.PHONY: install
install:
	install -D ./third_party/raylib/lib/libraylib.so $(PREFIX)/lib/libraylib.so
	ln -fs libraylib.so $(PREFIX)/lib/libraylib.so.500
	ldconfig
	install -D ./out/keymon $(PREFIX)/bin/keymon
	install -D -m0644 ./config/keymon.service $(PREFIX)/lib/systemd/user/keymon.service
	systemctl --user --machine=$$SUDO_USER@ daemon-reload
	systemctl --user --machine=$$SUDO_USER@ enable --now keymon.service
	install -D -m0644 ./config/keymon.sudoers /etc/sudoers.d/keymon

	install -D -m0644 ./fonts/Rubik-Regular.ttf /usr/share/launcher/fonts/Rubik-Regular.ttf
	install -D ./out/launcher $(PREFIX)/bin/launcher

.PHONY: uninstall
uninstall:
	rm -f $(PREFIX)/lib/libraylib.so
	rm -f $(PREFIX)/lib/libraylib.so.500
	ldconfig
	rm -f $(PREFIX)/bin/keymon
	rm -f $(PREFIX)/lib/systemd/user/keymon.service
	systemctl --user --machine=$$SUDO_USER disable --now keymon.service
	systemctl --user --machine=$$SUDO_USER daemon-reload
	rm -f /etc/sudoers.d/keymon

	rm -f $(PREFIX)/bin/launcher
	rm -rf /usr/share/launcher/fonts/
