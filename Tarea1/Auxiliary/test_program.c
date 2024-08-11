#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

//Test program used for the tracer "rastreador"
int main(int argc, char* argv[]){
    //Plain std print.
    printf("Test Program to trace System Calls!\n");

    //Counts the number of arguments recevied and loops over them to print them in the stdout
  	printf("Number of arguments: %d\n", argc);

  	for (int i = 0; i < argc; i++) {
		printf("Argument %d: %s\n", i, argv[i]);
  	}

    //Opens an txt file in current location of the "rastreador"
	printf("Opening test_file.txt.\n");
    int fd = open("test_file.txt", O_CREAT | O_WRONLY, 0644);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    
    //Writes a line to the txt file opened
    printf("Writing to a new test_file2.txt.\n");
    const char* text = "Writing this line to the test_file2.\n";
    if (write(fd, text, 21) != 21) {
        perror("write");
        close(fd);
        return 1;
    }

    close(fd);

	printf("End.\n");
    return 0;
}
