PREFIX ::= /usr/local
XDG_DATA_HOME ::= ~/.local/share

.PHONY: install
install:
	sudo install -D ./build/Debug/keymon $(PREFIX)/bin/keymon
	install -D -m0644 ./config/keymon.service $(XDG_DATA_HOME)/systemd/user/keymon.service
	sudo install -D -m0644 ./config/keymon.sudoers /etc/sudoers.d/keymon

	sudo install -D -m0644 ./fonts/Rubik-Regular.ttf $(PREFIX)/share/event-horizon/fonts/Rubik-Regular.ttf
	sudo install -D ./build/Debug/launcher $(PREFIX)/bin/launcher

	systemctl --user daemon-reload --wait
	systemctl --user enable --wait keymon.service
	systemctl --user restart keymon.service

.PHONY: uninstall
uninstall:
	systemctl --user disable --now keymon.service

	sudo rm -f $(PREFIX)/bin/keymon
	rm -f $(XDG_DATA_HOME)/systemd/user/keymon.service
	sudo rm -f /etc/sudoers.d/keymon

	sudo rm -f $(PREFIX)/bin/launcher
	sudo rm -rf $(PREFIX)/share/event-horizon/fonts/

	systemctl --user daemon-reload
