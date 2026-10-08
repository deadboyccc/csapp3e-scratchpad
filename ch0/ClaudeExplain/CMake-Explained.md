# CMake Reference: CSAPP3e Study Project

**Layout:** one executable per chapter directory (`ch0/`, `ch1/`, …), shared flags, optional sanitizers, auto-discovered sources.

```text
.
├── CMakeLists.txt
├── include/          # shared headers
├── ch0/main.c
├── ch1/*.c
└── ...
```

---

## 1. Full File

```cmake
cmake_minimum_required(VERSION 4.3)
project(CSAPP3e_study LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()
set(CMAKE_C_FLAGS_DEBUG "-Og -g3")

option(STUDY_SANITIZE "Enable ASan + UBSan" OFF)
find_package(Threads REQUIRED)

add_library(study_options INTERFACE)
target_compile_options(study_options INTERFACE
  -Wall -Wextra -Wconversion -Wsign-compare -fno-common)
target_link_libraries(study_options INTERFACE Threads::Threads m)

if(STUDY_SANITIZE)
  target_compile_options(study_options INTERFACE
    -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(study_options INTERFACE -fsanitize=address,undefined)
endif()

file(GLOB chapter_dirs RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS
     ${PROJECT_SOURCE_DIR}/ch[0-9]*)
foreach(dir IN LISTS chapter_dirs)
  if(IS_DIRECTORY ${PROJECT_SOURCE_DIR}/${dir})
    file(GLOB srcs CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/${dir}/*.c)
    if(srcs)
      add_executable(${dir} ${srcs})
      target_include_directories(${dir} PRIVATE
        ${PROJECT_SOURCE_DIR}/${dir} ${PROJECT_SOURCE_DIR}/include)
      target_link_libraries(${dir} PRIVATE study_options)
    endif()
  endif()
endforeach()
```

Changes versus the earlier version: `CMAKE_SOURCE_DIR` → `PROJECT_SOURCE_DIR` (correct when this project is added via `add_subdirectory`), and glob `ch*` → `ch[0-9]*` (does not match unrelated names such as `check/` or `charts.txt`).

---

## 2. Project Declaration

```cmake
cmake_minimum_required(VERSION 4.3)
project(CSAPP3e_study LANGUAGES C)
```

| Item | Effect |
|---|---|
| `cmake_minimum_required(VERSION X)` | Fails configuration on older CMake **and** sets all policies introduced up to `X` to `NEW`. Must come first. |
| `VERSION 3.25...4.3` | Range form: minimum 3.25, policy behavior of 4.3 when available. Lowers the hard requirement without losing new behavior. |
| `project(... LANGUAGES C)` | Sets `PROJECT_NAME`, `PROJECT_SOURCE_DIR`, `PROJECT_BINARY_DIR`; enables only C (default would also probe C++, adding configure time and a C++ compiler requirement). |

---

## 3. Language Standard

```cmake
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
```

| Variable | Effect |
|---|---|
| `CMAKE_C_STANDARD 23` | Requests C23. Emitted flag: `-std=gnu23` (GCC ≥ 14, Clang ≥ 18), `-std=gnu2x` on older compilers. |
| `CMAKE_C_STANDARD_REQUIRED ON` | Configure **fails** if the compiler lacks the standard. `OFF` silently decays to the closest older standard. |
| `CMAKE_C_EXTENSIONS ON` | Emits `-std=gnuXX`. `OFF` emits `-std=cXX` (strict ISO). |

Why `gnu23` and not strict `c23`:
- Strict mode defines `__STRICT_ANSI__`; glibc then hides POSIX/GNU declarations unless a feature-test macro (`_POSIX_C_SOURCE`, `_GNU_SOURCE`) is defined. Affected: parts of `fork`/`signal`/`sigaction`, sockets, `strdup`-class functions, `MAP_ANONYMOUS`.
- The `asm` keyword and `typeof` forms are GNU extensions in older modes (`__asm__` always works).

Ordering constraint: `CMAKE_C_*` variables initialize the `C_STANDARD` / `C_EXTENSIONS` properties **when a target is created**. Set them before every `add_executable`.

---

## 4. Tooling Support

```cmake
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
```

