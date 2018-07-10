# sel4-hobd-prototype

Prototype HOBD system running on the [seL4](https://sel4.systems/) microkernel.

Manages a diagnostic connection to the Honda CBR ECU and logs data to an SD card.

Supported platforms:

- Sabre Lite IMx6, quad core A9

See the `devel` branch for the most recent developments.

## TODO

- Add to this doc/other-docs/diagrams/etc
- Cleanup repo, remove old milestone tags
- Add cmake configs for simulation features
- Remove hard-coded debug configs
- Why is `dcache flush; dcache off` in U-boot needed?

## Links

- [IMX6 RM](http://cache.freescale.com/files/32bit/doc/ref_manual/IMX6DQRM.pdf)
- [HW user manual](https://1quxc51443zg3oix7e35dnvg-wpengine.netdna-ssl.com/wp-content/uploads/2014/11/SABRE_Lite_Hardware_Manual_rev11.pdf)
- [HW components](https://1quxc51443zg3oix7e35dnvg-wpengine.netdna-ssl.com/wp-content/uploads/2014/11/sabre_lite-revD.pdf)
- [GPIO mapping guide](https://www.kosagi.com/w/index.php?title=Definitive_GPIO_guide)
- [Honda OBD Data tables](http://projects.gonzos.net/wp-content/uploads/2015/09/Honda-data-tables.pdf)

## Project Files
- [CMake config for this project](configs/imx6_sabre_lite.cmake)
- [Main project root directory](projects/hobd_system)
- [ECU data generator tool](testing_tools/fake_hobd_ecu_data/README.md)

## Building

TODO - `SIMULATION_BUILD`

```bash
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

Affinity for the threads used in this project are in [config.h](projects/hobd_system/include/config.h).

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
root_task_init@root_task.c:105 Created global fault ep 0x287
root_task_init@root_task.c:107 Root task is initialized
thread_create@thread.c:69 Minting fault ep 0x2A0 for thread 'time-server'
thread_create@thread.c:124 Created thread 'time-server' - stack size 4096 bytes
init_tm@time_server_module.c:99 Created timer - current time is 6464000 ns
time_server_module_init@time_server_module.c:132 time-server is initialized
thread_create@thread.c:69 Minting fault ep 0x2E3 for thread 'hobd-comm'
thread_create@thread.c:124 Created thread 'hobd-comm' - stack size 8192 bytes
hobd_module_init@hobd_module.c:319 hobd-comm is initialized
thread_create@thread.c:69 Minting fault ep 0x2DA for thread 'sys'
thread_create@thread.c:124 Created thread 'sys' - stack size 4096 bytes
system_module_init@system_module.c:86 sys is initialized
debug_dump_scheduler@main.c:48 Dumping scheduler (only core 0 TCBs will be displayed)

Dumping all tcbs!
Name                                            State           IP                       Prio    Core
--------------------------------------------------------------------------------------
sys                                             restart         0x1588c 255                     0
hobd-comm                                       restart         0x13e50 255                     0
time-server                                     restart         0x11798 255                     0
idle_thread                                     idle            (nil)   0                       0
init                                            running         0x19bfc 255                     0

thread_fn@system_module.c:44 System ready to start
signal_ready_to_start@system_module.c:31 System starting
obd_comm_thread_fn@hobd_module.c:255 hobd-comm thread is running
ecu_init_seq@hobd_module.c:57 Performing ECU GPIO initialization sequence
time_server_thread_fn@time_server_module.c:42 time-server thread is running
comm_update_state@hobd_module.c:172 ->STATE_SEND_ECU_INIT
send_ecu_diag_messages@hobd_module.c:74 Sending ECU diagnostic messages
comm_send_msg@comm.c:101 tx_msg[FE:04:FF]
comm_send_msg@comm.c:101 tx_msg[72:05:00]
comm_recv_msg@comm.c:137 rx_msg[02:04:00]
wait_for_resp@hobd_module.c:134 Response msg time is 152580000 ns
comm_update_state@hobd_module.c:185 ->STATE_SEND_REQ0
comm_send_msg@comm.c:101 tx_msg[72:08:72]
comm_recv_msg@comm.c:137 rx_msg[02:86:72]
wait_for_resp@hobd_module.c:134 Response msg time is 199790000 ns
comm_update_state@hobd_module.c:202 ->STATE_SEND_REQ1
comm_send_msg@comm.c:101 tx_msg[72:08:72]
comm_recv_msg@comm.c:137 rx_msg[02:0C:72]
wait_for_resp@hobd_module.c:134 Response msg time is 244738000 ns
...
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
