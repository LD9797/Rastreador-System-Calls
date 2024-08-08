#ifndef TAREA1_TAREA1_HEADERS_SYSCALL_UTILS_H_
#define TAREA1_TAREA1_HEADERS_SYSCALL_UTILS_H_

#include <sys/types.h>
#include <stddef.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>

#define MAX_SYSCALLS 1024
#define MAX_PATH_LENGTH 4096
#define MAX_BUFFER_LENGTH 256  // Limit the size of buffer content to print for write

typedef struct {
  long syscall_number;
  int count;
} syscall_count_t;

extern syscall_count_t syscall_counts[MAX_SYSCALLS];
extern size_t syscall_count_size;

void add_syscall_count(long syscall_number);
void print_syscall_args(pid_t child_pid, long syscall_number, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);

#endif