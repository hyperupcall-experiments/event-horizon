#define _GNU_SOURCE
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <linux/input-event-codes.h>
#include <linux/input.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <libgen.h>
#include <systemd/sd-daemon.h>
#include <time.h>
#include <unistd.h>

bool is_keymon_process_running();

int main(int argc, char *argv[]) {
	printf("Starting keymon...\n");
	if (is_keymon_process_running()) {
		// TODO: Print to standard error for these types of messages.
		printf("A keymon process already exists. Terminating...\n");
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

#define MAX_DEVICES_LEN 8
	int devices_len = 0;
	char* devices[MAX_DEVICES_LEN];
	for (int i = 1, j = 0; i < argc; ++i) {
		if (strlen(argv[i]) > 0 && argv[i][0] != '-') {
			if (j >= MAX_DEVICES_LEN) {
				fprintf(stderr, "Error: Too many arguments");
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
				printf("Error: Failed to open file \"%s\"\n", devices[i]);
				exit(1);
			} else {
				printf("Warning: Failed to open file \"%s\"\n", devices[i]);
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
			printf("Timeout error\n");
			continue;
		}

		for (int i = 0; i < devices_len; ++i) {
			if (fds[i].revents == 0) {
				continue;
			}

			ssize_t r = read(fds[i].fd, &input_data, input_size);
			if (r < 0) {
				printf("Read error %d\n", (int)r);
				break;
			}
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

			// When debugging, it is useful to separate debug messages by whitespace.
			if (is_debug && input_data.code != KEY_ENTER) {
				printf("code=%hu value=%u time=%ld.%06lu\n", input_data.code, input_data.value, input_data.time.tv_sec,
							input_data.time.tv_usec);
			}

			if ((input_data.code == KEY_LEFTSHIFT || input_data.code == KEY_RIGHTSHIFT)) {
				if (input_data.value == 1) {
					++sequential_shifts;
				}
			} else {
				sequential_shifts = 0;
			}

			/**
			* There are two ways to launch the launcher:
			* 1. Press both left shift and right shift at the same time
			* 2. Press either left shift or right shift three times
			*/
			bool pressingBothShifts = (input_data.value == 1 && (input_data.code == KEY_LEFTSHIFT || input_data.code == KEY_RIGHTSHIFT)) &&
				(prev_input_data.value == 1 && (prev_input_data.code == KEY_LEFTSHIFT ||
				prev_input_data.code == KEY_RIGHTSHIFT));
			bool pressedShiftThreeTimes = sequential_shifts >= 3;
			if (pressingBothShifts || pressedShiftThreeTimes) {
				sequential_shifts = 0;
				printf("Launching launcher...\n");
				pid_t pid = fork();
				if (pid == -1) {
					perror("Failed to fork");
					exit(1);
				} else if (pid > 0) {
					int status;
					waitpid(pid, &status, 0);
					printf("Child terminated with status: %i\n", status);
				} else if (pid == 0) {
					int did_daemonize = daemon(false, true);
					if (did_daemonize != 0) {
						perror("Failed to daemonize");
						exit(1);
					}

					char exe_path[PATH_MAX + 1];
					exe_path[PATH_MAX] = '\0';
					ssize_t exe_path_size = readlink("/proc/self/exe", exe_path, PATH_MAX + 1);
					if (exe_path_size == -1) {
						perror("Failed to readlink \"/proc/self/exe\"");
						exit(1);
					}
					if (exe_path_size != strlen(exe_path)) {
						perror("Failed to set \"exe_path\"");
						exit(1);
					}
					char *exe_dirname = dirname(exe_path);
					char *launcher_exe;
					if (asprintf(&launcher_exe, "%s/launcher", exe_dirname) == -1) {
						perror("Failed to assign launcher_exe");
						exit(1);
					};
					printf("launcher_exe: %s\n", launcher_exe);

					// Run the launcher if "launcher" exists in the same directory as "keymon".
					struct stat st;
					if (stat(launcher_exe, &st) != -1) {
						char *exec;
						if (asprintf(&exec, "systemd-run  --quiet --machine=edwin@ --user --collect --pipe \"%s\"", launcher_exe) == -1) {
							perror("Failed to assign exec");
							exit(1);
						}
						int status = system(exec);
						exit(0);
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

bool is_keymon_process_running() {
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

		bool is_proc_dir = true;
		for (int i = 0; i < strlen(entry->d_name); i++) {
			if (!isdigit(entry->d_name[i])) {
				is_proc_dir = false;
				break;
			}
		}
		if (!is_proc_dir) {
			// Skip non-directory entries like `/proc/mounts`.
			continue;
		}

		char *cmdline_file_path;
		if (asprintf(&cmdline_file_path, "/proc/%s/cmdline", entry->d_name) == -1) {
			perror("Failed to assign cmdline_file_path");
			exit(1);
		}

		FILE *cmdline_file = fopen(cmdline_file_path, "r");
		if (!cmdline_file) {
			// Skip processes that we can't read the command line for.
			continue;
		}
		char exe_path[PATH_MAX + 1];
		// if (strcpy(exe_path, cmdline_file) == NULL) {
		// 	perror("Failed to read cmdline file");
		// 	exit(1);
		// }
		int i = 0;
		char c;
		while ((c = fgetc(cmdline_file)) != EOF) {
			exe_path[i] = c;
			++i;
		}
		fclose(cmdline_file);

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
				return true;
			}
		}
	}

	return false;
}
