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

// Get system call name using libseccomp
const char* get_syscall_name(long syscall_number) {
    return seccomp_syscall_resolve_num_arch(SCMP_ARCH_X86_64, syscall_number);
}

void run_target(const char* programname) {
    printf("Target started. Program name: %s\n", programname);
    // Allow tracing of this process
    if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) < 0) {
        perror("ptrace");
        exit(1);
    }
    // Replace this process with the target program
    execl(programname, programname, NULL);
}

void run_tracer(pid_t child_pid, int verbose, int pause) {
    int status;
    while (1) {
        // Wait for child process to change state
        waitpid(child_pid, &status, 0);
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
            printf("Hi systemcall\n");
        }
        printf("System call: %s(%ld, %ld, %ld)\n", syscall_name, arg1, arg2, arg3);

        // Continue the child process until the next system call entry or exit
        if (ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL) < 0) {
            perror("ptrace");
            exit(1);
        }

        // Wait for the system call to exit
        waitpid(child_pid, &status, 0);
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

        printf(" = %ld\n", retval);

        if (pause) {
            printf("Press enter to continue...\n");
            getchar();
        }

        // Continue the child process until the next system call entry or exit
        if (ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL) < 0) {
            perror("ptrace");
            exit(1);
        }
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
                fprintf(stderr, "Usage: %s [-v] [-V] <program to trace>\n", argv[0]);
                return 1;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Usage: %s [-v] [-V] <program to trace>\n", argv[0]);
        return 1;
    }

    const char* programname = argv[optind];

    pid_t child_pid = fork();
    if (child_pid == 0) {
        // Child process: Run the target program
        run_target(programname);
    } else if (child_pid > 0) {
        // Parent process: Run the tracer
        run_tracer(child_pid, verbose, pause);
    } else {
        perror("fork");
        return 1;
    }

    return 0;
}
