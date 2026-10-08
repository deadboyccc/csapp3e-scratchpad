cmake_minimum_required(VERSION 4.3)
project(CSAPP3e_study LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)               # gnu23: POSIX/GNU APIs (fork, signals, sockets, asm)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)    # clangd / IDE indexing

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()
set(CMAKE_C_FLAGS_DEBUG "-Og -g3")       # readable asm + full debug info

option(STUDY_SANITIZE "Enable ASan + UBSan" OFF)

find_package(Threads REQUIRED)

# Shared settings, applied to every chapter target
add_library(study_options INTERFACE)
target_compile_options(study_options INTERFACE
  -Wall -Wextra -Wconversion -Wsign-compare -fno-common)
target_link_libraries(study_options INTERFACE Threads::Threads m)

if(STUDY_SANITIZE)
  target_compile_options(study_options INTERFACE
    -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(study_options INTERFACE -fsanitize=address,undefined)
endif()

# One executable per chapter directory (ch0, ch1, ...), all .c files inside
file(GLOB chapter_dirs RELATIVE ${CMAKE_SOURCE_DIR} CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/ch*)
foreach(dir IN LISTS chapter_dirs)
  if(IS_DIRECTORY ${CMAKE_SOURCE_DIR}/${dir})
    file(GLOB srcs CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/${dir}/*.c)
    if(srcs)
      add_executable(${dir} ${srcs})
      target_include_directories(${dir} PRIVATE ${CMAKE_SOURCE_DIR}/${dir} ${CMAKE_SOURCE_DIR}/include)
      target_link_libraries(${dir} PRIVATE study_options)
    endif()
  endif()
endforeach()
