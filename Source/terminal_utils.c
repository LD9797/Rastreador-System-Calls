#include "../Headers/terminal_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

//Functions used to switch between canonical mode and regular terminal mode, allowing the "press any key" feature in the when running the -V option.
void set_non_canonical_mode(struct termios* orig_termios) {
  struct termios termios;
  if (tcgetattr(STDIN_FILENO, &termios) < 0) {
	perror("tcgetattr");
	exit(1);
  }
  *orig_termios = termios;
  termios.c_lflag &= ~(ICANON | ECHO);
  if (tcsetattr(STDIN_FILENO, TCSANOW, &termios) < 0) {
	perror("tcsetattr");
	exit(1);
  }
}

//Restores the terminal to regular mode
void restore_terminal_mode(const struct termios* orig_termios) {
  if (tcsetattr(STDIN_FILENO, TCSANOW, orig_termios) < 0) {
	perror("tcsetattr");
	exit(1);
  }
}