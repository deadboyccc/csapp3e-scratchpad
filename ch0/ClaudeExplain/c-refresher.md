# C Refresher for CSAPP

**Target:** CSAPP 3e, x86-64 Linux, GCC, LP64 data model, C11 (GNU extensions noted).
**Scope:** every C concept the book relies on, ordered from language core → memory → systems interfaces → C-to-machine mapping.

---

## Table of Contents

1. Translation Pipeline
2. Types and Data Representation
3. Expressions and Operators
4. Control Flow
5. Functions
6. Pointers
7. Arrays and Strings
8. Structs, Unions, Enums, Typedef, Bit-fields
9. Storage, Scope, Linkage, Program Layout
10. Dynamic Memory
11. Preprocessor
12. Multi-file Programs and Linking (C view)
13. Standard I/O
14. Systems Interfaces (Processes, Signals, Unix I/O, Sockets, Threads)
15. Undefined / Unspecified / Implementation-defined Behavior
16. C Construct → Machine Realization
17. C for Performance (Ch. 5–6)
18. GCC Extensions and C11 Features Used
19. Memory Bug Catalog
20. Tool Cheat Sheet
21. Verification Exercises

---

## 1. Translation Pipeline

| Stage | Tool | Input → Output | Flag |
|---|---|---|---|
| Preprocess | `cpp` | `x.c` → `x.i` | `-E` |
| Compile | `cc1` | `x.i` → `x.s` | `-S` |
| Assemble | `as` | `x.s` → `x.o` (relocatable ELF) | `-c` |
| Link | `ld` | `*.o` + libs → executable ELF | (default) |

- **Translation unit** = one `.c` file after preprocessing. Units are compiled independently; the compiler does **not** check types across units. Headers are the only cross-unit consistency mechanism.
- Linker resolves **symbols** (names), not types.

Common flags:

| Flag | Effect |
|---|---|
| `-O0 -Og -O1 -O2 -O3` | optimization level (`-Og` = debug-friendly) |
| `-Wall -Wextra -Wconversion -Wsign-compare` | warnings |
| `-std=c11` / `-std=gnu11` | language standard |
| `-g` | debug info |
| `-fno-stack-protector` | disable canary (bomb/attack labs) |
| `-no-pie` / `-static` | fixed addresses / static linking |
| `-fcommon` / `-fno-common` | tentative-definition handling (GCC ≥ 10 default: `-fno-common`) |
| `-fno-omit-frame-pointer` | keep `%rbp` frames |
| `-fsanitize=address,undefined` | runtime memory/UB checking |
| `-fwrapv` | define signed overflow as wraparound |
| `-m32` | 32-bit target (ILP32) |

---

## 2. Types and Data Representation

### 2.1 Integer Types (x86-64 Linux, LP64)

| Type | Bytes | Signed range | Unsigned max | `printf` |
|---|---|---|---|---|
| `char` | 1 | −128..127 (signedness is implementation-defined; signed on x86) | 255 | `%c`, `%hhd`, `%hhu` |
| `short` | 2 | −2¹⁵..2¹⁵−1 | 2¹⁶−1 | `%hd`, `%hu` |
| `int` | 4 | −2³¹..2³¹−1 | 2³²−1 | `%d`, `%u` |
| `long` | 8 | −2⁶³..2⁶³−1 | 2⁶⁴−1 | `%ld`, `%lu` |
| `long long` | 8 | −2⁶³..2⁶³−1 | 2⁶⁴−1 | `%lld`, `%llu` |
| pointer | 8 | — | — | `%p` |
| `size_t` | 8 (unsigned) | — | 2⁶⁴−1 | `%zu` |
| `ssize_t` | 8 (signed) | — | — | `%zd` |
| `ptrdiff_t` | 8 (signed) | — | — | `%td` |

- Hex output: `%x` (`unsigned int`), `%lx` (`unsigned long`), `%#x` (with `0x`), `%08x` (zero-padded width 8), `%hhx` (byte).
- ISO guarantees only: `sizeof(char) == 1`; `char ≥ 8 bits`; `short ≥ 16`, `int ≥ 16`, `long ≥ 32`, `long long ≥ 64` bits; `short ≤ int ≤ long ≤ long long`.
- Data models: **ILP32** (`int`, `long`, pointer = 4 B), **LP64** (`long`, pointer = 8 B; Linux/macOS 64-bit), **LLP64** (`long` = 4 B; Windows 64-bit).
- `<stdint.h>`: exact-width `int8_t … int64_t`, `uint8_t … uint64_t`; `intptr_t`, `uintptr_t` (integer wide enough for a pointer); limits `INT32_MAX`, `UINT64_MAX`, …
- `<limits.h>`: `CHAR_BIT`, `INT_MAX`, `INT_MIN`, `UINT_MAX`, `LONG_MAX`, `LONG_MIN`, `ULONG_MAX`, …
- Write TMin as `(-INT_MAX - 1)`; never as `-2147483648` (a decimal literal `2147483648` does not fit `int`, so it has type `long` on LP64 and the expression is not `int`).

**Literals**

| Form | Type |
|---|---|
| `10` | first of `int`, `long`, `long long` that fits |
| `0x1F`, `017` (octal) | first of `int`, `unsigned`, `long`, `unsigned long`, … that fits |
| suffix `U`, `L`, `UL`, `LL`, `ULL` | forces unsigned / long / … |
| `'a'` | **`int`** in C (value 97) |
| `1.0` | `double`; `1.0f` → `float`; `1.0L` → `long double` |
| `"abc"` | `char[4]` (static storage, immutable) |

- `0b1010` binary literal: GCC extension (C23 standard).
- `_Bool` / `bool` (`<stdbool.h>`): any nonzero value converts to 1.
- `enum` constants have type `int`.

### 2.2 Two's Complement (w-bit words)

| Quantity | Value |
|---|---|
| `UMax` | 2ʷ − 1 |
| `TMax` | 2ʷ⁻¹ − 1 |
| `TMin` | −2ʷ⁻¹ |
| `|TMin|` | `TMax + 1` (asymmetric range) |

- **B2U**(x) = Σ xᵢ2ⁱ; **B2T**(x) = −x_{w−1}2ʷ⁻¹ + Σ_{i<w−1} xᵢ2ⁱ.
- Negation: `-x == ~x + 1`; `~x == -x - 1`. `-TMin == TMin`; `-0 == 0`.
- **Signed ↔ unsigned cast** keeps the bit pattern; value changes:
  - T2U(x) = x + 2ʷ if x < 0, else x.
  - U2T(u) = u − 2ʷ if u > TMax, else u.
- **Size change precedes signedness change**: `short sx = -12345; unsigned uy = sx;` → sign-extend to 32 bits, then reinterpret → `4294954951`.
- **Extension**: source unsigned → zero-extend; source signed → sign-extend (replicate MSB). Value-preserving.
- **Truncation** to k bits: keep low k bits (mod 2ᵏ); for signed, reinterpret result as two's complement.
- **Unsigned addition**: result mod 2ʷ. Overflow iff `s = x + y; s < x`.
- **Signed addition overflow**: positive overflow (x>0, y>0, s≤0), negative overflow (x<0, y<0, s≥0). In C this is UB, so test *before* adding:

