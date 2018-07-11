cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(hobd-log-entry-dump VERSION 0.1 LANGUAGES C)

add_executable(
    hobd-log-entry-dump
    src/main.c)

set_property(
    TARGET
    hobd-log-entry-dump
    PROPERTY
    C_STANDARD
    99)

target_compile_options(
    hobd-log-entry-dump
    PRIVATE
    -Wall
    -Wextra
    -Wshadow
    -Wno-unused-parameter
    -Wwrite-strings
    -Wstrict-prototypes
    -Wold-style-definition
    -Wnested-externs
    -Wmissing-include-dirs
    -Wno-unused-result
    -Werror)

target_include_directories(
    hobd-log-entry-dump
    PRIVATE
    ../../projects/hobd-system/modules/hobd/include
    ../../projects/hobd-system/modules/mmc/include)

target_link_libraries(
    hobd-log-entry-dump
    -static-libgcc
    -static
    -lpopt)

install(
    TARGETS
    hobd-log-entry-dump
    DESTINATION
    /usr/local/bin)
