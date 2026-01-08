#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

int main(void)
{
    char buf[64];

  
    while (1) {
    	  int fd = open("/dev/myadc", O_RDONLY);
    if (fd < 0) {
        perror("Failed to open /dev/myadc");
        return 1;
    }   

        lseek(fd, 0, SEEK_SET);

        int r = read(fd, buf, sizeof(buf) - 1);
       
        buf[r] = '\0';       
        printf("ADC value: %s", buf);
        sleep(2);     
        close(fd); 
    }

    
    
    return 0;
}
