#ifndef TAREA1_TAREA1_HEADERS_TERMINAL_UTILS_H_
#define TAREA1_TAREA1_HEADERS_TERMINAL_UTILS_H_

#include <termios.h>

void set_non_canonical_mode(struct termios* orig_termios);
void restore_terminal_mode(const struct termios* orig_termios);

#endif //TAREA1_TAREA1_HEADERS_TERMINAL_UTILS_H_