- Writes `compile_commands.json` into the **build** directory, containing the exact compiler command per source file.
- Honored only by Makefile and Ninja generators.
- `clangd` searches the project root and `build/`; for other build directory names pass `--compile-commands-dir=<dir>` or symlink the file into the root.

---

## 5. Build Type and Debug Flags

```cmake
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()
set(CMAKE_C_FLAGS_DEBUG "-Og -g3")
```

| Element | Meaning |
|---|---|
| `CMAKE_BUILD_TYPE` | Selects the config for **single-config** generators (Makefiles, Ninja). Empty by default → no optimization or debug flags. |
| `CMAKE_CONFIGURATION_TYPES` | Non-empty for **multi-config** generators (Visual Studio, Xcode, Ninja Multi-Config); there, the config is chosen at build time (`--config`), so the guard skips setting a type. |
| `CACHE ... FORCE` | Stores the default in the cache; the guard ensures a user-supplied `-DCMAKE_BUILD_TYPE=...` is never overwritten. |
| `CMAKE_C_FLAGS_DEBUG` | **Replaces** the default (`-g`) for Debug. Flags apply only when the config is `Debug`. |
| `-Og` | Optimizations that preserve debuggability. Variables can still appear as `<optimized out>`; switch to `-O0` when that blocks inspection. For Ch. 3, `-Og` yields assembly closest to the book's listings. |
| `-g3` | Full DWARF plus **macro definitions** (usable in gdb: `info macro NAME`, `macro expand EXPR`). |

---

## 6. Sanitizer Option

```cmake
option(STUDY_SANITIZE "Enable ASan + UBSan" OFF)
```

- Defines a cache `BOOL`; set with `-DSTUDY_SANITIZE=ON`.
- Cached: changing it requires re-running configure (it reuses the cached value otherwise).

---

## 7. Threads

```cmake
find_package(Threads REQUIRED)
```

- Provides imported target `Threads::Threads`, which adds `-pthread` or `-lpthread` as the platform needs.
- On glibc ≥ 2.34 `libpthread` is merged into `libc`, but the target remains the portable form and also sets `-pthread` for compile-time macros.
- Needed by Ch. 12 (concurrency) code only; harmless elsewhere.

---

## 8. Shared Options Target

```cmake
add_library(study_options INTERFACE)
target_compile_options(study_options INTERFACE -Wall -Wextra -Wconversion -Wsign-compare -fno-common)
target_link_libraries(study_options INTERFACE Threads::Threads m)
```

An `INTERFACE` library has no sources or artifacts; its properties are **usage requirements** forwarded to every target that links it.

| Flag | Effect (C) |
|---|---|
| `-Wall -Wextra` | Core and extended warnings; `-Wextra` already includes `-Wsign-compare` in C. |
| `-Wconversion` | Warns on implicit conversions that may change a value; in C this also enables `-Wsign-conversion`. Targets Ch. 2 bugs: truncation, signed ↔ unsigned. |
| `-Wsign-compare` | Signed/unsigned comparison (`-1 < 0U`). Explicit for clarity. |
| `-fno-common` | Uninitialized globals become strong symbols in `.bss`; duplicate definitions across files fail at link time instead of silently merging (default since GCC 10; explicit for older/other compilers). |
| `m` | `libm` (`sqrt`, `pow`, `sin`, …). glibc does not link it implicitly; calls constant-folded at compile time may hide the need. |

---

## 9. Sanitizers

```cmake
if(STUDY_SANITIZE)
  target_compile_options(study_options INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(study_options INTERFACE -fsanitize=address,undefined)
endif()
```

| Item | Detail |
|---|---|
| ASan | Heap/stack/global out-of-bounds, use-after-free, double free, leaks (LeakSanitizer on Linux) |
| UBSan | Signed overflow, invalid shifts, null/misaligned access, `INT_MIN / -1`, … |
| Compile **and** link flag | The runtime library is linked by the same flag; omitting it at link time yields undefined sanitizer symbols. |
| `-fno-omit-frame-pointer` | Reliable stack traces in reports. |
| Limits | Incompatible with `-static`, `-fsanitize=thread`, and Valgrind. Adds red zones and shadow memory, **changing stack/heap layout**: build layout-dependent experiments (Ch. 3 buffer overflow, Ch. 9 allocator addresses) without it. |

