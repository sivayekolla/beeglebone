#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define PROC_FILE "/proc/kbd_tasklet_event"

int main()
{
    int fd;
    char buf[128];

    printf("Listening for keyboard events...\n");

    while (1) {
        fd = open(PROC_FILE, O_RDONLY);
        if (fd < 0) {
            perror("open");
            return 1;
        }

        memset(buf, 0, sizeof(buf));
        read(fd, buf, sizeof(buf));
        close(fd);

        if (strlen(buf) > 0)
            printf("EVENT: %s", buf);

        usleep(100000); /* 100ms polling */
    }
    return 0;
}