```c
int tadd_ok(int x, int y) {
    return !((x > 0 && y > INT_MAX - x) || (x < 0 && y < INT_MIN - x));
}
/* or */ bool ovf = __builtin_add_overflow(x, y, &s);   /* GCC/Clang */
```

- **Multiplication**: low w bits of the product are identical for signed and unsigned operands. `x << k == x * 2ᵏ` (mod 2ʷ).
- **Division by 2ᵏ**:
  - unsigned: `x >> k` (logical).
  - signed: arithmetic `>>` rounds toward −∞; C division rounds toward 0, so use bias: `(x + (1<<k) - 1) >> k` for x < 0 (compiler emits this).
- **Unsigned pitfalls**:
  - `for (size_t i = n-1; i >= 0; i--)` never terminates.
  - `n-1` with `unsigned n = 0` wraps to `UINT_MAX`.
  - `strlen(a) - strlen(b) > 0` is true whenever lengths differ.
  - `sizeof(x) - 5 < 0` is always false.

### 2.3 Bit-level Operations

| Op | Meaning |
|---|---|
| `& \| ^ ~` | bitwise AND, OR, XOR, NOT (operate per bit) |
| `&& \|\| !` | logical; result is `int` 0 or 1; `&&`/`\|\|` short-circuit |
| `<<` | left shift, zero fill |
| `>>` | unsigned: logical (zero fill); signed: arithmetic in GCC (implementation-defined) |

Shift rules:
- Count `< 0` or `≥` promoted width of left operand → **UB** (x86 hardware masks the count to 5/6 bits, so `x << 32` behaves as `x << 0` on 32-bit operands).
- Result type = promoted left operand; right operand independently promoted.
- Left shift of a negative signed value, or one whose result is unrepresentable (`1 << 31` with `int`), is UB. Use `1u << 31`.
- `<<` and `>>` have **lower precedence than `+ -`**: `1 << 2 + 3` is `1 << 5`.

Idioms:

```c
x & 0xFF                 /* low byte */
x | (1u << k)            /* set bit k */
x & ~(1u << k)           /* clear bit k */
x ^ (1u << k)            /* toggle bit k */
(x >> k) & 1             /* test bit k */
x & (x - 1)              /* clear lowest set bit */
x & -x                   /* isolate lowest set bit */
!!x                      /* normalize to 0/1 */
!(x ^ y)                 /* x == y */
```
- XOR swap (`a^=b; b^=a; a^=b;`) zeroes the value when `a` and `b` alias the same object.

### 2.4 Byte Order and Addressing

- x86 is **little-endian**: least significant byte at lowest address. `int x = 0x01234567;` → bytes `67 45 23 01`.
- Network byte order is big-endian: `htons`, `htonl`, `ntohs`, `ntohl` (`<arpa/inet.h>`).
- A multi-byte object's address = address of its lowest byte.
- Inspect representation via `unsigned char *`:

```c
void show_bytes(const void *p, size_t n) {
    const unsigned char *b = p;
    for (size_t i = 0; i < n; i++) printf(" %.2x", b[i]);
    putchar('\n');
}
```

### 2.5 Integer Promotion and Conversion

**Integer promotions**: any operand of rank lower than `int` (`char`, `short`, `_Bool`, bit-fields; signed or unsigned) becomes `int` (since `int` can represent all their values). Applied to operands of arithmetic, bitwise, unary `+ - ~`, each operand of shifts, and variadic arguments.

**Usual arithmetic conversions** (binary arithmetic, comparison, `?:` 2nd/3rd operands), after promotions:
1. If either is `long double` → `long double`; else `double`; else `float`.
2. Both integers: same type → done.
3. Same signedness → the higher rank.
4. Unsigned rank ≥ signed rank → **unsigned** type.
5. Else if signed type represents all values of the unsigned type → signed type.
6. Else → unsigned version of the signed type.

Consequences:

| Expression | Result | Reason |
|---|---|---|
| `-1 < 0U` | 0 | `-1` → `UINT_MAX` |
| `-1 < 0` | 1 | signed compare |
| `sizeof(int) > -1` | 0 | `-1` → `size_t` |
| `2147483647U > -2147483647-1` | 0 | `TMin` → `2³¹` unsigned |
| `(unsigned char)200 + (unsigned char)200` | 400 | promoted to `int` |
| `uint8_t a = 0xFF; ~a` | `-256` | `~` applied to promoted `int` |
| `1 ? -1 : 0U` | `UINT_MAX` | conditional operands converted to common unsigned type |

**Conversion rules**

| From → To | Behavior |
|---|---|
| any integer → unsigned | value mod 2ʷ (defined) |
| out-of-range → signed | implementation-defined (GCC: mod 2ʷ) |
| float/double → integer | truncate toward 0; out-of-range is UB (x86 `cvttss2si` yields `0x80000000`) |
| integer → float/double | exact if representable, else rounded (nearest-even) |
| float → double | exact |
| double → float | rounded; overflow → ±∞ |

### 2.6 Floating Point (IEEE 754)

Layout `s | exp | frac`:

| Type | Bits (s/exp/frac) | Bias |
|---|---|---|
| `float` | 1 / 8 / 23 | 127 |
| `double` | 1 / 11 / 52 | 1023 |
| `long double` (x87) | 80-bit extended, 16 B storage | — |

| exp | frac | Meaning |
|---|---|---|
| ≠ 0, ≠ all-1 | any | **normalized**: V = (−1)ˢ · 1.frac · 2^(exp−bias) |
| 0 | any | **denormalized**: V = (−1)ˢ · 0.frac · 2^(1−bias) (includes ±0) |
| all 1 | 0 | ±∞ |
| all 1 | ≠ 0 | NaN |

- Rounding modes: nearest-even (default), toward 0, toward +∞, toward −∞.
- Not associative: `(3.14 + 1e10) - 1e10 == 0.0` but `3.14 + (1e10 - 1e10) == 3.14`. Not distributive.
- Every `int` is exact in `double`; `int` > 2²⁴ may be inexact in `float` (`(float)16777217 == 16777216.0f`).
- NaN: every comparison false except `!=`; `x != x` is true iff NaN. `0.0 == -0.0` true.
- `0.1`, `0.2`, `0.3` not exactly representable: `0.1 + 0.2 == 0.3` is false.
- x86-64 uses SSE for `float`/`double` (`addss`, `addsd`); `long double` uses x87.
- `-ffast-math` permits reassociation and breaks IEEE semantics.
- `<float.h>`: `FLT_MAX`, `DBL_MAX`, `DBL_EPSILON`, …

---

## 3. Expressions and Operators

| Prec. | Operators | Assoc. |
|---|---|---|
| 1 | `()` `[]` `.` `->` postfix `++ --` compound literal | L→R |
| 2 | prefix `++ --`, unary `+ -`, `! ~`, `*` (deref), `&`, `sizeof`, `(type)` cast | R→L |
| 3 | `* / %` | L→R |
| 4 | `+ -` | L→R |
| 5 | `<< >>` | L→R |
| 6 | `< <= > >=` | L→R |
| 7 | `== !=` | L→R |
| 8 | `&` | L→R |
| 9 | `^` | L→R |
| 10 | `\|` | L→R |
| 11 | `&&` | L→R |
| 12 | `\|\|` | L→R |
| 13 | `?:` | R→L |
| 14 | `= += -= *= /= %= <<= >>= &= ^= \|=` | R→L |
| 15 | `,` | L→R |

