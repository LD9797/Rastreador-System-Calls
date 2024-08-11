#include "../Headers/syscall_utils.h"
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <seccomp.h>

//Set the maximum number of system calls
syscall_count_t syscall_counts[MAX_SYSCALLS];
size_t syscall_count_size = 0;

//Function used to get the name of the system call using the library seccomp, this avoids the need to manually translate the name of the system calls
const char* get_syscall_name(long syscall_number) {
  return seccomp_syscall_resolve_num_arch(SCMP_ARCH_X86_64, syscall_number);
}

//Helper function to get the data from the arguments of the system calls from memory
void read_from_child(pid_t child_pid, unsigned long addr, size_t length, char* buf, int read_until_null) {
  //Handles the number of bytes read
  size_t len = 0;
  unsigned long word;
  //Reads the data from the child process using PTRACE_PEEKDATA
  while (len < length) {
	errno = 0;
	word = ptrace(PTRACE_PEEKDATA, child_pid, addr + len, NULL);
	if (errno != 0) {
	  break;
	}
	//Copies the data to the var buf
	size_t bytes_to_copy = sizeof(word);
	if (len + bytes_to_copy > length) {
	  bytes_to_copy = length - len;
	}
	memcpy(buf + len, &word, bytes_to_copy);
	//Valites for possible null
	if (read_until_null && memchr(&word, 0, bytes_to_copy) != NULL) {
	  break;
	}
	len += bytes_to_copy;
  }
  if (read_until_null) {
	buf[length - 1] = '\0';
  }
}

//Function used to decode the arguments of the system call "openat"
char* read_string_from_child(pid_t child_pid, unsigned long addr) {
  static char buf[MAX_PATH_LENGTH];
  read_from_child(child_pid, addr, MAX_PATH_LENGTH, buf, 1);
  return buf;
}

//Function used to decode the arguments of the system call "write" and "read"
void read_buffer_from_child(pid_t child_pid, unsigned long addr, size_t length, char* buf) {
  read_from_child(child_pid, addr, length, buf, 0);
}

//Function to count the number of times that a system call appears, used to create the summary table
void add_syscall_count(long syscall_number) {
  for (size_t i = 0; i < syscall_count_size; ++i) {
	if (syscall_counts[i].syscall_number == syscall_number) {
	  syscall_counts[i].count++;
	  return;
	}
  }
  if (syscall_count_size < MAX_SYSCALLS) {
	syscall_counts[syscall_count_size].syscall_number = syscall_number;
	syscall_counts[syscall_count_size].count = 1;
	syscall_count_size++;
  }
}

//Function to print more details on specific system calls arguments, manually checks the system call and then uses their respective functions to decode the arguments into a more readable format.
void print_syscall_args(pid_t child_pid, long syscall_number, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
  switch (syscall_number) {
	case SYS_openat: {
	  const char* pathname = read_string_from_child(child_pid, arg2);
	  printf(" (dirfd=%ld, pathname=\"%s\", flags=%ld, mode=%ld)", arg1, pathname, arg3, arg4);
	  break;
	}
	case SYS_write: {
	  char buffer[MAX_BUFFER_LENGTH + 1] = {0};
	  size_t length = (arg3 < MAX_BUFFER_LENGTH) ? arg3 : MAX_BUFFER_LENGTH;
	  read_buffer_from_child(child_pid, arg2, length, buffer);
	  printf(" (fd=%ld, buf=\"%.*s\", count=%ld)", arg1, (int)length, buffer, arg3);
	  break;
	}
	case SYS_read: {
	  char buffer[MAX_BUFFER_LENGTH + 1] = {0};
	  size_t length = (arg3 < MAX_BUFFER_LENGTH) ? arg3 : MAX_BUFFER_LENGTH;
	  read_buffer_from_child(child_pid, arg2, length, buffer);
	  printf(" (fd=%ld, buf=\"%.*s\", count=%ld)", arg1, (int)length, buffer, arg3);
	  break;
	}
	default:
	  printf(" (%ld, %ld, %ld, %ld, %ld, %ld)", arg1, arg2, arg3, arg4, arg5, arg6);
	  break;
  }
}