#include <stdio.h>

#define SQUARE(x) ((x) * (x))
#define STRINGIFY(x) #x
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define STATIC_ASSERT(cond, msg) _Static_assert((cond), msg)

// Static gives internal linkage; without static, the symbol is visible to other files.
static int internal_value = 42;

static int square_int(int x) {
    return SQUARE(x);
}

int main(void) {
    // The preprocessor does textual substitution before compilation.
    printf("SQUARE(7) = %d\n", SQUARE(7));
    printf("MAX(8, 13) = %d\n", MAX(8, 13));
    printf("STRINGIFY(hello) = %s\n", STRINGIFY(hello));
    printf("internal_value = %d\n", internal_value);
    printf("square_int(9) = %d\n", square_int(9));

    STATIC_ASSERT(sizeof(int) >= 4, "int must be at least 32 bits");
    return 0;
}