Precedence traps:
- `x & 1 == 0` ≡ `x & (1 == 0)` → write `(x & 1) == 0`.
- `*p++` ≡ `*(p++)`; `(*p)++` increments pointee; `*++p`; `++*p`.
- `a = b == c` ≡ `a = (b == c)`.
- `1 << 2 + 3` ≡ `1 << (2 + 3)`.

Evaluation:
- **Sequence points** exist at `&&`, `||`, `?:`, `,`, end of full expression, and before a function call (after argument evaluation).
- Order of evaluation of operands and of function arguments is **unspecified**.
- Modifying an object twice (or modifying and reading it for another purpose) between sequence points is **UB**: `a[i] = i++;`, `f(i++, i++)`, `i = i++ + 1`.
- `sizeof expr` does not evaluate `expr` (except VLA types); result type `size_t`.
- Integer `/` truncates toward 0; `%` takes the sign of the dividend; `(a/b)*b + a%b == a`. `x/0`, `x%0`, and `INT_MIN / -1` are UB (x86 `idiv` raises SIGFPE).
- Assignment is an expression: `while ((c = getchar()) != EOF)`.
- `lvalue`: designates an object (can appear left of `=`, operand of `&`).

---

## 4. Control Flow

```c
if (c) s1; else s2;
switch (e) { case 1: ...; break; case 2: /* fallthrough */ case 3: ...; break; default: ...; }
while (c) s;   do s; while (c);   for (init; cond; step) s;   /* init may declare (C99) */
break; continue; goto label; return e;
```

- `switch` controlling expression is integer; case labels must be integer constant expressions, unique. Missing `break` falls through. GCC extension: `case 1 ... 5:`.
- `goto` is function-scoped; idiom: single cleanup/exit path.
- Any scalar (int, pointer, float) is a valid condition: nonzero/non-null → true.
- Machine mapping (see §16): `if`/loops → `cmp`/`test` + `jcc`; dense `switch` → jump table (`jmp *table(,%rax,8)`); simple `?:` / `if` assignments may become `cmov`.

---

## 5. Functions

```c
return_type name(type1 p1, type2 p2);       /* declaration / prototype */
return_type name(type1 p1, type2 p2) { … }  /* definition */
```

- `int f()` in C ≤ C17 means *unspecified* parameters; write `int f(void)` for none. Implicit declarations are errors in modern GCC.
- **Pass by value** always. To modify the caller's object, pass its address.
- Array parameters decay to pointers (`int a[]` ≡ `int *a`; `sizeof a` inside = 8).
- Structs are copied when passed/returned (size > 16 B returned via hidden pointer; see §16).
- `static` function → internal linkage. `static inline` in headers is the safe form of inlining.
- Recursion uses stack frames; tail calls optimized at `-O2` (not guaranteed). Locals die at return: returning `&local` is UB.
- `main`:

```c
int main(int argc, char *argv[], char *envp[]);   /* argv[argc] == NULL; envp also via `extern char **environ` */
```
Return value → `exit(value)`; status visible to parent via `wait`.
- Non-returning: `exit`, `_exit`, `abort`, `longjmp`, `execve` (on success).

**Function pointers**

```c
int (*fp)(int, int) = add;     /* name decays to pointer */
fp(1, 2);  (*fp)(1, 2);        /* equivalent calls */
typedef int (*binop)(int, int);
void qsort(void *base, size_t n, size_t size, int (*cmp)(const void *, const void *));
```
- Arrays of function pointers implement dispatch/jump tables.
- Casting between function and object pointers is not ISO-defined (POSIX `dlsym` requires it).

**Variadic functions** (`<stdarg.h>`):

```c
int sum(int n, ...) {
    va_list ap; va_start(ap, n);
    int s = 0; for (int i = 0; i < n; i++) s += va_arg(ap, int);
    va_end(ap); return s;
}
```
- Default argument promotions for variadic args: `char/short → int`, `float → double`. `va_arg(ap, float)` is wrong.
- Mismatch between format and arguments in `printf` is UB.

---

## 6. Pointers

### 6.1 Basics

```c
int x = 5;  int *p = &x;  *p = 6;     /* & and * are inverses */
int* a, b;                            /* b is int; '*' binds to the declarator */
```
- `NULL` is a null pointer constant (`((void*)0)`); dereferencing is UB (typically SIGSEGV). Uninitialized pointers are wild.
- Pointer size = 8 B on x86-64 (48-bit virtual addresses used).
- Pointer type determines the **scale** of arithmetic and the **width/interpretation** of dereference.

### 6.2 Arithmetic

| Expression | Meaning |
|---|---|
| `p + n` | address + `n * sizeof(*p)` |
| `p - q` | element count between them (`ptrdiff_t`); same array only |
| `p[i]` | `*(p + i)` (also `i[p]`) |
| `p < q` | valid only within the same array (or one past its end) |

- Arithmetic is valid only inside an array object or one element past its end.
- `void *` arithmetic is a GCC extension (unit = 1 byte). For portable byte-level arithmetic cast to `char *` / `unsigned char *`.
- `char *` (and `unsigned char *`) access may alias any object.

### 6.3 Arrays vs Pointers

```c
int a[5];
sizeof a          /* 20 */
a                 /* decays to &a[0], type int*   (rvalue) */
&a                /* type int (*)[5]; &a + 1 advances 20 bytes */
```
- Decay does **not** occur for: `sizeof`, unary `&`, string-literal initializer of a `char` array.
- Arrays are not assignable or comparable as values; structs are assignable.
- `a + i` is the address `a + i*sizeof(a[0])`.

### 6.4 Multi-dimensional Arrays

```c
int A[R][C];      /* contiguous, row-major */
&A[i][j] == (char *)A + (i*C + j) * sizeof(int)
```
- Type of `A[i]` is `int[C]`; `A` decays to `int (*)[C]`.
- `int *B[R]` is an array of pointers: rows may be non-contiguous; access requires two loads (load row pointer, load element). Different object, same syntax `B[i][j]`.
- Passing: `void f(int A[][C])`, `void f(int (*A)[C])`, or VLA parameter `void f(int n, int m, int A[n][m])`.
- Accessing column-by-column strides by `C * sizeof(int)` bytes (see §17).

### 6.5 Reading Declarations

Start at the identifier; go right (`[]`, `()`), then left (`*`); parentheses override.

| Declaration | Reading |
|---|---|
| `int *p` | p: pointer to int |
| `int *a[3]` | a: array[3] of pointer to int |
| `int (*a)[3]` | a: pointer to array[3] of int |
| `int *f(int)` | f: function(int) returning pointer to int |
| `int (*f)(int)` | f: pointer to function(int) returning int |
| `int (*(*f())[13])()` | f: function returning pointer to array[13] of pointer to function returning int |
| `void (*signal(int, void (*)(int)))(int)` | signal: function(int, ptr to function(int)→void) returning ptr to function(int)→void |
| `char **argv` | pointer to pointer to char |
| `const char *p` | pointer to const char |
| `char *const p` | const pointer to char |
| `const char *const p` | const pointer to const char |

### 6.6 Qualifiers

