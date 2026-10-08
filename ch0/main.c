/**
 * csapp_refresher.c
 * Target: x86-64 Linux, GCC, LP64, C11/GNU11
 * Scope: Core C -> Memory -> System Interfaces -> Performance
 */

// --- 11. Preprocessor & 1. Translation Pipeline ---
#define _GNU_SOURCE           // Request GNU extensions (e.g., POSIX, extra headers)
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>           // Exact-width integers (uint32_t, etc.)
#include <stdbool.h>          // bool, true, false
#include <string.h>           // memcpy, memset, strlen
#include <stdarg.h>           // va_list, va_start
#include <stddef.h>           // offsetof, ptrdiff_t, size_t
#include <unistd.h>           // fork, read, write, execve
#include <pthread.h>          // POSIX threads
#include <signal.h>           // sigaction, sig_atomic_t
#include <limits.h>           // INT_MAX, UINT_MAX

// Macros: Textual expansion. Protect args with ()
#define SWAP(a, b) do { typeof(a) t_ = (a); (a) = (b); (b) = t_; } while(0)
#define LIKELY(x)   __builtin_expect(!!(x), 1) // 18. GCC Extensions: Branch prediction hint

// --- 9. Storage, Scope, Linkage ---
int global_strong = 1;        // .data segment (initialized, strong symbol)
int global_bss;               // .bss segment (uninitialized, zeroes, strong if -fno-common)
static int internal_var = 42; // .data, internal linkage (file-scope only)
volatile sig_atomic_t flag;   // Safe for signal handlers, prevents register caching

// --- 8. Structs, Unions, Enums, Bit-fields ---
struct layout_demo {
    char c;                   // @0, size 1
                              // 3 bytes padding inserted here by compiler
    int i;                    // @4, size 4, requires 4-byte alignment
    double d;                 // @8, size 8, requires 8-byte alignment
};                            // Total size: 16, Alignment: 8

union type_pun {              // All members share offset 0
    float f;
    uint32_t u;
};

// --- 5. Functions & Variadic args ---
// Function pointer typedef: 'cmp_fn' is a pointer to a function returning int
typedef int (*cmp_fn)(const void*, const void*);

int sum_variadic(int count, ...) {
    va_list ap; 
    va_start(ap, count);
    int total = 0;
    for (int i = 0; i < count; i++) {
        // Warning: char/short promote to int, float to double in variadic!
        total += va_arg(ap, int); 
    }
    va_end(ap);
    return total;
}

// --- 2. Types, 3. Expressions & Two's Complement ---
void types_and_bits() {
    printf("\n--- Types, Bits, and Math ---\n");
    
    // Limits and Two's Complement
    int tmin = INT_MIN;                   // Asymmetric: -2147483648
    int tmax = INT_MAX;                   // 2147483647
    printf("-TMin == TMin: %d\n", -tmin == tmin); // TRUE! Overflow in negation

    // Casting: Size change precedes signedness change!
    short sx = -12345;
    unsigned uy = sx; // Sign-extends to 32-bit int, THEN reinterprets as unsigned
    printf("Sign-extended then unsigned: %u\n", uy);

    // Usual Arithmetic Conversions & Promotion
    // -1 promoted to unsigned int (UINT_MAX), so -1 < 0U is FALSE
    printf("-1 < 0U is %d\n", -1 < 0U); 

    // Bitwise idioms
    uint32_t x = 0b10101010; // GCC extension for binary literal
    x |= (1u << 4);          // Set bit 4
    x &= ~(1u << 2);         // Clear bit 2
    x ^= (1u << 0);          // Toggle bit 0
    
    // Shifts: Signed right shift (arithmetic) vs Unsigned (logical)
    int s_shift = -8 >> 1;     // -4 (fills with 1s, compiler-defined but standard on x86)
    unsigned u_shift = 8u >> 1; // 4 (fills with 0s)
    
    // IEEE 754 float exactness
    float f = 16777217.0f;     // > 2^24, inexact representation!
    printf("Float precision loss: %f\n", f); 
}

