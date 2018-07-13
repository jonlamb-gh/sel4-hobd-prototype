cmake_minimum_required(VERSION 3.7.2)

project(hobd-system C)

set(configure_string "")

config_option(
    UserDebugBuild
    USER_DEBUG_BUILD
    "Enable user debugging."
    DEFAULT ON
)

# enable all 4 cores in QEMU
set(QemuFlags "-smp cores=4")

ApplyCommonSimulationSettings()

ApplyData61ElfLoaderSettings()

add_executable(
    hobd-system
    modules/time_server/src/time_server_module.c
    modules/mmc/src/mmc_module.c
    modules/console/src/cli.c
    modules/console/src/console_module.c
    modules/hobd/src/hobd_parser.c
    modules/hobd/src/comm.c
    modules/hobd/src/hobd_module.c
    modules/system/src/system_module.c
    src/platform.c
    src/root_task.c
    src/thread.c
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
    -Wshadow
    -Wno-unused-parameter
    -Wwrite-strings
    -Wstrict-prototypes
    -Wold-style-definition
    -Wnested-externs
#    -Wmissing-include-dirs
    -Wno-unused-result
    -Werror)

target_compile_definitions(
    hobd-system
    PRIVATE
    FATFS_USE_CUSTOM_OPTS_FILE)

target_include_directories(
    hobd-system
    PRIVATE
    modules/time_server/include
    modules/mmc/include
    modules/console/include
    modules/hobd/include
    modules/system/include
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
    sel4sync
    sel4debug
    platsupport
    utils
    cpio
    elf
    sdhcdrivers
    fatio
    microrl)

DeclareRootserver(hobd-system)

GenerateSimulateScript()
