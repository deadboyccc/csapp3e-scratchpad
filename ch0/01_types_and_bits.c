#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static void show_bytes(const void *ptr, size_t n) {
    const unsigned char *bytes = (const unsigned char *)ptr;
    for (size_t i = 0; i < n; ++i) {
        printf("%02x ", bytes[i]);
    }
    putchar('\n');
}

static int tadd_ok(int x, int y) {
    // Signed addition is undefined on overflow; check before the math.
    return !((x > 0 && y > INT_MAX - x) || (x < 0 && y < INT_MIN - x));
}

static unsigned set_bit(unsigned value, int bit_index) {
    return value | (1u << bit_index);
}

static unsigned clear_bit(unsigned value, int bit_index) {
    return value & ~(1u << bit_index);
}

static bool test_bit(unsigned value, int bit_index) {
    return (value & (1u << bit_index)) != 0u;
}

int main(void) {
    int x = -1;
    unsigned u = 0;

    printf("INT_MIN = %d\n", INT_MIN);
    printf("INT_MAX = %d\n", INT_MAX);
    printf("UINT_MAX = %u\n", UINT_MAX);
    printf("(unsigned)-1 < 0U -> %d\n", ((unsigned int)x) < u);
    printf("-1 < 0U (as written) -> %d\n", -1 < 0U);

    unsigned int value = 0x00000000u;
    value = set_bit(value, 3);
    value = set_bit(value, 10);
    value = clear_bit(value, 3);

    printf("bit 10 set? %s\n", test_bit(value, 10) ? "yes" : "no");
    printf("bit 3 set? %s\n", test_bit(value, 3) ? "yes" : "no");

    int a = INT_MAX;
    int b = 1;
    printf("tadd_ok(%d, %d) -> %d\n", a, b, tadd_ok(a, b));

    // Bit-pattern view: same bytes, different interpretation.
    int sample = 0x01234567;
    printf("sample in hex: 0x%08x\n", (unsigned)sample);
    show_bytes(&sample, sizeof(sample));

    // Sign extension changes the numeric value after reinterpretation.
    short s = -12345;
    unsigned int uy = (unsigned int)s;
    printf("short -12345 as unsigned int = %u\n", uy);

    return 0;
}
