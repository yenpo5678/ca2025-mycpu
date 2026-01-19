/*
 * libc_backend.c
 * System Call Stubs for Picolibc / Newlib
 */

#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h> 

// ============================================================================
// 1. Standard Output (Enable printf via UART)
// ============================================================================

// UART Transmit Register Address (Must match config.h / hardware)
#define UART_TX_ADDR ((volatile int*)0x40000010)

// This function is called by printf to output a single character
static int _uart_putc(char c, FILE *file) {
    // Write character to UART MMIO
    *UART_TX_ADDR = c;
    return c;
}

// Setup Standard Streams (stdin, stdout, stderr)
static FILE __stdio = FDEV_SETUP_STREAM(_uart_putc, NULL, NULL, _FDEV_SETUP_WRITE);

FILE *const stdin  = &__stdio;
FILE *const stdout = &__stdio;
FILE *const stderr = &__stdio;

// ============================================================================
// 2. Heap Management (sbrk)
// Implements the backend for malloc().
// ============================================================================

extern char _end[]; // Symbol defined by Linker Script denoting end of .bss
static char *heap_ptr = NULL;

// Hard limit for Heap to prevent Stack collision (Set at 56MB)
#define HEAP_LIMIT ((char*)0x03800000) 

void *_sbrk(int incr) {
    char *prev_heap;

    // Initialize Heap Pointer on first call
    if (heap_ptr == NULL) {
        heap_ptr = _end;
        // [CRITICAL] Force 8-byte alignment
        // RISC-V double-word loads/stores (uint64_t) require alignment,
        // or they will trap (or be very slow). Picolibc expects this.
        if ((uintptr_t)heap_ptr % 8 != 0) {
            heap_ptr += 8 - ((uintptr_t)heap_ptr % 8);
        }
    }

    prev_heap = heap_ptr;
    
    // Debug: Print 'S' to indicate an allocation request
    *UART_TX_ADDR = 'S';

    // Check for Heap Overflow
    if (heap_ptr + incr > HEAP_LIMIT) {
        *UART_TX_ADDR = '!'; // Out of Memory Indicator
        errno = ENOMEM;
        return (void*)-1;
    }
    
    heap_ptr += incr;
    return (void*)prev_heap;
}

// ============================================================================
// 3. File System Stubs (All return error / no-op)
// Since we don't have a filesystem, these just satisfy the linker.
// ============================================================================

int open(const char *name, int flags, int mode) {
    errno = ENOENT;
    return -1;
}

int close(int file) {
    return -1;
}

int fstat(int file, struct stat *st) {
    st->st_mode = S_IFCHR;
    return 0;
}

int stat(const char *path, struct stat *st) {
    st->st_mode = S_IFCHR;
    return 0;
}

int unlink(const char *name) {
    errno = ENOENT;
    return -1;
}

// Process Control Stubs
int getpid(void) { return 1; }
int kill(int pid, int sig) { return -1; }

int isatty(int file) {
    return 1;
}

off_t lseek(int file, off_t ptr, int dir) {
    return 0;
}

ssize_t read(int file, void *ptr, size_t len) {
    return 0;
}

// Low-level write (Redirects to UART)
ssize_t write(int file, const void *ptr, size_t len) {
    const char *c = (const char*)ptr;
    for (size_t i = 0; i < len; i++) {
        *UART_TX_ADDR = c[i];
    }
    return len;
}

void _exit(int status) {
    // Halt CPU loop
    while(1);
}