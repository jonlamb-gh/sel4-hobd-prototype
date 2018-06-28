cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(fake-hobd-ecu-data VERSION 0.1 LANGUAGES C)

add_executable(
    fake-hobd-ecu-data
    src/hobd_parser.c
    src/ecu.c
    src/main.c)

set_property(
    TARGET
    fake-hobd-ecu-data
    PROPERTY
    C_STANDARD
    99)

target_compile_options(
    fake-hobd-ecu-data
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
    fake-hobd-ecu-data
    PRIVATE
    ../../projects/hobd_system/modules/hobd/include
    include)

target_link_libraries(
    fake-hobd-ecu-data
    -static-libgcc
    -static
    -lpopt)

install(
    TARGETS
    fake-hobd-ecu-data
    DESTINATION
    /usr/local/bin)