- `const`: modification through that lvalue is a constraint violation; modifying an object *defined* `const` is UB. `const T *p` (ptr to const T) ≡ `T const *p`; `T *const p` (const ptr).
- `volatile`: every access is performed exactly as written (no caching in registers, no elision/reordering relative to other volatile accesses). **Not** atomic, **not** a memory barrier. Uses: memory-mapped I/O, `volatile sig_atomic_t` flags shared with signal handlers, locals modified between `setjmp`/`longjmp`.
- `restrict`: promise that, during the pointer's lifetime, the pointed-to object is accessed only through that pointer (or pointers derived from it). Violation is UB. Enables optimization (`memcpy` parameters are `restrict`; `memmove` are not).

### 6.7 Casts, `void *`, Alignment, Aliasing

- `void *` converts implicitly to/from any object pointer type in C.
- Pointer ↔ integer: use `uintptr_t`.
- **Alignment**: an object of type T must reside at an address that is a multiple of `_Alignof(T)`. Misaligned access through a typed pointer is UB (x86 tolerates most scalar loads; SSE aligned moves fault).
- **Strict aliasing**: accessing an object through an lvalue of an incompatible type is UB. Allowed: same type (ignoring signedness/qualifiers), `char` types, aggregates/unions containing the type.
- Type-pun via `memcpy` or `union`, not pointer casts:

```c
uint32_t bits; memcpy(&bits, &f, sizeof bits);            /* float → bits */
union { float f; uint32_t u; } v = { .f = 1.0f };         /* v.u == 0x3f800000 */
```

### 6.8 Dangling / Wild Pointers

- After `free(p)`; after the pointed-to automatic object's scope ends; pointer returned from `realloc` move; pointer into a reallocated buffer. Set `p = NULL` after `free` to make misuse fail fast.

---

## 7. Arrays and Strings

**Arrays**
```c
int a[] = {1, 2, 3};            /* size inferred = 3 */
int b[5] = {1, 2};              /* rest zero */
int c[8] = { [2] = 7, [5] = 9 };/* designated initializers */
```
- No bounds checking; out-of-bounds access is UB (the mechanism behind stack buffer overflow).
- Element count: `sizeof a / sizeof a[0]` (only valid where `a` is an array, not a decayed pointer).
- **VLA** (`int a[n];`): automatic storage sized at runtime; `sizeof` evaluated at runtime; allocated on stack by adjusting `%rsp`.

**Strings**
- A string is a `char` array terminated by `'\0'`. `strlen` excludes it; `sizeof` of literal includes it.

```c
char s1[] = "abc";     /* mutable copy, 4 bytes */
char *s2  = "abc";     /* points into .rodata; writing → SIGSEGV/UB */
```

`<string.h>`:

| Function | Notes |
|---|---|
| `size_t strlen(s)` | O(n); returns unsigned |
| `strcpy(d, s)`, `strcat(d, s)` | unbounded → overflow risk |
| `strncpy(d, s, n)` | pads with `\0` up to `n`; **does not terminate** if `strlen(s) ≥ n` |
| `strcmp`, `strncmp` | compare as `unsigned char`; return <0, 0, >0 |
| `strchr`, `strrchr`, `strstr` | search; return pointer or `NULL` |
| `strtok` | keeps hidden state; not thread-safe (`strtok_r`) |
| `memcpy(d, s, n)` | regions must not overlap |
| `memmove(d, s, n)` | overlap-safe |
| `memset(p, byte, n)` | sets bytes, not ints |
| `memcmp(a, b, n)` | byte comparison (unreliable on structs: padding) |

- `snprintf(buf, size, fmt, …)` is bounded; `sprintf`, `gets` (removed in C11), `scanf("%s")` are unbounded.
- Parsing numbers: `strtol(s, &end, base)` (check `end` and `errno`); `atoi` gives no error detection.
- Use `unsigned char` for raw bytes; plain `char` may be signed.
- Stack buffer overflow: an unbounded write into a stack array overwrites saved registers/return address (mitigations: stack canary, NX, ASLR; bypass: ROP).

---

## 8. Structs, Unions, Enums, Typedef, Bit-fields

### 8.1 Structs

```c
struct point { int x, y; };
struct point p = { .x = 1, .y = 2 };       /* designated init */
struct point *q = &p;   q->x ≡ (*q).x
```
- Members are laid out in declaration order at increasing addresses; compiler may not reorder. Address of struct = address of first member.
- Member access = base + **compile-time constant offset**.
- Assignment and by-value pass/return copy the whole object; `==` is not defined for structs.

**Alignment (x86-64 Linux)**
- Scalar of size K (K = 1, 2, 4, 8) is aligned to K; `long double` and `__int128` to 16; pointers to 8.
- Struct alignment = max member alignment; struct size is a multiple of its alignment (tail padding).

```c
struct A { char c; int i; char d; };   /* c@0, pad3, i@4, d@8, pad3  → size 12, align 4 */
struct B { int i; char c; char d; };   /* i@0, c@4, d@5, pad2        → size 8 */
struct C { char c; double d; char e; };/* c@0, pad7, d@8, e@16, pad7 → size 24, align 8 */
```
- Ordering members by descending alignment minimizes padding.
- `offsetof(type, member)` (`<stddef.h>`); `_Alignof(T)` / `alignof` (`<stdalign.h>`); `_Alignas(n)`.
- `__attribute__((packed))` removes padding at the cost of misaligned member accesses.
- **Flexible array member** (last member, C99): `struct buf { size_t n; char data[]; };` allocate `malloc(sizeof(struct buf) + n)`.
- **Self-referential** types use pointers: `struct node { int v; struct node *next; };`. Forward-declared `struct foo;` is an incomplete type; only pointers to it can be formed (opaque handle pattern).
- Compound literal: `(struct point){ 1, 2 }`.
- Anonymous struct/union members (C11).

### 8.2 Unions

```c
union u { char c[8]; int i; double d; };    /* all members at offset 0; size = max member, padded */
```
- Reading a member other than the last one written reinterprets stored bytes (defined in C11 for the overlapping bytes).
- Uses: type punning, variant records, inspecting endianness:

```c
union { uint32_t i; unsigned char c[4]; } t = { .i = 1 };   /* t.c[0] == 1 on little-endian */
```

### 8.3 Enums and Typedef

```c
enum color { RED, GREEN = 5, BLUE };         /* 0, 5, 6; type int-compatible, no type safety */
typedef struct node node_t;
typedef unsigned char *byte_pointer;
typedef int (*cmp_fn)(const void *, const void *);
```
- `typedef` creates an alias, not a new type.

### 8.4 Bit-fields

```c
struct flags { unsigned a : 3; unsigned b : 5; };
```
- Layout, packing, and ordering are implementation-defined; fields are not addressable. Use masks and shifts for portable layouts (headers, protocols, hardware registers).

---

## 9. Storage, Scope, Linkage, Program Layout

### 9.1 Storage Duration

| Duration | Objects | Lifetime |
|---|---|---|
| automatic | non-`static` locals, parameters | enclosing block; uninitialized by default |
| static | globals, `static` locals | whole program; zero-initialized by default |
| allocated | `malloc` family | until `free` |
| thread | `_Thread_local` | thread lifetime |

- Static initializers must be constant expressions.
- `static` local: one instance, initialized once, retains value across calls.
- `register`: obsolete hint; address cannot be taken. `auto`: default for locals.

### 9.2 Scope and Linkage

| Linkage | Applies to |
|---|---|
| external | file-scope functions/variables (default); visible to the linker |
| internal | file-scope `static` |
| none | locals, parameters, typedefs |

