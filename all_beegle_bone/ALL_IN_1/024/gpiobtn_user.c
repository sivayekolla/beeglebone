#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define GPIOBTN_RESET _IO('G', 0)

int main() {
    int fd = open("/dev/gpiobtn", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    char buf[64];

    /* Read press count */
    read(fd, buf, sizeof(buf));
    printf("Read: %s", buf);

    /* Toggle LED */
    write(fd, "1", 1);
    printf("LED toggled via write()\n");

    /* Reset counter */
    ioctl(fd, GPIOBTN_RESET);
    printf("Press count reset via ioctl()\n");

    close(fd);
    return 0;
}

