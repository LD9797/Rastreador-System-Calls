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
#include <stddef.h>
#include <sys/syscall.h>
#include <fcntl.h>

#define MAX_SYSCALLS 1024
#define MAX_PATH_LENGTH 4096
#define MAX_BUFFER_LENGTH 256  // Limit the size of buffer content to print for write

typedef struct {
    long syscall_number;
    int count;
} syscall_count_t;

syscall_count_t syscall_counts[MAX_SYSCALLS];
size_t syscall_count_size = 0;

const char* get_syscall_name(long syscall_number) {
    return seccomp_syscall_resolve_num_arch(SCMP_ARCH_X86_64, syscall_number);
}

const char* get_error_description(long retval) {
    if (retval >= 0) return "";
    return strerror(-retval);
}

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

void run_target(const char* programname, char* const argv[]) {
    printf("Target started. Program name: %s\n", programname);
    if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) < 0) {
        perror("ptrace");
        exit(1);
    }
    execv(programname, argv);
    perror("execv");
    exit(1);
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

void read_from_child(pid_t child_pid, unsigned long addr, size_t length, char* buf, int read_until_null) {
    size_t len = 0;
    unsigned long word;
    while (len < length) {
        errno = 0;
        word = ptrace(PTRACE_PEEKDATA, child_pid, addr + len, NULL);
        if (errno != 0) {
            break;
        }
        size_t bytes_to_copy = sizeof(word);
        if (len + bytes_to_copy > length) {
            bytes_to_copy = length - len;
        }
        memcpy(buf + len, &word, bytes_to_copy);
        if (read_until_null && memchr(&word, 0, bytes_to_copy) != NULL) {
            break;
        }
        len += bytes_to_copy;
    }
    if (read_until_null) {
        buf[length - 1] = '\0';
    }
}

char* read_string_from_child(pid_t child_pid, unsigned long addr) {
    static char buf[MAX_PATH_LENGTH];
    read_from_child(child_pid, addr, MAX_PATH_LENGTH, buf, 1);
    return buf;
}

void read_buffer_from_child(pid_t child_pid, unsigned long addr, size_t length, char* buf) {
    read_from_child(child_pid, addr, length, buf, 0);
}

void print_syscall_args(pid_t child_pid, long syscall_number, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    switch (syscall_number) {
        case SYS_openat: {
            const char* pathname = read_string_from_child(child_pid, arg2);
            //const char* pathname = read_buffer_from_child(child_pid, arg2);
            if (arg1 == AT_FDCWD) {
                printf(" (dirfd=AT_FDCWD, pathname=\"%s\", flags=%ld, mode=%ld)", pathname, arg3, arg4);
            } else {
                printf(" (dirfd=%ld, pathname=\"%s\", flags=%ld, mode=%ld)", arg1, pathname, arg3, arg4);
            }
            break;
        }
        case SYS_write: {
            char buffer[MAX_BUFFER_LENGTH + 1] = {0};
            size_t length = (arg3 < MAX_BUFFER_LENGTH) ? arg3 : MAX_BUFFER_LENGTH;
            read_buffer_from_child(child_pid, arg2, length, buffer);
            printf(" (fd=%ld, buf=\"%.*s\", count=%ld)", arg1, (int)length, buffer, arg3);
            break;
        }
        case SYS_read:
            printf(" (fd=%ld, buf=0x%lx, count=%ld)", arg1, arg2, arg3);
            break;
        default:
            printf(" (%ld, %ld, %ld, %ld, %ld, %ld)", arg1, arg2, arg3, arg4, arg5, arg6);
            break;
    }
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
#elif __i386__
        long syscall_number = regs.orig_eax;
        long arg1 = regs.ebx;
        long arg2 = regs.ecx;
        long arg3 = regs.edx;
        long arg4 = regs.esi;
        long arg5 = regs.edi;
        long arg6 = regs.ebp;
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

#ifdef __x86_64__
        long retval = regs.rax;
#elif __i386__
        long retval = regs.eax;
#else
#error "Unsupported architecture"
#endif

        // const char* error_desc = get_error_description(retval);
        // if (verbose || pause) {
        //     printf(" = %ld (%s)\n", retval, error_desc);
        // }

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
        printf("No program specified. Only showing syscall counts\n");
        return 0;
    }

    pid_t child_pid = fork();
    if (child_pid < 0) {
        perror("fork");
        return 1;
    }
    if (child_pid == 0) {
        run_target(argv[optind], &argv[optind]);
    } else {
        run_tracer(child_pid, verbose, pause);
    }

    return 0;
}