- `extern int x;` **declares** (no storage). `int x = 1;` / `int x;` at file scope **defines**.
- Headers hold declarations (prototypes, `extern` variables, types, macros, `static inline`); `.c` files hold definitions. Use include guards.
- Scopes: block, file, function (labels), function prototype. Inner declarations shadow outer ones.

### 9.3 Where Objects Live (x86-64 Linux process image)

| Region | Content |
|---|---|
| `.text` | machine code |
| `.rodata` | string literals, `const` globals, jump tables |
| `.data` | initialized non-zero globals/statics |
| `.bss` | uninitialized / zero-initialized globals/statics (no file space) |
| heap | `malloc` blocks; grows toward higher addresses (`brk`) |
| mmap region | shared libraries, large `malloc` blocks, `mmap` mappings |
| stack | frames; grows toward **lower** addresses |
| kernel | upper half of address space |

Low → high: `.text`, `.rodata`, `.data`, `.bss`, heap ↑ … mmap region … stack ↓, kernel.

- Local-static symbols appear in the symbol table with mangled names (`x.1234`); two functions may each have a static `x`.
- C has no name mangling: the symbol for `foo` is `foo`.

---

## 10. Dynamic Memory (`<stdlib.h>`)

| Call | Semantics |
|---|---|
| `void *malloc(size_t n)` | uninitialized block; `NULL` on failure |
| `void *calloc(size_t c, size_t n)` | zeroed; checks `c*n` overflow |
| `void *realloc(void *p, size_t n)` | may move; on failure returns `NULL` and **keeps** old block; `realloc(NULL,n) ≡ malloc(n)` |
| `void free(void *p)` | `free(NULL)` is a no-op; only pointers from the allocator, once |

Idioms:
```c
T *p = malloc(n * sizeof *p);          /* sizeof *p tracks type changes; no cast in C */
if (!p) { /* handle */ }
void *tmp = realloc(p, new);  if (!tmp) { /* p still valid */ } else p = tmp;
```
- Check `n * sizeof *p` for overflow (or use `calloc`).
- glibc returns 16-byte-aligned blocks on x86-64.
- Underlying system calls: `brk/sbrk` (grow heap), `mmap/munmap` (large or standalone blocks).
  - `void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off)` → `MAP_FAILED` (`(void *)-1`) on error.
- `alloca` and VLAs allocate on the stack (released at function return).

**Allocator-implementation C idioms (malloc lab, CSAPP Fig. 9.43)**

```c
#define WSIZE 4
#define DSIZE 8
#define PACK(size, alloc)  ((size) | (alloc))
#define GET(p)             (*(unsigned int *)(p))
#define PUT(p, val)        (*(unsigned int *)(p) = (val))
#define GET_SIZE(p)        (GET(p) & ~0x7)
#define GET_ALLOC(p)       (GET(p) & 0x1)
#define HDRP(bp)           ((char *)(bp) - WSIZE)
#define FTRP(bp)           ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define NEXT_BLKP(bp)      ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)      ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))
```
- Block size is a multiple of 8 → low 3 bits of the header are free for flags.
- Cast to `char *` for byte arithmetic; cast to `unsigned int *` for word access; fully parenthesize macro arguments.

---

## 11. Preprocessor

| Directive | Use |
|---|---|
| `#include <f>` / `"f"` | system path / local-first path |
| `#define N 10`, `#define F(x) …`, `#undef N` | object-like / function-like macros |
| `#if`, `#ifdef`, `#ifndef`, `#elif`, `#else`, `#endif`, `defined(X)` | conditional compilation |
| `#error`, `#warning`, `#pragma` | diagnostics / compiler hints |
| `#`, `##` | stringify / token-paste (inside macros) |

- Expansion is **textual**; macros ignore scope and types.
- Parenthesize arguments and the whole body; arguments with side effects are evaluated once per use:

```c
#define SQ(x)  ((x) * (x))
SQ(i++)                          /* UB: i++ evaluated twice */
#define SWAP(a,b) do { typeof(a) t_ = (a); (a) = (b); (b) = t_; } while (0)   /* safe in if/else */
```
- Include guard:

```c
#ifndef FOO_H
#define FOO_H
/* declarations */
#endif
```
- Predefined: `__FILE__`, `__LINE__`, `__func__` (C99), `__DATE__`, `__STDC_VERSION__`.
- Variadic macros: `#define LOG(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)`.
- `-DNAME=val` defines at the command line; `assert()` disabled by `-DNDEBUG`.
- Prefer `static inline` functions and `enum` constants over macros when possible.

---

## 12. Multi-file Programs and Linking (C View)

**Symbols**
- Global symbols: non-`static` functions and variables. Local symbols: `static` (not visible to other modules).
- **Strong**: functions and initialized globals. **Weak**: uninitialized globals (only under `-fcommon`) and `__attribute__((weak))`.

Resolution rules:
1. Multiple strong symbols with the same name → link error.
2. One strong + weak(s) → strong wins.
3. Multiple weak → arbitrary pick (silent!).

```c
/* m1.c */ int x = 15213;     /* strong */
/* m2.c */ int x = 15212;     /* strong → "multiple definition" error */
```
- With `-fcommon`, `int x;` in two modules (even with different types, e.g., `int` vs `double`) links silently and can corrupt neighbors. GCC ≥ 10 defaults to `-fno-common` → uninitialized globals are strong (in `.bss`).
- Rule: share globals with `extern` declarations in one header + exactly one definition; mark module-private items `static`; compile with `-Wall`.

**Libraries**
- Static archive `libx.a`: linker takes only members that resolve currently-undefined symbols; **order matters** — objects first, libraries after (`gcc main.o -lx`); a library that appears before the object referencing it is skipped.
- Shared object: `gcc -shared -fPIC -o libx.so x.c`.
- Run-time loading (`<dlfcn.h>`): `void *h = dlopen("libx.so", RTLD_LAZY); void *f = dlsym(h, "name"); dlclose(h);` (link with `-ldl` on older glibc).
- Library interpositioning: compile-time (`#define malloc mymalloc`), link-time (`-Wl,--wrap,malloc` → `__wrap_malloc` / `__real_malloc`), run-time (`LD_PRELOAD=./mymalloc.so`).
- `__attribute__((constructor))` / `((destructor))` run before `main` / after exit.

---

## 13. Standard I/O (`<stdio.h>`)

- `FILE *` = buffered stream over a file descriptor. `stdin`, `stdout`, `stderr` predefined. `stdout`: line-buffered on a terminal, fully buffered otherwise; `stderr`: unbuffered.
- `fopen(path, "r|w|a|r+|w+|a+")`, `fclose`, `fflush`, `setvbuf`.
- Character/line: `int fgetc(FILE*)`, `getchar()` return **`int`** (needed to distinguish `EOF` = −1); `char *fgets(buf, n, fp)` reads ≤ `n−1` chars, keeps `'\n'`, `NULL` on EOF/error.
- Block: `size_t fread(ptr, size, count, fp)`, `fwrite(...)` return **count of items**; use `feof`/`ferror` to distinguish.
- `scanf`-family returns number of conversions assigned (or `EOF`); args must be **pointers**.

`printf` conversion essentials

