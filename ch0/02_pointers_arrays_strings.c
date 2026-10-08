#include <stdio.h>
#include <string.h>

static int sum_array(const int *arr, size_t len) {
    int total = 0;
    // Pointer arithmetic and array decay are the same idea in C.
    for (size_t i = 0; i < len; ++i) {
        total += arr[i];
    }
    return total;
}

static void reverse_in_place(char *s) {
    size_t len = strlen(s);
    for (size_t i = 0; i < len / 2; ++i) {
        char tmp = s[i];
        s[i] = s[len - 1 - i];
        s[len - 1 - i] = tmp;
    }
}

int main(void) {
    int values[5] = {10, 20, 30, 40, 50};
    int *p = values;

    printf("values[2] = %d\n", values[2]);
    printf("*(p + 2) = %d\n", *(p + 2));
    printf("sum = %d\n", sum_array(values, 5));

    // Arrays decay to pointers when passed to functions.
    printf("address of values = %p\n", (void *)values);
    printf("address of p = %p\n", (void *)p);

    char msg[] = "hello";
    reverse_in_place(msg);
    printf("reversed string = %s\n", msg);

    const char *text = "C is a systems language";
    printf("strlen(%s) = %zu\n", text, strlen(text));

    // Strings are NUL-terminated arrays of chars.
    char copy[32] = {0};
    snprintf(copy, sizeof(copy), "%s", text);
    printf("copy = %s\n", copy);

    return 0;
}
