#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <time.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <systemd/sd-daemon.h>

int main(int argc, char *argv[]) {
	time_t start_time = time(0);

	const int timeout_ms = -1;
	char* input_dev = "/dev/input/by-id/usb-Microsoft_Surface_Type_Cover-event-kbd";
	int st;
	int ret;
	struct pollfd fds[1];
	fds[0].fd = open(input_dev, O_RDONLY|O_NONBLOCK);
	if(fds[0].fd < 0) {
		printf("Failed to open file '%s'\n",input_dev);
		exit(1);
	}

	const int input_size = sizeof(struct input_event);

	struct input_event input_data;
	memset(&input_data, 0, input_size);
	struct input_event previous_input_data;
	memset(&previous_input_data, 0, input_size);

	fds[0].events = POLLIN;
	sd_notify(1, "READY=1");
	printf("before segfault\n");
	fflush(stdin);
	while (true) {
		time_t current_time = time(0);
		if (current_time - start_time < 2) {
			continue;
		}

		ret = poll(fds, 1, timeout_ms);
		if(!(ret > 0)) {
			printf("timeout\n");
			continue;
		}

		if(!fds[0].revents) {
			printf("error\n");
			continue;
		}

		ssize_t r = read(fds[0].fd, &input_data, input_size);
		if(r < 0) {
			printf("error %d\n",(int)r);
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

		// printf(
		// 	"code=%hu value=%u time=%ld.%06lu\n",
		// 	input_data.code,
		// 	input_data.value,
		// 	input_data.time.tv_sec,
		// 	input_data.time.tv_usec
		// );

		if (input_data.code == KEY_LEFTSHIFT && previous_input_data.time.tv_sec > 0) {
			double time_diff = previous_input_data.time.tv_sec - input_data.time.tv_sec;
			// printf("time: %f\n", time_diff);
		}

		if (
			(input_data.value == 1 && input_data.code ==  KEY_LEFTSHIFT || input_data.code == KEY_RIGHTSHIFT) &&
			(previous_input_data.value == 1 && previous_input_data.code ==  KEY_LEFTSHIFT || previous_input_data.code == KEY_RIGHTSHIFT)
		) {
			printf("FORKINGGG2\n");
			pid_t pid = fork();
			if (pid == -1) {
				perror("Failed to fork");
				exit(1);
			} else if (pid > 0) {
				int status;
				waitpid(pid, &status, 0);
				printf("Done executing launch: %i\n", status);
			} else if(pid == 0) {
				int status = system("su edwin -c \"launcher\"");
				exit(status);
			}
		}

		memcpy(&previous_input_data, &input_data, input_size);
		memset(&input_data, 0, input_size);
	}
	close(fds[0].fd);
}
