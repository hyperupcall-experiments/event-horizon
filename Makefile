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
	sudo install -D ./out/keymon $(PREFIX)/bin/keymon
	install -D -m0644 ./config/keymon.service $(XDG_DATA_HOME)/systemd/user/keymon.service
	systemctl --user daemon-reload
	systemctl --user enable --now keymon.service
	systemctl --user restart keymon.service

	sudo install -D -m0644 ./config/keymon.sudoers /etc/sudoers.d/keymon

	sudo install -D -m0644 ./fonts/Rubik-Regular.ttf $(PREFIX)/share/launcher/fonts/Rubik-Regular.ttf
	sudo install -D ./out/launcher $(PREFIX)/bin/launcher

.PHONY: uninstall
uninstall:
	sudo rm -f $(PREFIX)/bin/keymon
	rm -f $(XDG_DATA_HOME)/systemd/user/keymon.service
	systemctl --user disable --now keymon.service
	systemctl --user daemon-reload
	sudo rm -f /etc/sudoers.d/keymon

	sudo rm -f $(PREFIX)/bin/launcher
	sudo rm -rf $(PREFIX)/share/launcher/fonts/
