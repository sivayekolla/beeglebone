#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define ADC_DEV "/dev/myadc"

int main(void)
{
    int fd;
    char buf[64];
    int n;

    fd = open(ADC_DEV, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    n = read(fd, buf, sizeof(buf));
    if (n < 0) {
        perror("read");
        close(fd);
        return 1;
    }

    printf("ADC Value: %s", buf);

    close(fd);
    return 0;
}

