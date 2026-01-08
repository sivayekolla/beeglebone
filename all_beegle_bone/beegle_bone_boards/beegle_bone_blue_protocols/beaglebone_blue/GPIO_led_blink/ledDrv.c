#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>


int main() {

    int fd = open("/sys/class/myled/led0/state", O_WRONLY);
    if (fd < 0) {
        perror("Failed to open LED sysfs file");
        return 1;
    }

    while (1) {
        // Turn LED ON
        if (write(fd, "1", 1) < 0) {
            perror("Failed to write LED ON");
            break;
        }
else
{
	printf("Led on...\n");
}
        sleep(1); // wait 1 second

        // Turn LED OFF
        if (write(fd, "0", 1) < 0) {
            perror("Failed to write LED OFF");
            break;
        }
	else
	{
		printf("Led off...\n");
	}
        sleep(1); // wait 1 second
    }

    close(fd);
    return 0;
}

