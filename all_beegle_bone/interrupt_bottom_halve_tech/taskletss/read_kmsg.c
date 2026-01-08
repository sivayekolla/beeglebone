#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define BUF_SIZE 1024

int main(void)
{
    int fd;
    char buf[BUF_SIZE];

    fd = open("/dev/kmsg", O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    printf("Listening to kernel messages...\n");

    while (1) {
        int n = read(fd, buf, BUF_SIZE - 1);
        if (n > 0) {
            buf[n] = '\0';

            /* Print only our messages */
            if (strstr(buf, "TASKLET") || strstr(buf, "WORKQUEUE"))
                printf("%s", buf);
        }

        usleep(100000); /* 100 ms */
    }

    close(fd);
    return 0;
}

