#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>

#define DEVICE "/dev/ioctldev"
#define MAJOR_NUM 240
#define IOCTL_RESET_BUFFER _IO(MAJOR_NUM, 0)
#define IOCTL_GET_COUNT    _IOR(MAJOR_NUM, 1, int)

int main() {
    int fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    char msg[] = "Hello ioctl!";
    write(fd, msg, strlen(msg));

    int count;
    ioctl(fd, IOCTL_GET_COUNT, &count);
    printf("Write count = %d\n", count);

    ioctl(fd, IOCTL_RESET_BUFFER);
    printf("Buffer reset.\n");


 
    close(fd);
    return 0;
}
