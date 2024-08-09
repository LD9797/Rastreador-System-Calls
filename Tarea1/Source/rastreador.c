#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <unistd.h>
#include <termios.h>
#include <stddef.h>
#include <string.h>
#include "../Headers/syscall_utils.h"
#include "../Headers/file_utils.h"
#include "../Headers/terminal_utils.h"

void run_target(const char* program_name, char* const argv[]) {
    if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) < 0) {
        perror("ptrace");
        exit(1);
    }
  	printf("Target started. Program name: %s\n", program_name);
    execv(program_name, argv);
    perror("execv");
    exit(1);
}

void run_tracer(pid_t child_pid, int verbose, int pause) {
    int status;
    struct termios orig_termios;

    if (pause) {
        set_non_canonical_mode(&orig_termios);
    }

    while (1) {
        if (waitpid(child_pid, &status, 0) < 0) {
            perror("waitpid");
            exit(1);
        }
        if (WIFEXITED(status)) break;

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
        long arg4 = regs.r10;
        long arg5 = regs.r8;
        long arg6 = regs.r9;
#else
#error "Unsupported architecture"
#endif

        const char* syscall_name = get_syscall_name(syscall_number);
        if (verbose) {
            printf("System call: %s", syscall_name);
            print_syscall_args(child_pid, syscall_number, arg1, arg2, arg3, arg4, arg5, arg6);
            printf("\n");
        }
        if (pause) {
            printf("System call: %s", syscall_name);
            print_syscall_args(child_pid, syscall_number, arg1, arg2, arg3, arg4, arg5, arg6);
            printf("\n");
        }

        add_syscall_count(syscall_number);

        if (ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL) < 0) {
            perror("ptrace");
            exit(1);
        }

        if (waitpid(child_pid, &status, 0) < 0) {
            perror("waitpid");
            exit(1);
        }
        if (WIFEXITED(status)) break;

        if (ptrace(PTRACE_GETREGS, child_pid, NULL, &regs) < 0) {
            perror("ptrace");
            exit(1);
        }

        if (pause) {
            printf("Press any key to continue...\n");
            fflush(stdout);
            getchar();
        }

        if (ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL) < 0) {
            perror("ptrace");
            exit(1);
        }
    }

    if (pause) {
        restore_terminal_mode(&orig_termios);
    }

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

  	 // Parsing the -v and -V options
     // These options must be passed immediately after the executable name.
     // All other arguments will be passed to the target program.
     // This ensures strict adherence to the specification:
	 // rastreador [opciones rastreador] Prog [opciones de Prog]
  	int i;
  	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-v") == 0) {
		  	verbose = 1;
		} else if (strcmp(argv[i], "-V") == 0) {
		  	pause = 1;
		} else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-H") == 0 ) {
		  	fprintf(stdout, "Usage: %s [-v] [-V] <program to trace> [program args...]\n", argv[0]);
			return 1;
		} else if (argv[i][0] == '-') {
		  	fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
		  	fprintf(stderr, "Usage: %s [-v] [-V] <program to trace> [program args...]\n", argv[0]);
			return 1;
		}
		else {
			break;
		}
  	}

  	if (i >= argc) {
		fprintf(stderr, "No program specified.\n");
	  	fprintf(stderr, "Usage: %s [-v] [-V] <program to trace> [program args...]\n", argv[0]);
		return 1;
  	}

  	const char *program_name = argv[i];

  	if (program_exists(program_name) == 1){
		return 1;
  	}

    pid_t child_pid = fork();
    if (child_pid < 0) {
        perror("fork");
        return 1;
    }
    if (child_pid == 0) {
        run_target(program_name, &argv[i]);
    } else {
        run_tracer(child_pid, verbose, pause);
    }

    return 0;
}