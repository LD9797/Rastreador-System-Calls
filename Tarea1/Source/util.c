#include "../Headers/utils.h"
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

int program_exists(const char* program_name){
  if (access(program_name, X_OK) == -1) {
	perror("access");
	if (errno == ENOENT) {
	  printf("The specified program was not found.\n");
	} else if (errno == EACCES) {
	  printf("The specified program is not executable.\n");
	} else {
	  printf("An error occurred while checking the program.\n");
	}
	return 1;
  }
  return 0;
}
