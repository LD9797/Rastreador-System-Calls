#include <stdio.h>
#include <seccomp.h>
#include <errno.h>

const char* get_syscall_name(long syscall_number) {
    const char* name = seccomp_syscall_resolve_num_arch(SCMP_ARCH_X86_64, syscall_number);
    if (name == NULL) {
        return "Unknown syscall";
    }
    return name;
}

int main() {
    // List of known system call numbers (e.g., for x86_64 architecture)
    // You can add more system call numbers to test
    long test_syscalls[] = { 0, 1, 2, 3, 60 };  // Example numbers; replace with real ones
    size_t num_syscalls = sizeof(test_syscalls) / sizeof(test_syscalls[0]);

    for (size_t i = 0; i < num_syscalls; ++i) {
        long syscall_number = test_syscalls[i];
        const char* syscall_name = get_syscall_name(syscall_number);
        printf("Syscall number %ld: %s\n", syscall_number, syscall_name);
    }

    return 0;
}
