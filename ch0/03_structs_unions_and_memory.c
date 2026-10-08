#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Record {
    int id;
    char tag;
    double value;
};

typedef union {
    uint32_t bits;
    float value;
} float_bits;

struct Packet {
    // Bit-fields pack small members tightly into one word.
    unsigned version : 3;
    unsigned type : 5;
    unsigned flags : 1;
    unsigned reserved : 7;
};

enum Color {
    RED = 1,
    GREEN = 2,
    BLUE = 4,
};

static void print_record_layout(void) {
    struct Record r = {42, 'A', 3.5};
    printf("sizeof(struct Record) = %zu\n", sizeof(r));
    printf("offset(id) = %zu\n", offsetof(struct Record, id));
    printf("offset(tag) = %zu\n", offsetof(struct Record, tag));
    printf("offset(value) = %zu\n", offsetof(struct Record, value));
    printf("record = {%d, %c, %.1f}\n", r.id, r.tag, r.value);
}

static void print_float_bits(float x) {
    float_bits u = {0};
    u.value = x;
    printf("%f -> raw bits = 0x%08x\n", x, u.bits);
}

int main(void) {
    print_record_layout();

    // Union members share the same storage, so memory is reused.
    float_bits u = {.value = 1.0f};
    printf("union as float = %.1f\n", u.value);
    printf("union as bits = 0x%08x\n", u.bits);
    print_float_bits(3.5f);

    struct Packet p = {.version = 7, .type = 3, .flags = 1, .reserved = 0};
    printf("packet size = %zu bytes\n", sizeof(p));
    printf("packet fields = version=%u type=%u flags=%u\n",
           p.version, p.type, p.flags);

    enum Color chosen = GREEN;
    printf("chosen color = %d\n", chosen);

    // Dynamic allocation is separate from stack object lifetime.
    int *nums = malloc(5 * sizeof(*nums));
    if (nums == NULL) {
        return 1;
    }
    for (int i = 0; i < 5; ++i) {
        nums[i] = i * 10;
    }
    printf("allocated values = %d %d %d %d %d\n",
           nums[0], nums[1], nums[2], nums[3], nums[4]);
    free(nums);

    return 0;
}