---

## 10. Chapter Discovery and Targets

```cmake
file(GLOB chapter_dirs RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/ch[0-9]*)
foreach(dir IN LISTS chapter_dirs)
  if(IS_DIRECTORY ${PROJECT_SOURCE_DIR}/${dir})
    file(GLOB srcs CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/${dir}/*.c)
    if(srcs)
      add_executable(${dir} ${srcs})
      ...
```

| Element | Detail |
|---|---|
| `file(GLOB ...)` | Evaluated at configure time; the result is a snapshot. |
| `CONFIGURE_DEPENDS` | Re-runs the glob at **build** time and re-configures when the result changes, so added/removed files and directories are detected. Cost: one directory scan per build. CMake documentation discourages `GLOB` for sources; acceptable here because targets are intentionally implicit. |
| `RELATIVE` | Entries become `ch1` instead of absolute paths; the loop variable doubles as target name. |
| `IS_DIRECTORY` | Excludes files matching `ch[0-9]*`. |
| `if(srcs)` | Skips chapters without `.c` files (no empty target). |
| Target name = directory name | `ch1` → target `ch1`, binary `<build>/ch1`. |
| Rule | All `.c` files in a chapter directory link into **one** executable: exactly one `main` per directory. Standalone experiments need their own directory. |

```cmake
target_include_directories(${dir} PRIVATE ${PROJECT_SOURCE_DIR}/${dir} ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(${dir} PRIVATE study_options)
```

- `#include "x.h"` already searches the including file's directory; the chapter directory entry matters only for `#include <x.h>`.
- `PRIVATE` is correct for executables (nothing links against them).
- Linking `study_options` is the only step that applies warnings, sanitizers, `Threads`, and `m`; a target that skips it gets none of them.

---

## 11. Per-target Overrides

Targets exist only after the loop; overrides must come **after** it.

```cmake
if(TARGET ch3)
  target_compile_options(ch3 PRIVATE -fno-stack-protector -fcf-protection=none)
  target_link_options(ch3 PRIVATE -no-pie)
endif()
```

| Flag | Purpose |
|---|---|
| `-fno-stack-protector` | Remove canary (buffer-overflow experiments) |
| `-no-pie` | Fixed code addresses |
| `-fcf-protection=none` | Drop `endbr64` landing pads for cleaner disassembly |
| `-z execstack` (link) | Executable stack for code-injection experiments |

---

## 12. Commands

```bash
cmake -S . -B build                         # configure (Debug by default)
cmake --build build -j                      # build all chapters
cmake --build build --target ch1            # build one chapter
./build/ch1                                 # run

cmake -S . -B build-asan -DSTUDY_SANITIZE=ON    # separate tree for sanitizers
cmake -S . -B build-rel  -DCMAKE_BUILD_TYPE=Release
cmake -S . -B build -G Ninja                # faster incremental builds
```

- One build directory per configuration; the cache fixes `CMAKE_BUILD_TYPE` and `STUDY_SANITIZE` per tree.
- `-DCMAKE_C_COMPILER=clang` selects the compiler at first configure only (delete the build directory to change it).

---

## 13. Pitfalls

| Pitfall | Cause / Fix |
|---|---|
| New chapter not built | Glob not re-run → `CONFIGURE_DEPENDS` handles it; verify the directory name matches `ch[0-9]*`. |
| `multiple definition of main` | Two `main` functions in one chapter directory. |
| `undefined reference to sqrt` | Target does not link `study_options` (or `m`). |
| `unknown type name` / missing POSIX symbol | `CMAKE_C_EXTENSIONS OFF` or missing feature-test macro. |
| Sanitizer flag ignored | Cached `STUDY_SANITIZE=OFF` in an old build tree; pass `-D` explicitly or use a fresh directory. |
| `-std=c23` rejected | Compiler older than GCC 14 / Clang 18 with `STANDARD_REQUIRED ON`. |
| Debugger shows `<optimized out>` | Use `-O0` in `CMAKE_C_FLAGS_DEBUG`. |
