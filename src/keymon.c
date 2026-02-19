#define _GNU_SOURCE
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <libgen.h>
#include <limits.h>
#include <linux/input-event-codes.h>
#include <linux/input.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <systemd/sd-daemon.h>
#include <time.h>
#include <unistd.h>

bool is_keymon_process_running(void);
char *get_launcher_exe(void);

int main(int argc, char *argv[]) {
	/**
	 * Hack to ensure that keymon is started no matter what. Prior to
	 * this fix, if the service was started before $DISPLAY and $XAUTHORITY
	 * were available, it would segfault when attempting to display a GUI. Now,
	 * it will continuiously restart until it can access the X11 environment.
	 */
	if (getenv("DISPLAY") == 0) {
		exit(1);
	}

	printf("Starting keymon...\n");
	if (is_keymon_process_running()) {
		fprintf(stderr, "A keymon process already exists. Terminating...\n");
		exit(0);
	}

	char *usage_text = "Usage: keymon <device_paths...> [--help]\n";
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--help") == 0) {
			printf("%s", usage_text);
			exit(0);
		}
	}
	if (argc < 2) {
		fprintf(stderr, "%s", usage_text);
		fprintf(stderr, "Error: Incorrect arguments\n");
		exit(1);
	}

	char *launcher_exe = get_launcher_exe();

#define MAX_DEVICES_LEN 8
	int devices_len = 0;
	char *devices[MAX_DEVICES_LEN];
	for (int i = 1, j = 0; i < argc; ++i) {
		if (strlen(argv[i]) > 0 && argv[i][0] != '-') {
			if (j >= MAX_DEVICES_LEN) {
				fprintf(stderr, "Error: Too many arguments\n");
				exit(1);
			}

			printf("Device %d: %s\n", j + 1, argv[i]);
			devices[j] = argv[i];
			devices_len = ++j;
		}
	}

	printf("Listening to %d device(s)...\n", devices_len);

	struct pollfd fds[MAX_DEVICES_LEN];
	for (int i = 0; i < devices_len; ++i) {
		fds[i].events = POLLIN;
		fds[i].fd = open(devices[i], O_RDONLY | O_NONBLOCK);
		if (fds[i].fd < 0) {
			if (devices_len == 1) {
				fprintf(stderr, "Error: Failed to open file \"%s\"\n", devices[i]);
			} else {
				fprintf(stderr, "Warning: Failed to open file \"%s\"\n", devices[i]);
			}
		}
	}

	const bool is_debug = getenv("DEBUG") == NULL ? false : true;

	const int input_size = sizeof(struct input_event);
	struct input_event input_data;
	struct input_event prev_input_data;
	memset(&input_data, 0, input_size);
	memset(&prev_input_data, 0, input_size);
	int sequential_shifts = 0;

	sd_notify(1, "READY=1");
	while (true) {
		int ret = poll(fds, devices_len, -1);
		if (ret == -1) {
			perror("Poll error");
			exit(1);
		} else if (ret == 0) {
			fprintf(stderr, "Timeout error\n");
			continue;
		}

		for (int i = 0; i < devices_len; ++i) {
			if (fds[i].revents == 0) {
				continue;
			}

			ssize_t r = read(fds[i].fd, &input_data, input_size);
			if (r == -1) {
				// If a device is no longer connected, ignore the error and continue.
				if (errno == ENODEV) {
					continue;
				}
				perror("Failed to read");
			}

			if (input_data.code == EV_SYN || input_data.code == EV_MSC) {
				continue;
			}

			// When moving the mouse, "reset" the variables.
			if (input_data.type == EV_REL) {
				memset(&input_data, 0, input_size);
				memset(&prev_input_data, 0, input_size);
				sequential_shifts = 0;
				if (is_debug) {
					printf("Resetting variables...\n");
				}
				continue;
			}

			// When clicking the mouse, "reset" the variables.
			if (input_data.code == BTN_LEFT || input_data.code == BTN_MIDDLE || input_data.code == BTN_RIGHT) {
				memset(&input_data, 0, input_size);
				memset(&prev_input_data, 0, input_size);
				sequential_shifts = 0;
				if (is_debug) {
					printf("Resetting variables...\n");
				}
				continue;
			}

			// Check for "KEY_ENTER" so debug messages can be separated by whitespace
			// in the console.
			if (is_debug && input_data.code != KEY_ENTER) {
				printf("code=%hu value=%u time=%ld.%06lu\n", input_data.code, input_data.value, input_data.time.tv_sec,
				       input_data.time.tv_usec);
			}

			// Filter out non-key input events.
			if (input_data.type != EV_KEY) {
				continue;
			}

			// When any key is released, "reset" these variables.
			if (input_data.value == 0) {
				memset(&input_data, 0, input_size);
				memset(&prev_input_data, 0, input_size);
				continue;
			}

			if ((input_data.code == KEY_LEFTSHIFT || input_data.code == KEY_RIGHTSHIFT)) {
				if (input_data.value == 1) {
					++sequential_shifts;
				}
			} else {
				sequential_shifts = 0;
			}

			bool pressingMetaCombo = prev_input_data.value == 1 &&
			                         (prev_input_data.code == KEY_LEFTMETA || prev_input_data.code == KEY_RIGHTMETA) &&
			                         input_data.value == 1 &&
			                         (input_data.code == KEY_BACKSPACE || input_data.code == KEY_ENTER ||
			                          input_data.code == KEY_SPACE || input_data.code == KEY_RIGHTALT);
			if (pressingMetaCombo) {
				sequential_shifts = 0;
				pid_t pid = fork();
				if (pid == -1) {
					perror("Failed to fork");
					exit(1);
				} else if (pid > 0) {
					int status;
					if (waitpid(pid, &status, 0) == -1) {
						perror("Failed waitpid");
						exit(1);
					}
				} else {
					if (daemon(false, true) == -1) {
						perror("Failed to daemonize");
						exit(1);
					}

					if (execl(launcher_exe, launcher_exe, (char *)NULL) == -1) {
						perror("Failed to execl");
						exit(1);
					}
				}
			}

			memcpy(&prev_input_data, &input_data, input_size);
			memset(&input_data, 0, input_size);
		}
	}

	for (int i = 0; i < devices_len; ++i) {
		close(fds[i].fd);
	}
}

