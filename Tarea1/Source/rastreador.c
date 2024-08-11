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

//Starts the tracee program
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

//Run the tracer using ptrace
void run_tracer(pid_t child_pid, int verbose, int pause) {
    //This segment is used to switch between canonical mode and regular termina mode, allowing the "press any key" feature in the when running the -V option.
    int status;
    struct termios orig_termios;
    
    //Validates if -V has been set, the flag is "pause" variable
    if (pause) {
        set_non_canonical_mode(&orig_termios);
    }

    //This loops until the child process has finished running
    while (1) {
        //Make the parent process to wait for child process.
        if (waitpid(child_pid, &status, 0) < 0) {
            perror("waitpid");
            exit(1);
        }
        //Validates if the child process finished correctly
        if (WIFEXITED(status)) break;
        //Uses ptrace to get the system call from the child process and stores it on regs
        struct user_regs_struct regs;
        if (ptrace(PTRACE_GETREGS, child_pid, NULL, &regs) < 0) {
            perror("ptrace");
            exit(1);
        }


//Directive to confirm current architecture, currently just validates if is x86_64, else will return unsupported.
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

        //Uses seccomp library to get the system call name using the system call number capture with ptrace
        const char* syscall_name = get_syscall_name(syscall_number);
        if (verbose) {
            //Print system call name
            printf("System call: %s", syscall_name);
            //Prints the system call name and arguments, using the print_syscall_args function
            print_syscall_args(child_pid, syscall_number, arg1, arg2, arg3, arg4, arg5, arg6);
            printf("\n");
        }
        if (pause) {
            //Print system call name
            printf("System call: %s", syscall_name);
            //Prints the system call name and arguments, using the print_syscall_args function
            print_syscall_args(child_pid, syscall_number, arg1, arg2, arg3, arg4, arg5, arg6);
            printf("\n");
        }

        //Counts the system calls, for the summary table
        add_syscall_count(syscall_number);

        //Uses PTRACE_SYSCALL to have the kernel stop the child process when it sees a system call and checks for any failure in ptrace, in which case will exit the program with an error code. Handles entry.
        if (ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL) < 0) {
            perror("ptrace");
            exit(1);
        }

        //Wait for possible changes in the child process
        if (waitpid(child_pid, &status, 0) < 0) {
            perror("waitpid");
            exit(1);
        }
        if (WIFEXITED(status)) break;

        //Inspects the child process state and saves the argument values in regs
        if (ptrace(PTRACE_GETREGS, child_pid, NULL, &regs) < 0) {
            perror("ptrace");
            exit(1);
        }

        //If the program has been run with -V it pauses and waits for user input
        if (pause) {
            printf("Press any key to continue...\n");
            fflush(stdout);
            getchar();
        }

        //Uses PTRACE_SYSCALL to have the kernel stop the child process when it sees a system call and checks for any failure in ptrace, in which case will exit the program with an error code. Handles exit.
        if (ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL) < 0) {
            perror("ptrace");
            exit(1);
        }
    }

    //If the program has been run with -V restores the terminal to regular mode
    if (pause) {
        restore_terminal_mode(&orig_termios);
    }

    //Prints the summary of the system calls made by the child program
    printf("\nSystem Call Counts:\n");
    printf("%-30s %-10s\n", "System Call Name", "Count");
    for (size_t i = 0; i < syscall_count_size; ++i) {
        const char* syscall_name = get_syscall_name(syscall_counts[i].syscall_number);
        printf("%-30s %-10d\n", syscall_name, syscall_counts[i].count);
    }
}

int main(int argc, char* argv[]) {
    //Flag for -v parameter
    int verbose = 0;
    //Flag for -V parameter
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

    //Validates that a program is passes to "rastreador"
  	if (i >= argc) {
		fprintf(stderr, "No program specified.\n");
	  	fprintf(stderr, "Usage: %s [-v] [-V] <program to trace> [program args...]\n", argv[0]);
		return 1;
  	}
    
    //Gets the program name and validate that it exists
  	const char *program_name = argv[i];

  	if (program_exists(program_name) == 1){
		return 1;
  	}

    //Creates the child process to be traced using fork, if the value is less than 0, indicates that there is an error with the fork.
    pid_t child_pid = fork();
    if (child_pid < 0) {
        perror("fork");
        return 1;
    }
    //Fork returns 0 when the child process is created successfully, if is 0 then runs the program with their repesctive arguments.
    if (child_pid == 0) {
        run_target(program_name, &argv[i]);
    } else {
        run_tracer(child_pid, verbose, pause);
    }

    return 0;
}