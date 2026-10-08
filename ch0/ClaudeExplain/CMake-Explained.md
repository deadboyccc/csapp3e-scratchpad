# CMake explained for this project

This file configures a C project that builds one executable per chapter directory (`ch0`, `ch1`, ...). It sets compiler standards, debug flags, sanitizer support, and common warning/link settings for every chapter target.

## 1) Required CMake version and project definition

```cmake
cmake_minimum_required(VERSION 4.3)
project(CSAPP3e_study LANGUAGES C)
```

- `cmake_minimum_required(VERSION 4.3)`: tells CMake the minimum required version. If the environment has an older CMake, configuration stops.
- `project(CSAPP3e_study LANGUAGES C)`: declares the project name and says this project uses the C language.

This is the entry point of the build.

## 2) C language standard

```cmake
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
```

- `CMAKE_C_STANDARD 23`: requests C23.
- `CMAKE_C_STANDARD_REQUIRED ON`: CMake will error if the compiler cannot fully support C23.
- `CMAKE_C_EXTENSIONS ON`: allows compiler-specific GNU extensions. This is intentionally set so the project can use POSIX/GNU-style APIs (for example, `fork`, `signals`, `sockets`, `asm`). Without this, some compilers may switch to a stricter non-extended mode.

In short: project wants modern C, but still allows the GNU extension set needed by low-level systems programming.

## 3) Compile database for tooling

```cmake
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
```

This generates a `compile_commands.json` file. Tools like `clangd`, IDE indexers, and some linters use it to know the exact compiler flags for each source file.

This is important for editor support and accurate code navigation.

## 4) Default build type

```cmake
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()
```

- If the user did not specify a build type (`Debug`, `Release`, `RelWithDebInfo`, etc.), this sets it to `Debug`.
- `CACHE STRING "" FORCE` stores it in the CMake cache so it persists across builds.
- This ensures the project defaults to a debuggable configuration.

## 5) Debug flags

```cmake
set(CMAKE_C_FLAGS_DEBUG "-Og -g3")
```

This appends debug-specific compiler flags for the `Debug` configuration:

- `-O0`/`-Og`: optimize for debugging readability rather than runtime speed.
- `-g3`: include high-quality debug symbols, more complete than minimal `-g`.

This is a common choice for CSAPP-style code where readable assembly and debugging are more valuable than optimization.

## 6) Optional sanitizer support

```cmake
option(STUDY_SANITIZE "Enable ASan + UBSan" OFF)
```

Creates a user-settable CMake option:

- default value: `OFF`
- name: `STUDY_SANITIZE`
- description: `Enable ASan + UBSan`

This lets the project optionally build with sanitizers when enabled via CMake, for example:

```bash
cmake -DSTUDY_SANITIZE=ON ..
```

## 7) Thread library

```cmake
find_package(Threads REQUIRED)
```

This finds the system threading library and makes the imported target `Threads::Threads` available.

It is required because the project uses pthreads or thread-related APIs in C code, and CMake handles platform differences automatically.

## 8) Shared compile/link settings

```cmake
add_library(study_options INTERFACE)
```

Creates an interface library. It does not build an actual object file; it only carries compile and link properties to other targets.

This is a clean pattern for common flags that apply to every chapter executable.

```cmake
target_compile_options(study_options INTERFACE
  -Wall -Wextra -Wconversion -Wsign-compare -fno-common)
```

These flags are added to any target linking against `study_options`:

- `-Wall`: enable most warning checks
- `-Wextra`: enable extra warnings beyond `-Wall`
- `-Wconversion`: warn about implicit conversions that may change values
- `-Wsign-compare`: warn about signed/unsigned comparisons
- `-fno-common`: avoid old-style global variable collisions from `-fcommon` behavior

This makes the code stricter and easier to debug.

```cmake
target_link_libraries(study_options INTERFACE Threads::Threads m)
```

Links the common target against:

- `Threads::Threads`: pthread support
- `m`: the C math library (`libm`), commonly needed for functions like `sin`, `sqrt`, `pow`, etc.

Because this is an `INTERFACE` library, these are propagated to all chapter executables that link against it.

## 9) Sanitizer configuration

```cmake
if(STUDY_SANITIZE)
  target_compile_options(study_options INTERFACE
    -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(study_options INTERFACE -fsanitize=address,undefined)
endif()
```

When `STUDY_SANITIZE=ON`:

- `-fsanitize=address,undefined` enables AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan)
- `-fno-omit-frame-pointer` keeps frame pointers for better sanitizer reports
- `target_link_options` makes the linker add the same sanitizers at link time

This gives cleaner catch points for memory errors and undefined behavior during development.

## 10) Discover chapter directories and build one executable per chapter

```cmake
file(GLOB chapter_dirs RELATIVE ${CMAKE_SOURCE_DIR} CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/ch*)
```

This collects all top-level directories whose names start with `ch`:

- example: `ch0`, `ch1`, `ch2`, etc.
- `RELATIVE ${CMAKE_SOURCE_DIR}` makes each entry relative to the project root
- `CONFIGURE_DEPENDS` tells CMake to re-detect changes automatically when directories are added or removed

This is a simple project layout: source files are grouped by chapter folder.

```cmake
foreach(dir IN LISTS chapter_dirs)
  if(IS_DIRECTORY ${CMAKE_SOURCE_DIR}/${dir})
```

Loop over each chapter folder and only continue if it really is a directory.

## 11) Gather source files inside each chapter

```cmake
file(GLOB srcs CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/${dir}/*.c)
```

This finds all `.c` files in the current chapter directory.

Example:

```text
ch1/main.c
ch1/bitops.c
ch1/printf_demo.c
```

`srcs` becomes a list of those files.

## 12) Create one executable per chapter

```cmake
if(srcs)
  add_executable(${dir} ${srcs})
```

If the chapter contains any C source files, an executable is created with the same name as the chapter directory.

Example:

- `ch0` -> executable target named `ch0`
- `ch1` -> executable target named `ch1`

This avoids writing a separate `add_executable` line for each chapter by auto-discovering them.

## 13) Include directories and link common settings

```cmake
target_include_directories(${dir} PRIVATE ${CMAKE_SOURCE_DIR}/${dir} ${CMAKE_SOURCE_DIR}/include)
```

Adds include paths for the target:

- the chapter's own directory
- the top-level `include` directory

This allows source files in a chapter to include local headers and shared project headers without manual path fiddling.

```cmake
target_link_libraries(${dir} PRIVATE study_options)
```

Links each chapter executable to the shared `study_options` interface library.

This propagates the warning flags, math library, and thread library to every chapter target.

## 14) Final behavior of the project

This CMakeLists does the following in one build system:

1. Require a modern C toolchain.
2. Set up Debug builds by default.
3. Enable useful warnings and strict compile settings.
4. Optionally enable sanitizers.
5. Discover each chapter directory.
6. Compile every `.c` file in that chapter into an executable.
7. Apply common include and link settings to each chapter binary.

## 15) Why this structure fits the project

This is a good fit for a study repository with many independent chapter programs:

- no need to maintain a long explicit target list
- adding a new chapter folder automatically creates a new executable
- shared compile settings stay consistent across chapters
- toolchain hygiene is built in (`-Wall`, sanitizers, debug symbols)

## 16) Typical build

```bash
cmake -S . -B build
cmake --build build
```

Then each chapter is built as a separate executable under the build directory.

## 17) One-line summary

This file is a compact build configuration that turns the project into a collection of chapter-based C executables, with strict warnings, debug-friendly settings, optional sanitizers, and shared link-time configuration.