bool is_keymon_process_running(void) {
	DIR *proc_dir = opendir("/proc");
	if (proc_dir == NULL) {
		perror("Failed to open /proc");
		exit(1);
	}

	struct dirent *entry;
	while ((entry = (struct dirent *)readdir(proc_dir)) != NULL) {
		if (entry->d_type != DT_DIR) {
			continue;
		}

		bool is_process_dir = true;
		for (int i = 0; i < strlen(entry->d_name); i++) {
			if (!isdigit(entry->d_name[i])) {
				is_process_dir = false;
				break;
			}
		}
		if (!is_process_dir) {
			// Skip process-related directories like `/proc/fs`.
			continue;
		}

		char *cmdline_file_path = NULL;
		if (asprintf(&cmdline_file_path, "/proc/%s/cmdline", entry->d_name) == -1) {
			perror("Failed to assign to cmdline_file_path");
			exit(1);
		}

		FILE *cmdline_file = fopen(cmdline_file_path, "r");
		if (!cmdline_file) {
			// Skip processes that we can't read the command line for.
			continue;
		}

		// TODO
		char exe_path[PATH_MAX + 1];
		int i = 0;
		char c;
		while ((c = fgetc(cmdline_file)) != EOF) {
			exe_path[i] = c;
			++i;
		}
		fclose(cmdline_file);
		free(cmdline_file_path);

		char exe_name1[] = "/usr/local/bin/keymon";
		char exe_name2[] = "/usr/bin/keymon";

		if (strcmp(exe_path, exe_name1) == 0 || strcmp(exe_path, exe_name2) == 0) {
			char *end;
			errno = 0;
			long d_pid = strtol(entry->d_name, &end, 10);
			if (end == entry->d_name || *end != '\0') {
				perror("Failed to parse PID");
				exit(1);
			}
			pid_t actual_pid = getpid();
			if (d_pid != actual_pid) {
				if (closedir(proc_dir) == -1) {
					perror("Failed to close /proc");
					exit(1);
				}
				return true;
			}
		}
	}

	if (closedir(proc_dir) == -1) {
		perror("Failed to close /proc");
		exit(1);
	}
	return false;
}

char *get_launcher_exe(void) {
	char exe_path[PATH_MAX + 1];
	memset(exe_path, 0, PATH_MAX + 1);
	ssize_t exe_path_size = readlink("/proc/self/exe", exe_path, PATH_MAX);
	if (exe_path_size == -1) {
		perror("Failed to readlink \"/proc/self/exe\"");
		exit(1);
	}
	if (exe_path_size != strlen(exe_path)) {
		fprintf(stderr, "Failed to set \"exe_path\"(%s)\n", exe_path);
		exit(1);
	}
	char *exe_dirname = dirname(exe_path);
	char *launcher_exe;
	if (asprintf(&launcher_exe, "%s/launcher", exe_dirname) == -1) {
		perror("Failed to assign launcher_exe");
		exit(1);
	};
	printf("launcher_exe: %s\n", launcher_exe);

	return launcher_exe;
}
