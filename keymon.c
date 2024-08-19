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
#include <systemd/sd-daemon.h>
#include <time.h>
#include <unistd.h>

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

int main(int argc, char *argv[]) {
	if (is_keymon_process_running()) {
		printf("keymon process already exists. Terminating.\n");
		exit(0);
	}

	char *input_dev = "/dev/input/by-id/usb-Microsoft_Surface_Type_Cover-event-kbd";

	const bool isDebug = getenv("DEBUG") == NULL ? false : true;

	time_t start_time = time(0);

	struct pollfd fds[1];
	fds[0].fd = open(input_dev, O_RDONLY | O_NONBLOCK);
	if (fds[0].fd < 0) {
		printf("Failed to open file '%s'\n", input_dev);
		exit(1);
	}

	const int input_size = sizeof(struct input_event);

	struct input_event input_data;
	memset(&input_data, 0, input_size);
	struct input_event previous_input_data;
	memset(&previous_input_data, 0, input_size);

	fds[0].events = POLLIN;

	sd_notify(1, "READY=1");
	while (true) {
		time_t current_time = time(0);
		if (current_time - start_time < 2) {
			continue;
		}

		int ret = poll(fds, 1, -1);
		if (ret == -1) {
			perror("poll error");
			exit(1);
		} else if (ret == 0) {
			printf("timeout\n");
			continue;
		}

		if (!fds[0].revents) {
			printf("error\n");
			continue;
		}

		ssize_t r = read(fds[0].fd, &input_data, input_size);
		if (r < 0) {
			printf("error %d\n", (int)r);
			break;
		}

		if (input_data.type != EV_KEY) {
			continue;
		}

		if (input_data.value == 0) {
			memset(&input_data, 0, input_size);
			memset(&previous_input_data, 0, input_size);
			continue;
		}

		if (input_data.code == KEY_ENTER) {
			continue;
		}

		if (isDebug) {
			printf("code=%hu value=%u time=%ld.%06lu\n", input_data.code, input_data.value, input_data.time.tv_sec,
			       input_data.time.tv_usec);
		}

		// TODO
		if (input_data.code == KEY_LEFTSHIFT && previous_input_data.time.tv_sec > 0) {
			double time_diff = previous_input_data.time.tv_sec - input_data.time.tv_sec;
		}

		if ((input_data.value == 1 && input_data.code == KEY_LEFTSHIFT || input_data.code == KEY_RIGHTSHIFT) &&
		    (previous_input_data.value == 1 && previous_input_data.code == KEY_LEFTSHIFT ||
		     previous_input_data.code == KEY_RIGHTSHIFT)) {
			printf("forking...\n");
			pid_t pid = fork();
			if (pid == -1) {
				perror("Failed to fork");
				exit(1);
			} else if (pid > 0) {
				int status;
				waitpid(pid, &status, 0);
				printf("keymon finished launching: %i\n", status);
			} else if (pid == 0) {
				char *cwd = getcwd(NULL, 0);
				if (cwd == NULL) {
					perror("Failed to get current working directory");
					exit(1);
				}
				char *launcher_exec;
				char *dirname = get_current_dir_name();
				if (strcmp(dirname, "launcher") == 0) {
					launcher_exec = "./out/launcher";
				} else {
					launcher_exec = "/usr/local/bin/launcher";
				}

				char *exec;
				if (asprintf(&exec, "systemd-run  --quiet --machine=edwin@ --user --collect --pipe \"%s\"",
				             launcher_exec) == -1) {
					perror("Failed to assign exec");
					exit(1);
				}
				int status = system(exec);
				exit(status);
			}
		}

		memcpy(&previous_input_data, &input_data, input_size);
		memset(&input_data, 0, input_size);
	}
	close(fds[0].fd);
}
