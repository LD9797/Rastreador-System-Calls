#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/reg.h>
#include <unistd.h>
#include <errno.h>
#include <seccomp.h>
#include <string.h>
#include <termios.h>
#include "../Headers/utils.h"


// Define a maximum number of syscalls for simplicity
#define MAX_SYSCALLS 1024

// Data structure to store syscall counts
typedef struct {
    long syscall_number;
    int count;
} syscall_count_t;

syscall_count_t syscall_counts[MAX_SYSCALLS];
size_t syscall_count_size = 0;

// Get system call name using libseccomp
const char* get_syscall_name(long syscall_number) {
    return seccomp_syscall_resolve_num_arch(SCMP_ARCH_X86_64, syscall_number);
}

const char* get_error_description(long retval) {
    if (retval >= 0) return "";
    return strerror(-retval);
}

// Add a syscall count entry
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

void run_target(const char *program_name, char *const argv[]) {
  // Allow tracing of this process
  if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) < 0) {
	perror("ptrace");
	exit(EXIT_FAILURE);
  }

  // Print the message after ensuring ptrace succeeded
  printf("Target started. Program name: %s\n", program_name);

  // Replace this process with the target program
  execv(program_name, argv);

  // If execv returns, it must have failed
  perror("execv");
  exit(EXIT_FAILURE);
}

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

void restore_terminal_mode(const struct termios* orig_termios) {
    if (tcsetattr(STDIN_FILENO, TCSANOW, orig_termios) < 0) {
        perror("tcsetattr");
        exit(1);
    }
}

void run_tracer(pid_t child_pid, int verbose, int pause) {
    int status;
    struct termios orig_termios;

    // Set terminal to non-canonical mode if -V is specified
    if (pause) {
        set_non_canonical_mode(&orig_termios);
    }

    while (1) {
        // Wait for child process to change state
        if (waitpid(child_pid, &status, 0) < 0) {
            perror("waitpid");
            exit(1);
        }
        if (WIFEXITED(status)) break;

        // Get the system call number
        struct user_regs_struct regs;
        if (ptrace(PTRACE_GETREGS, child_pid, NULL, &regs) < 0) {
            perror("ptrace");
            exit(1);
        }

#ifdef __x86_64__
        long syscall_number = regs.orig_rax;
        long arg1 = regs.rdi;
        long arg2 = regs.rsi;
        long arg3 = regs.rdx;
#elif __i386__
        long syscall_number = regs.orig_eax;
        long arg1 = regs.ebx;
        long arg2 = regs.ecx;
        long arg3 = regs.edx;
#else
#error "Unsupported architecture"
#endif

        const char* syscall_name = get_syscall_name(syscall_number);
        if (verbose) {
            printf("System call: %s(%ld, %ld, %ld)\n", syscall_name, arg1, arg2, arg3);
        }
        if (pause) {
            printf("System call: %s(%ld, %ld, %ld)\n", syscall_name, arg1, arg2, arg3);
        }

        // Increment the syscall count
        add_syscall_count(syscall_number);

        // Continue the child process until the next system call entry or exit
        if (ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL) < 0) {
            perror("ptrace");
            exit(1);
        }

        // Wait for the system call to exit
        if (waitpid(child_pid, &status, 0) < 0) {
            perror("waitpid");
            exit(1);
        }
        if (WIFEXITED(status)) break;

        // Get the return value of the system call
        if (ptrace(PTRACE_GETREGS, child_pid, NULL, &regs) < 0) {
            perror("ptrace");
            exit(1);
        }

#ifdef __x86_64__
        long retval = regs.rax;
#elif __i386__
        long retval = regs.eax;
#else
#error "Unsupported architecture"
#endif

        //const char* error_desc = get_error_description(retval);
        //printf(" = %ld (%s)\n", retval, error_desc);

        if (pause) {
            printf("Press any key to continue...\n");
            fflush(stdout);  // Ensure the prompt is printed before waiting for input
            getchar();
        }

        // Continue the child process until the next system call entry or exit
        if (ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL) < 0) {
            perror("ptrace");
            exit(1);
        }
    }

    // Restore terminal mode if it was changed
    if (pause) {
        restore_terminal_mode(&orig_termios);
    }

    // Print syscall counts at the end of tracing
    printf("\nSystem Call Counts:\n");
    printf("%-30s %-10s\n", "System Call Name", "Count");
    for (size_t i = 0; i < syscall_count_size; ++i) {
        const char* syscall_name = get_syscall_name(syscall_counts[i].syscall_number);
        printf("%-30s %-10d\n", syscall_name, syscall_counts[i].count);
    }
}

int main(int argc, char* argv[]) {
    int verbose = 0;
    int pause = 0;
    int opt;

    while ((opt = getopt(argc, argv, "vV")) != -1) {
        switch (opt) {
            case 'v':
                verbose = 1;
                break;
            case 'V':
                pause = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-v] [-V] <program to trace> [program args...]\n", argv[0]);
                return 1;
        }
    }

    if (optind >= argc) {
        // If no program is specified, we still need to initialize tracing
        printf("No program specified. Only showing syscall counts after initialization.\n");
    }

    const char* program_name = (optind < argc) ? argv[optind] : NULL;

	// Checking if the program exists.
	if (program_exists(program_name) == 1){
	  return 1;
	}

    pid_t child_pid = fork();
    if (child_pid == 0) {
        // Child process: Run the target program
		run_target(program_name, &argv[optind]);
    } else if (child_pid > 0) {
        // Parent process: Run the tracer
        run_tracer(child_pid, verbose, pause);
    } else {
        perror("fork");
        return 1;
    }

    return 0;
}
