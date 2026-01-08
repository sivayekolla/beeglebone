#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main(void)
{
	int fd = open("/sys/class/myled/led0/state", O_WRONLY);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	while (1) {
		write(fd, "1", 1);
		printf("LED ON\n");
		sleep(1);

		write(fd, "0", 1);
		printf("LED OFF\n");
		sleep(1);
	}

	close(fd);
	return 0;
}

