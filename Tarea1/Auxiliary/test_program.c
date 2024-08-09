#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char* argv[]){
    printf("Test Program!\n");

  	printf("Number of arguments: %d\n", argc);

  	for (int i = 0; i < argc; i++) {
		printf("Argument %d: %s\n", i, argv[i]);
  	}

	printf("Opening test_file.txt.\n");
    int fd = open("test_file.txt", O_CREAT | O_WRONLY, 0644);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    printf("Writing to test_file.txt.\n");
    const char* text = "This is a test file.\n";
    if (write(fd, text, 21) != 21) {
        perror("write");
        close(fd);
        return 1;
    }

    close(fd);

	printf("End.\n");
    return 0;
}
