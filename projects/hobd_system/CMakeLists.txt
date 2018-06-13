cmake_minimum_required(VERSION 3.7.2)

project(hobd-system C)

set(configure_string "")

config_option(
    UserDebugBuild
    USER_DEBUG_BUILD
    "Enable user debugging."
    DEFAULT ON
)

ApplyCommonSimulationSettings()

ApplyData61ElfLoaderSettings()

add_executable(
    hobd-system
    src/main.c)

add_config_library(
    hobd-system
    "${configure_string}")

set_property(
    TARGET
    hobd-system
    PROPERTY
    C_STANDARD
    99)

target_compile_options(
    hobd-system
    PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wwrite-strings
    -Wstrict-prototypes
#    -Wmissing-include-dirs
    -Werror)

target_include_directories(
    hobd-system
    PRIVATE
    include)

target_link_libraries(
    hobd-system
    Configuration
    muslc
    sel4
    sel4allocman
    sel4vka
    sel4utils
    sel4platsupport
    sel4muslcsys
    sel4simple-default
    sel4debug
    platsupport
    utils
    cpio
    elf)

DeclareRootserver(hobd-system)

GenerateSimulateScript()
