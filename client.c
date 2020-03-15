#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define FIB_DEV "/dev/fibonacci"

#define MAX_LENGTH 100

void big_fibnum(char *buf, long long sz)
{
    char lower[MAX_LENGTH] = {0};
    snprintf(lower, sizeof(lower), "%s", buf);

    long long upper = sz;
    int carry = 0;

    char scale[MAX_LENGTH] = {0};
    snprintf(scale, sizeof(scale), "%s", "18446744073709551616");

    for (int i = 0; i < strlen(lower) / 2; i++) {
        char tmp = lower[i];
        lower[i] = lower[strlen(lower) - 1 - i];
        lower[strlen(lower) - 1 - i] = tmp;
    }

    for (int i = 0; i < strlen(scale) / 2; i++) {
        char tmp = scale[i];
        scale[i] = scale[strlen(scale) - 1 - i];
        scale[strlen(scale) - 1 - i] = tmp;
    }

    for (int i = 0; i < MAX_LENGTH; i++) {
        if ((upper == 0 || scale[i] == '\0') && lower[i] == '\0' &&
            carry == 0) {
            break;
        }
        int x = 0, y = 0;
        if (scale[i] != '\0')
            x = upper * (scale[i] - '0');
        if (lower[i] != '\0')
            y = (lower[i] - '0');
        int tmp = x + y + carry;
        buf[i] = (char) (tmp % 10 + '0');
        carry = tmp / 10;
    }

    for (int i = 0; i < strlen(buf) / 2; i++) {
        char tmp = buf[i];
        buf[i] = buf[strlen(buf) - 1 - i];
        buf[strlen(buf) - 1 - i] = tmp;
    }
    return;
}

int main()
{
    long long sz;

    char buf[100];
    char write_buf[] = "testing writing";
    int offset = 100; /* TODO: try test something bigger than the limit */

    int fd = open(FIB_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }

    for (int i = 0; i <= offset; i++) {
        sz = write(fd, write_buf, strlen(write_buf));
        printf("Writing to " FIB_DEV ", returned the sequence %lld\n", sz);
    }

    for (int i = 0; i <= offset; i++) {
        lseek(fd, i, SEEK_SET);
        sz = read(fd, buf, 1);
        big_fibnum(buf, sz);
        printf("Reading from " FIB_DEV
               " at offset %d, returned the sequence "
               "%s.\n",
               i, buf);
    }

    for (int i = offset; i >= 0; i--) {
        lseek(fd, i, SEEK_SET);
        sz = read(fd, buf, 1);
        big_fibnum(buf, sz);
        printf("Reading from " FIB_DEV
               " at offset %d, returned the sequence "
               "%s.\n",
               i, buf);
    }

    close(fd);
    return 0;
}
