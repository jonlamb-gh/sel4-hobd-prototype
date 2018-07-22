# sel4-hobd-prototype

Prototype HOBD system running on the [seL4](https://sel4.systems/) microkernel.

Manages a diagnostic connection to the Honda CBR ECU and logs data to an SD card.

There is a simple console available for interacting with the system.

Supported platforms:

- Sabre Lite IMx6, quad core A9

See the [devel](https://github.com/jonlamb-gh/sel4-hobd-prototype/tree/devel) branch for the most recent developments.

## Links

- [seL4 manual](https://sel4.systems/Info/Docs/seL4-manual-latest.pdf)
- [IMX6 RM](http://cache.freescale.com/files/32bit/doc/ref_manual/IMX6DQRM.pdf)
- [HW user manual](https://1quxc51443zg3oix7e35dnvg-wpengine.netdna-ssl.com/wp-content/uploads/2014/11/SABRE_Lite_Hardware_Manual_rev11.pdf)
- [HW components](https://1quxc51443zg3oix7e35dnvg-wpengine.netdna-ssl.com/wp-content/uploads/2014/11/sabre_lite-revD.pdf)
- [GPIO mapping guide](https://www.kosagi.com/w/index.php?title=Definitive_GPIO_guide)
- [Honda OBD Data tables](http://projects.gonzos.net/wp-content/uploads/2015/09/Honda-data-tables.pdf)

## Project Files
- [CMake config for this project](configs/imx6_sabre_lite.cmake)
- [Main project root directory](projects/hobd-system)
- [ECU data generator tool](testing-tools/fake-hobd-ecu-data/README.md)
- [MMC entry log file dump tool](testing-tools/hobd-log-entry-dump/README.md)

## Building

```bash
# also runs `git submodule update --init`
./scripts/apply-patches

./scripts/build
```

## SMP / Multicore

By default, 4 cores are enabled to match my hardware.

See CMake config `KernelMaxNumNodes` is in `configs/imx6_sabre_lite.cmake`.

My project sets the QEMU option `QemuFlags` to add the
following (note that it must be >= `KernelMaxNumNodes`):

```base
-smp cores=4
```

Affinity for the threads used in this project are in [config.h](projects/hobd-system/include/config.h).

## GPIO

```bash
CSI0_DAT10 | ALT3 | UART1_TX_DATA
           | ALT5 | GPIO5_IO28
CSI0_DAT11 | ALT3 | UART1_RX_DATA
           | ALT5 | GPIO5_IO29

# Sabre Lite uses these on the connector for UART1
SD3_DAT7 | ALT1 | UART1_TX_DATA
         | ALT5 | GPIO6_IO17
SD3_DAT6 | ALT1 | UART1_RX_DATA
         | ALT5 | GPIO6_IO18
```

## QEMU Serial Port

In the generated `simulate` script, replace the first serial config:

```bash
-serial null
```

With (for example):

```bash
-serial telnet:localhost:1235,server

# another example
-serial tcp:localhost:1235,server,nowait,nodelay
```

Now on the host machine:

```bash
# connect to serial port via telnet, data will be forwarded to IMX_UART1
telnet 127.0.0.1 1235

# or bash escapes for binary hex data
echo -ne '\x02\x04\x00\xFA'  > /dev/tcp/127.0.0.1/1235
```

## Output

Example U-boot command:

```bash
setenv bootsel4 'tftp ${loadaddr} ${serverip}:${imgname}; dcache flush; dcache off; bootelf'

run bootsel4
```

```bash
ELF-loader started on CPU: ARM Ltd. Cortex-A9 r0p0
  paddr=[20000000..201abfff]
ELF-loading image 'kernel'
  paddr=[10000000..10032fff]
  vaddr=[e0000000..e0032fff]
  virt_entry=e0000000
ELF-loading image 'hobd-system'
  paddr=[10033000..101a9fff]
  vaddr=[10000..186fff]
  virt_entry=3ced4
Bringing up 3 other cpus
Enabling MMU and paging
Jumping to kernel-image entry point...

Bootstrapping kernel
Booting all finished, dropped to user space

Warning: using printf before serial is set up. This only works as your
printf is backed by seL4_Debug_PutChar()
init debug_print_bootinfo@bootinfo.c:25 Node 0 of 1
init debug_print_bootinfo@bootinfo.c:26 IOPT levels:     0
init debug_print_bootinfo@bootinfo.c:27 IPC buffer:      0x187000
init debug_print_bootinfo@bootinfo.c:28 Empty slots:     [547 --> 4096)
init debug_print_bootinfo@bootinfo.c:29 sharedFrames:    [0 --> 0)
init debug_print_bootinfo@bootinfo.c:30 userImageFrames: [14 --> 389)
init debug_print_bootinfo@bootinfo.c:31 userImagePaging: [12 --> 14)
init debug_print_bootinfo@bootinfo.c:32 untypeds:        [389 --> 547)
init debug_print_bootinfo@bootinfo.c:33 Initial thread domain: 0

init debug_print_bootinfo@bootinfo.c:34 Initial thread cnode size: 12
init debug_print_bootinfo@bootinfo.c:35 List of untypeds
init debug_print_bootinfo@bootinfo.c:36 ------------------
init debug_print_bootinfo@bootinfo.c:37 Paddr    | Size   | Device
init debug_print_bootinfo@bootinfo.c:44 0x10000000 | 16 | 0
init debug_print_bootinfo@bootinfo.c:44 0x101aa000 | 13 | 0
init debug_print_bootinfo@bootinfo.c:44 0x101ac000 | 14 | 0
...
2 untypeds of size 27
------------------------------

platform_init@platform.c:96 Platform is initialized
root_task_init@root_task.c:105 Created global fault ep 0x29F
root_task_init@root_task.c:107 Root task is initialized
thread_create@thread.c:149 Created thread 'time-server' - stack size 4096 bytes
thread_create@thread.c:149 Created thread 'mmc' - stack size 4096 bytes
thread_create@thread.c:149 Created thread 'console' - stack size 8192 bytes
thread_create@thread.c:149 Created thread 'hobd-comm' - stack size 8192 bytes
thread_create@thread.c:149 Created thread 'sys' - stack size 4096 bytes
debug_dump_scheduler@main.c:54 Dumping scheduler (only core 0 TCBs will be displayed)

Dumping all tcbs!
Name                                            State           IP          Prio    Core
--------------------------------------------------------------------------------------
sys                                             running         0x52e20     255     0
console                                         running         0x52df8     255     0
idle_thread                                     idle            (nil)       0       0
init                                            running         0x1fa88     255     0

(press enter to get the console prompt)

IRin > help
--- hobd console ---
commands:
help       - print this help message
version    - print version information
clear      - clear the screen
time       - get the current time
stats      - print module statistics and metrics
mmc-size   - get current MMC file size
mmc-rm     - delete the current MMC file

IRin >
```

Running the [fake-hobd-ecu-data](testing-tools/fake-hobd-ecu-data/README.md) tool:

```bash
$ fake-hobd-ecu-data

->STATE_WAIT4_WAKEUP
rx_msg[FE:04:FF]
->STATE_WAIT4_DIAG_INIT
rx_msg[72:05:00]
->STATE_ACTIVE_LISTEN
tx_msg[02:04:00]
rx_msg[72:08:72]
tx_msg[02:86:72]
rx_msg[72:08:72]
tx_msg[02:0C:72]
rx_msg[72:08:72]
tx_msg[02:0C:72]
...
```

## Console

Press enter to get the console prompt:

```bash
IRin > help
```

```bash
--- console ---
commands:
help           - print this help message
version        - print version information
clear          - clear the screen
time           - get the current time
info           - print module info, statistics and metrics
debug-sched    - print the kernel scheduler information
mmc <subcmd>   - MMC module and file ops
    on         - enable MMC module file logging
    off        - disable MMC module file logging
    status     - get MMC module status
    size       - get the curent MMC file size
    rm         - delete the current MMC file
hobd <subcmd>  - HOBD and subsystems
    on         - enable HOBD K-line comms
    off        - disable HOBD K-line comms
    status     - get HOBD comms module status
    passive    - toggle HOBD comms listen-only mode
```

## Diagrams

### Wiring

![wiring diagram](images/wiring_diagram.png)
