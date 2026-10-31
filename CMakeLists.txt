cmake_minimum_required(VERSION 3.13)

# Set the project name
project(bigfiles VERSION 1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Extra build types this compiler supports. MSVC only has AddressSanitizer;
# MemorySanitizer needs Clang on Linux.
set(EXTRA_BUILD_TYPES ASAN)
if (NOT MSVC)
    list(APPEND EXTRA_BUILD_TYPES UBSAN TSAN COVERAGE)
endif()
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND EXTRA_BUILD_TYPES MSAN)
endif()

# Multi-config generators (Visual Studio, Xcode) only offer the supported
# types; single-config generators fail early on an unsupported one.
get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (IS_MULTI_CONFIG)
    set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo ${EXTRA_BUILD_TYPES})
else()
    string(TOUPPER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_UPPER)
    if (BUILD_TYPE_UPPER MATCHES "^(ASAN|UBSAN|TSAN|MSAN|COVERAGE)$"
        AND NOT BUILD_TYPE_UPPER IN_LIST EXTRA_BUILD_TYPES)
        message(FATAL_ERROR "${CMAKE_BUILD_TYPE} builds are not supported with "
            "${CMAKE_CXX_COMPILER_ID} on ${CMAKE_SYSTEM_NAME}")
    endif()
endif()

# Flags for the extra build types
if (MSVC)
    # MSVC enables ASan at compile time; the linker needs no flag.
    set(CMAKE_CXX_FLAGS_ASAN "/fsanitize=address")
else()
    set(CMAKE_CXX_FLAGS_ASAN "-fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS_ASAN "-fsanitize=address")

    set(CMAKE_CXX_FLAGS_UBSAN "-fsanitize=undefined -fno-sanitize-recover=all")
    set(CMAKE_EXE_LINKER_FLAGS_UBSAN "-fsanitize=undefined")

    set(CMAKE_CXX_FLAGS_TSAN "-fsanitize=thread")
    set(CMAKE_EXE_LINKER_FLAGS_TSAN "-fsanitize=thread")

    set(CMAKE_CXX_FLAGS_MSAN "-fsanitize=memory -fsanitize-memory-track-origins")
    set(CMAKE_EXE_LINKER_FLAGS_MSAN "-fsanitize=memory")

    set(CMAKE_CXX_FLAGS_COVERAGE "--coverage -O0 -g")
    set(CMAKE_EXE_LINKER_FLAGS_COVERAGE "--coverage")
endif()

# Add the executable
add_executable(bigfiles main.cc)

# Warnings
if (MSVC)
    target_compile_options(bigfiles PRIVATE /W4)
else()
    target_compile_options(bigfiles PRIVATE -Wall -Wextra -pedantic)
endif()

# Tests
enable_testing()
if (NOT WIN32)
    add_test(NAME smoke_test
        COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke_test.sh $<TARGET_FILE:bigfiles>)
endif()

# macOS specific settings
if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set_target_properties(bigfiles PROPERTIES
        OSX_ARCHITECTURES "x86_64;arm64"
    )
endif()