| Spec | Argument type |
|---|---|
| `%d %i` / `%u` | `int` / `unsigned int` |
| `%ld %lu` / `%lld %llu` | `long`/`unsigned long` / `long long`/`unsigned long long` |
| `%zu %zd` | `size_t` / `ssize_t` |
| `%x %X %o` | unsigned, hex / HEX / octal |
| `%p` | `void *` |
| `%c` / `%s` | `int` char / `char *` (NUL-terminated) |
| `%f %e %g` | `double` (`%Lf` for `long double`) |
| `%%` | literal `%` |

- Modifiers: width, `.precision`, flags `-`, `0`, `+`, `#`, ` `; `%*d` takes width from argument.
- Specifier/argument mismatch is UB. Never pass user data as the format string (`printf(s)`); `%n` writes to memory (format-string attack).
- Buffered data is duplicated by `fork()` → duplicated output; `fflush(NULL)` before `fork`.
- `printf` is not async-signal-safe.

---

## 14. Systems Interfaces

### 14.1 Error Handling

- System calls return `-1` and set thread-local `errno` (`<errno.h>`); pointer-returning calls return `NULL` or `MAP_FAILED`; `pthread_*` return the error number directly (do not use `errno`); `getaddrinfo` returns codes decoded by `gai_strerror`.
- `errno` is meaningful only immediately after a failed call. Decode: `strerror(errno)`, `perror("msg")`.
- Pattern (CSAPP wrappers):

