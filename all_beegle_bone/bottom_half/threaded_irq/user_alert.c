#include <stdio.h>
#include <unistd.h>

int main()
{
    FILE *fp;
    char buf[256];

    fp = fopen("/var/log/alert.log", "r");
    if (!fp)
    {
        perror("fopen");
        return 1;
    }

    fseek(fp, 0, SEEK_END);

    while (1)
    {
        while (fgets(buf, sizeof(buf), fp))
        {
            printf("ALERT: %s", buf);
            fflush(stdout);
        }
        sleep(1);
    }

    fclose(fp);
    return 0;
}