// --- 6. Pointers & 7. Arrays ---
void pointers_and_arrays() {
    printf("\n--- Pointers & Arrays ---\n");
    
    int a[5] = {10, 20, 30, 40, 50};
    int *p = a;                 // Array decays to pointer (&a[0])
    
    // Pointer arithmetic scales by sizeof(type)
    printf("p = %p, p + 1 = %p (+4 bytes)\n", (void*)p, (void*)(p + 1));
    printf("p[2] == *(p+2): %d\n", p[2]);

    // sizeof differences
    printf("sizeof(a) = %zu (entire array, 20 bytes)\n", sizeof(a));
    printf("sizeof(p) = %zu (pointer, 8 bytes)\n", sizeof(p));

    // Multidimensional Arrays (contiguous memory, row-major)
    int mat[2][3] = {{1, 2, 3}, {4, 5, 6}};
    // Manual address calculation: base + (row * cols + col) * sizeof(type)
    printf("mat[1][2] = %d\n", mat[1][2]); 

    // String literals reside in read-only memory (.rodata)
    char *ro_str = "Immutable"; 
    // ro_str[0] = 'i'; // UB: SIGSEGV!
    char rw_str[] = "Mutable";  // Copied to stack
    rw_str[0] = 'm';            // Safe
    
    // Strict aliasing & Type Punning via Union
    union type_pun tp = { .f = 1.0f };
    printf("1.0f in hex: 0x%08x\n", tp.u); // Safely inspect float bits
}

// --- 10. Dynamic Memory ---
void dynamic_memory() {
    printf("\n--- Dynamic Memory ---\n");
    
    size_t n = 5;
    // malloc: uninitialized memory. sizeof(*arr) perfectly tracks type changes.
    int *arr = malloc(n * sizeof(*arr)); 
    if (!arr) return; // Always check for NULL!
    
    // calloc: zero-initialized. Good for preventing "reading uninitialized memory" bugs.
    int *clean_arr = calloc(n, sizeof(*clean_arr));

    // realloc: expanding memory. NEVER do `p = realloc(p, size)` directly (leak on fail).
    void *tmp = realloc(arr, (n * 2) * sizeof(*arr));
    if (tmp) {
        arr = tmp; // Safe to assign back
    }

    free(arr);
    arr = NULL; // Set to NULL to prevent Dangling Pointer / Use-After-Free
    free(clean_arr);
    // free(arr); // Safe because free(NULL) is a no-op
}

// --- 14. Systems Interfaces (Concurrency, Unix I/O, Processes) ---
void *thread_routine(void *arg) {
    // Cast argument back from uintptr_t (safe integer-to-pointer cast)
    int thread_id = (int)(uintptr_t)arg;
    printf("Hello from thread %d\n", thread_id);
    return NULL;
}

void system_interfaces() {
    printf("\n--- System Interfaces ---\n");

    // 14.2 Processes (Fork)
    // NOTE: Guarded by if(0) to prevent actual fork in test environments
    if (0) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed"); // Error handling idiom
            exit(1);
        } else if (pid == 0) {
            // Child process: executes new program
            char *args[] = {"/bin/ls", "-l", NULL};
            execve(args[0], args, NULL);
            _exit(127); // Async-signal-safe exit if execve fails
        } else {
            // Parent process: reap child to prevent Zombie process
            int status;
            waitpid(pid, &status, 0); 
        }
    }

    // 14.7 Threads (Pthreads)
    pthread_t tid;
    // Argument passing race prevention: cast by value via uintptr_t
    int val = 42; 
    pthread_create(&tid, NULL, thread_routine, (void*)(uintptr_t)val);
    pthread_join(tid, NULL); // Wait for thread to finish

    // 14.5 Unix I/O
    // File descriptors: 0=stdin, 1=stdout, 2=stderr
    // Short counts handle: write() might write fewer bytes than requested.
    const char *msg = "Raw Unix I/O write\n";
    ssize_t bytes_written = write(STDOUT_FILENO, msg, strlen(msg));
    (void)bytes_written; // Suppress unused warning
}

// --- 17. C for Performance ---
// restrict keyword: Promises 'dest' and 'src' don't overlap in memory. 
// Allows compiler to cache loads/stores and vectorise (SSE/AVX).
void fast_copy(int *restrict dest, const int *restrict src, size_t n) {
    // Loop unrolling & spatial locality (stride-1 access)
    for (size_t i = 0; LIKELY(i < n); i++) {
        dest[i] = src[i];
    }
}

// --- Entry Point ---
int main(int argc, char *argv[]) {
    // Standard I/O output buffers (line-buffered for terminal)
    printf("CSAPP Refresher Executing...\n");
    printf("argc = %d, argv[0] = %s\n", argc, argv[0]);

    types_and_bits();
    pointers_and_arrays();
    dynamic_memory();
    system_interfaces();

    // 8. Structs & offsetof
    printf("\nStruct padding: offset of 'd' is %zu (size %zu)\n", 
           offsetof(struct layout_demo, d), sizeof(struct layout_demo));

    printf("\nDone.\n");
    return 0; // exit(0) under the hood
}