```c
void unix_error(char *msg) { fprintf(stderr, "%s: %s\n", msg, strerror(errno)); exit(1); }
pid_t Fork(void) { pid_t pid; if ((pid = fork()) < 0) unix_error("Fork error"); return pid; }
```
(The book's `unix_error` calls `exit(0)`; use a nonzero status in real code.)

### 14.2 Processes (`<unistd.h>`, `<sys/wait.h>`)

| Call | Semantics |
|---|---|
| `pid_t fork(void)` | called once, returns twice: `0` in child, child PID in parent, `-1` on error. Child gets a copy of the address space (copy-on-write), same open-file table entries. Parent/child execute concurrently; output order is nondeterministic. |
| `void exit(int status)` | runs `atexit` handlers, flushes stdio, terminates. `_exit` skips both. |
| `pid_t waitpid(pid_t pid, int *st, int opts)` / `wait(&st)` | reap child; `opts`: `WNOHANG`, `WUNTRACED`, `WCONTINUED`; returns `-1` with `ECHILD` if no children |
| `int execve(const char *path, char *const argv[], char *const envp[])` | replaces process image; returns only on error. `argv` and `envp` are `NULL`-terminated arrays. |
| `getpid`, `getppid`, `getpgrp`, `setpgid` | IDs / process groups |
| `getenv`, `setenv`, `environ` | environment |
| `sleep`, `pause`, `alarm` | timing / signal waiting |

- Status macros: `WIFEXITED(st)`, `WEXITSTATUS(st)`, `WIFSIGNALED(st)`, `WTERMSIG(st)`, `WIFSTOPPED(st)`, `WSTOPSIG(st)`.
- A terminated, unreaped child is a **zombie**; orphaned children are reparented to `init`.

```c
pid_t pid = fork();
if (pid == 0) { execve(path, argv, environ); _exit(127); }   /* child */
else if (pid > 0) { int st; waitpid(pid, &st, 0); }          /* parent */
```

### 14.3 Signals (`<signal.h>`)

- Signal handler type: `void (*)(int)`. Install with `sigaction` (portable semantics) rather than `signal`.

```c
struct sigaction sa = { .sa_handler = h, .sa_flags = SA_RESTART };
sigemptyset(&sa.sa_mask);  sigaction(SIGINT, &sa, NULL);
```

| Signal (Linux x86) | # | Default |
|---|---|---|
| SIGHUP / SIGINT / SIGQUIT | 1 / 2 / 3 | terminate |
| SIGILL / SIGABRT / SIGFPE | 4 / 6 / 8 | terminate (+core) |
| SIGKILL / SIGSTOP | 9 / 19 | terminate / stop; **cannot be caught/blocked** |
| SIGUSR1 / SIGSEGV | 10 / 11 | terminate |
| SIGPIPE / SIGALRM / SIGTERM | 13 / 14 / 15 | terminate |
| SIGCHLD | 17 | ignore |
| SIGCONT / SIGTSTP | 18 / 20 | continue / stop |

- Semantics: a pending signal of a type is a single bit (**signals are not queued**); blocked signals stay pending until unblocked; `kill(pid, sig)` (negative pid → process group), `raise`, `alarm`.
- Blocking: `sigset_t`, `sigemptyset`, `sigfillset`, `sigaddset`, `sigprocmask(SIG_BLOCK|SIG_UNBLOCK|SIG_SETMASK, &set, &old)`; `sigsuspend(&mask)` atomically unblocks and waits.
- **Safe-handler rules**:
  1. Keep handlers simple.
  2. Call only **async-signal-safe** functions (`write`, `_exit`, `kill`, `sigprocmask`, …; **not** `printf`, `malloc`).
  3. Save and restore `errno`.
  4. Block all signals while accessing shared state.
  5. Declare shared globals `volatile`; flags as `volatile sig_atomic_t`.
  6. Reap with a loop: `while (waitpid(-1, NULL, WNOHANG) > 0);` (pending SIGCHLDs coalesce).
- Race pattern: block `SIGCHLD` before `fork`, add job, then unblock (child could be reaped before the parent records it).

### 14.4 Non-local Jumps (`<setjmp.h>`)

```c
jmp_buf env;
if (setjmp(env) == 0) { /* first return: 0 */ risky(); }   /* risky() may call longjmp(env, 1) */
else { /* returned via longjmp with nonzero value */ }
```
- `longjmp` unwinds to the `setjmp` frame (must still be active). Locals changed after `setjmp` and not `volatile` are indeterminate. In handlers use `sigsetjmp`/`siglongjmp`.

### 14.5 Unix I/O (`<fcntl.h>`, `<unistd.h>`, `<sys/stat.h>`)

- File descriptor = small non-negative `int`; `0` stdin, `1` stdout, `2` stderr (`STDIN_FILENO` …).

| Call | Notes |
|---|---|
| `int open(path, flags, mode)` | flags: `O_RDONLY O_WRONLY O_RDWR` + `O_CREAT O_TRUNC O_APPEND`; `mode` masked by `umask` |
| `ssize_t read(fd, buf, n)` | returns bytes read; `0` = EOF; `-1` error |
| `ssize_t write(fd, buf, n)` | returns bytes written; `-1` error |
| `off_t lseek(fd, off, whence)` | `SEEK_SET/CUR/END` |
| `int close(fd)` | check return; leaking fds exhausts the table |
| `int dup2(old, new)` | make `new` refer to `old`'s file (redirection) |
| `int pipe(int fd[2])` | `fd[0]` read end, `fd[1]` write end |
| `stat/fstat(…, struct stat *)` | `st_mode`, `st_size`; `S_ISREG`, `S_ISDIR`, `S_ISSOCK` |
| `opendir/readdir/closedir` | directory traversal |

- **Short counts** (`read`/`write` returning `< n`) are normal on EOF, terminals, pipes, sockets. Robust code loops until all bytes are transferred and retries on `EINTR` (CSAPP RIO: `rio_readn`, `rio_writen`, buffered `rio_readlineb`, `rio_readnb`).
- Kernel structures: per-process **descriptor table** → system-wide **open file table** (offset, refcount) → **v-node table** (metadata).
  - Two `open`s of one file → two file-table entries (independent offsets).
  - `fork` → child shares file-table entries (shared offset); `dup2` also shares.
- Use Unix I/O/RIO (not stdio) for sockets; Unix I/O is async-signal-safe.

### 14.6 Network Programming (`<sys/socket.h>`, `<netdb.h>`)

- `struct sockaddr` is the generic address type; protocol-specific `struct sockaddr_in` / `sockaddr_in6` are **cast** to `(struct sockaddr *)` in calls. Ports/addresses are in network byte order (`htons`, `htonl`).
- Client: `getaddrinfo` → `socket` → `connect`. Server: `getaddrinfo` → `socket` → `bind` → `listen` → loop `accept` (returns connected fd).
- `getaddrinfo(host, service, &hints, &list)` returns a linked list of `struct addrinfo`; free with `freeaddrinfo`; protocol independent.
- Sockets are file descriptors; `read`/`write` apply (short counts!). Writing to a closed peer raises `SIGPIPE` (ignore it or use `MSG_NOSIGNAL`).

### 14.7 Threads (`<pthread.h>`, `<semaphore.h>`; compile with `-pthread`)

```c
void *routine(void *arg);
pthread_create(&tid, NULL, routine, arg);   /* returns 0 or error number */
pthread_join(tid, &retval);   pthread_detach(tid);   pthread_exit(retval);   pthread_self();
```
- Each thread: own stack, registers, `errno`, thread ID; shares code, heap, globals, `static` locals, open files.
- **Argument-passing race**: `pthread_create(&t, NULL, f, &i)` inside a loop shares `i`; pass a heap copy (`malloc`) or the value cast through `uintptr_t`.
- Synchronization:
  - Mutex: `pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER; pthread_mutex_lock(&m); … pthread_mutex_unlock(&m);`
  - Semaphore: `sem_init(&s, 0, val)`, `sem_wait(&s)` (P), `sem_post(&s)` (V).
  - Condition variable: `pthread_cond_wait(&c, &m)` inside a `while (!pred)` loop.
- `cnt++` compiles to load–update–store; concurrent execution races. `volatile` does not fix it; use mutex, semaphore, or `_Atomic` / `<stdatomic.h>`.
- Deadlock: acquire locks in a consistent global order.
- **Thread-safe** vs **reentrant**: avoid functions with hidden static state (`rand`, `strtok`, `ctime`, `localtime`, `gethostbyname`) → use `rand_r`, `strtok_r`, `localtime_r`, `getaddrinfo`.
- Data races are UB.

---

## 15. Undefined / Unspecified / Implementation-defined Behavior

**Undefined behavior** (compiler may assume it never happens; any result is allowed):

| Category | Examples |
|---|---|
| Arithmetic | signed overflow; `x/0`; `INT_MIN / -1`; shift count < 0 or ≥ width; left-shifting negative/unrepresentable values |
| Memory | NULL / dangling / wild dereference; out-of-bounds access; pointer arithmetic outside array bounds; use-after-free; double `free`; reading uninitialized automatic variable |
| Types | strict-aliasing violation; misaligned access; modifying string literal or `const`-defined object |
| Calls | `memcpy` with overlap; `restrict` violation; using the value of a non-void function that ends without `return`; `printf` format/argument mismatch |
| Order | unsequenced modification (`i = i++`) |
| Concurrency | data race |

- Compiler consequences: `x + 1 > x` for signed `x` folds to `1`; a null-check after a dereference may be deleted; loops with signed counters assume no overflow.
- **Unspecified**: order of argument/operand evaluation.
- **Implementation-defined**: sizes of types, `char` signedness, right shift of negative values, out-of-range conversion to signed, endianness, struct padding/bit-field layout.
- Detection: `-Wall -Wextra -Wconversion -Wsign-compare`, `-fsanitize=undefined,address`, Valgrind. `-fwrapv` defines signed overflow as wraparound.

---

## 16. C Construct → Machine Realization (bridge to Ch. 3–5)

| C construct | x86-64 realization |
|---|---|
| local scalar | register or stack slot; taking `&x` forces memory |
| `a[i]` (`int a[]`) | `(%rdi,%rsi,4)`; scale ∈ {1,2,4,8} |
| `p->f` | `offset(%reg)` |
| `&a[i]`, pointer arithmetic, `x*5` | `leaq` (address arithmetic doubles as integer arithmetic) |
| `x*2ᵏ`, `x/2ᵏ`, `x%2ᵏ` | `shl`, `sar`/`shr` (with bias for signed), `and`; other constants → multiply or `lea` sequences |
| `if`, `?:` | `cmp`/`test` + `jcc`; `cmov` when both arms are cheap and side-effect-free |
| loops | `-Og`: jump-to-middle; `-O1`: guarded-do; `-O2`: unrolling/vectorization |
| dense `switch` | jump table in `.rodata`, indirect `jmp *table(,%reg,8)`; sparse → compare chain |
| signed vs unsigned compare | `jl/jg` (SF, OF) vs `jb/ja` (CF) — same `cmp`, different jump |
| function call | `call` pushes return address; args in `%rdi %rsi %rdx %rcx %r8 %r9`, rest on stack; return in `%rax` (`%xmm0` for FP) |
| callee-saved | `%rbx %rbp %r12–%r15`; others are caller-saved |
| stack alignment | `%rsp` 16-byte aligned at the `call` instruction; leaf functions may use the 128-byte red zone below `%rsp` |
| struct > 16 B returned | caller passes hidden pointer in `%rdi`; callee returns it in `%rax` |
| function pointer call | `call *%rax` |
| `va_arg` | register-save area + overflow-arg area |
| VLA / `alloca` | dynamic `%rsp` adjustment; frame pointer `%rbp` used |
| global/static data | `x(%rip)` PC-relative addressing |
| string literal | `.rodata` label + `leaq .LC0(%rip), %rdi` |
| `volatile` access | forced load/store each time |
| 32-bit `int` op | `l` suffix (`addl`, `movl`); writing a 32-bit register **zeroes the upper 32 bits** |
| 64-bit `long`/pointer op | `q` suffix |
| widening | `movslq`, `cltq` (sign), `movzbl`, `movsbl` (byte → 32-bit) |
| `float`/`double` | SSE: `movss/movsd`, `addss/addsd`, `cvtsi2sd`, `cvttsd2si`, `ucomisd` |
| stack canary | value at `%fs:0x28` placed before locals, checked before `ret` |

---

## 17. C for Performance (Ch. 5–6)

**Optimization blockers**
- **Memory aliasing**: compiler cannot assume `*xp` and `*yp` differ:

```c
void twiddle1(long *xp, long *yp) { *xp += *yp; *xp += *yp; }  /* xp==yp → 4*x */
void twiddle2(long *xp, long *yp) { *xp += 2 * *yp; }          /* xp==yp → 3*x → not equivalent */
```
Fixes: local accumulators, `restrict`, hoisting loads.
- **Function calls** may have side effects: `for (i = 0; i < strlen(s); i++)` is O(n²) unless `strlen` is hoisted.
- Floating-point reassociation is not performed under IEEE semantics, so multiple accumulators must be written manually.
- Techniques: code motion, strength reduction, loop unrolling, multiple accumulators / reassociation, `static inline`, avoiding unpredictable branches (`__builtin_expect`, branchless forms).

**Locality**
- Row-major arrays: iterate with the **last index innermost** (stride-1).

```c
for (i = 0; i < N; i++) for (j = 0; j < N; j++) sum += a[i][j];   /* good */
for (j = 0; j < N; j++) for (i = 0; i < N; i++) sum += a[i][j];   /* stride N*sizeof(int): poor */
```
- Matrix multiply loop orders: `kij`/`ikj` (stride-1 inner) beats `ijk`/`jik`, and `jki`/`kji` is worst.
- Blocking (tiling) keeps a working set within cache; pad arrays (`[N][N+1]`) to avoid power-of-two conflict misses; array-of-structs vs struct-of-arrays decides which fields share cache lines.
- Timing: `clock_gettime(CLOCK_MONOTONIC, …)`; measure with `-O2`, repeat runs.

---

## 18. GCC Extensions and C11 Features Used

| Feature | Form |
|---|---|
| Inline assembly | `asm volatile ("addq %1, %0" : "+r"(x) : "r"(y) : "cc", "memory");` (outputs : inputs : clobbers) |
| Attributes | `__attribute__((packed, aligned(16), noreturn, unused, weak, constructor))` |
| Builtins | `__builtin_expect`, `__builtin_popcount`, `__builtin_clz`, `__builtin_ctz`, `__builtin_add_overflow` / `sub` / `mul`, `__builtin_bswap32` |
| `typeof(x)` | type of expression |
| Statement expressions | `({ int t = f(); t * 2; })` |
| Computed goto | `goto *ptr; &&label` |
| `_Static_assert(cond, "msg")` | compile-time check |
| `_Alignas`, `_Alignof` | alignment control |
| `_Generic` | type-based selection |
| `_Atomic`, `<stdatomic.h>` | atomic types/operations |
| `_Thread_local` | per-thread storage |
| `<stdbool.h>`, `<stdint.h>`, `<inttypes.h>` | `bool`; exact-width ints; `PRIu64`, `PRId32` printf macros |
| Designated initializers, compound literals, VLAs, `//` comments, mixed declarations | C99 |

---

## 19. Memory Bug Catalog (CSAPP §9.11)

| # | Bug | Wrong | Right |
|---|---|---|---|
| 1 | Dereferencing a bad pointer | `scanf("%d", val);` | `scanf("%d", &val);` |
| 2 | Reading uninitialized memory | assuming `malloc` zeroes | `calloc` or explicit init |
| 3 | Stack buffer overflow | `char buf[8]; gets(buf);` | `fgets(buf, sizeof buf, stdin);` |
| 4 | Pointer/object size mismatch | `int **A = malloc(n * sizeof(int));` | `malloc(n * sizeof(int *))` / `sizeof *A` |
| 5 | Off-by-one | `for (i = 0; i <= n; i++) a[i]`; `malloc(strlen(s))` | `i < n`; `malloc(strlen(s) + 1)` |
| 6 | Pointer instead of pointee | `*size--;` | `(*size)--;` |
| 7 | Pointer-arithmetic scale error | `p += sizeof(int);` (skips 4 elements) | `p++;` |
| 8 | Reference to nonexistent variable | `return &local;` | return value, or caller-supplied buffer, or heap |
| 9 | Use after free | `free(p); *p = 1;` | `free(p); p = NULL;` |
| 10 | Memory leak | losing last pointer to a block | pair every allocation with `free`; Valgrind |
| 11 | Double free / invalid free | `free(p); free(p);` / `free(&x)` | free once; only allocator pointers |
| 12 | `realloc` leak | `p = realloc(p, n);` | assign through temp (§10) |
| 13 | Missing `&`/`sizeof` on array-decayed param | `sizeof arr` in callee = 8 | pass length explicitly |

---

## 20. Tool Cheat Sheet

| Tool | Command | Purpose |
|---|---|---|
| gcc | `gcc -Og -S -fno-asynchronous-unwind-tables x.c` | readable assembly |
| objdump | `objdump -d x.o` / `-d -j .text` | disassemble |
| | `objdump -t`, `-r`, `-s -j .rodata`, `-h` | symbols, relocations, section dump, headers |
| readelf | `readelf -h -S -s -r a.out` | ELF header, sections, symbols, relocs |
| nm / size / strings | `nm a.out`, `size a.out`, `strings a.out` | symbols, section sizes, text |
| ldd / ltrace / strace | `ldd a.out`, `ltrace`, `strace` | shared libs, library calls, system calls |
| gdb | `break f`, `run args`, `si` / `ni`, `finish`, `bt`, `layout asm` | control |
| | `disas`, `info registers`, `p/x $rax`, `x/8xb $rsp`, `x/4gx $rsp`, `x/s addr`, `display/i $pc` | inspect |
| | `watch var`, `info frame`, `p *array@n` | watch / frame / array view |
| valgrind | `valgrind --leak-check=full ./a.out` | leaks, invalid access |
| ASan/UBSan | `-fsanitize=address,undefined` | runtime errors |
| make | `make`, `make clean` | build |

---

## 21. Verification Exercises (answers below each)

1. `-1 < 0U` → **0**.
2. `(unsigned char)(200 + 100)` → **44** (300 mod 256).
3. `sizeof(int) - 5 < 0` → **0** (`size_t` arithmetic wraps).
4. `short s = -1; (unsigned)s` → **4294967295** (sign-extend, then reinterpret).
5. `char c = 0x80; (unsigned)c` (signed `char`) → **4294967168** (`0xFFFFFF80`).
6. `-7 / 2`, `-7 % 2`, `-7 >> 1` → **-3, -1, -4**.
7. `int a[4]; sizeof a / sizeof a[0]`, `&a[3] - &a[0]`, `(char *)&a[3] - (char *)&a[0]` → **4, 3, 12**.
8. `int a[3][4]; &a[1][2]` offset from `a` → **(1·4 + 2)·4 = 24 bytes**.
9. `struct { char a; int b; char c; }` size → **12**; reordered `{int b; char a, c;}` → **8**.
10. `int x = 0x01234567; *(unsigned char *)&x` → **0x67** (little-endian).
11. `x & 1 == 0` parses as → **`x & (1 == 0)`** → always `0`.
12. `(float)16777217` → **16777216.0f** (round-to-nearest-even; 2²⁴+1 unrepresentable).
13. `0.1 + 0.2 == 0.3` → **0**.
14. `~0`, `!!5`, `-(-2147483647-1)` → **-1, 1, `INT_MIN`** (last is UB by the standard; wraps in practice).
15. `char *s = "abc"; s[0] = 'x';` → **UB; SIGSEGV** (`.rodata` is read-only).
16. `uint8_t a = 0xFF; a == ~a` → **0**; `~a` is `int` `-256`.
17. `int i = 5; i = i++ + ++i;` → **UB** (unsequenced modifications).
18. `1 << 31` (`int`) → **UB**; use `1u << 31` → `0x80000000`.
19. `unsigned i; for (i = n; i >= 0; i--)` → **infinite loop** (`i >= 0` always true).
20. `fork(); fork(); printf("x");` (stdout to terminal, no newline buffering effects ignored) → **4 processes** each print once → 4 `x`.