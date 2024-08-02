#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main() {
    printf("Hello, world!\n");

    int fd = open("test_file.txt", O_CREAT | O_WRONLY, 0644);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    const char* text = "This is a test file.\n";
    if (write(fd, text, 21) != 21) {
        perror("write");
        close(fd);
        return 1;
    }

    close(fd);

    return 0;
}
