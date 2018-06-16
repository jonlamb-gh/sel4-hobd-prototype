# sel4-hobd-prototype
Prototype HOBD system running on seL4

## TODO

- Add cmake configs for simulation features
- Remove hard-coded debug configs

## Links

- [IMX6 RM](http://cache.freescale.com/files/32bit/doc/ref_manual/IMX6DQRM.pdf)

## Building

```bash
mkdir build/

cd build/

../scripts/build
```

## SMP / Multicore

By default, only a single node/core is used, because the simulate script logic
does automatically add smp options yet.

CMake config `KernelMaxNumNodes` is in `configs/imx6_sabre_lite.cmake`.

Add this to the generated `simulate` script if
simulating (note that it must be >= `KernelMaxNumNodes`):

```base
-smp cores=4
```

## Output

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

platform_init@platform.c:89 Platform is initialized
root_task_init@root_task.c:105 Created global fault ep 0x27B
root_task_init@root_task.c:107 Root task is initialized
thread_create@thread.c:69 Minting fault ep 0x2B4 for thread 'hobd'
thread_create@thread.c:124 Created thread 'hobd' - stack size 4096 bytes
hobd_module_init@hobd_module.c:122 hobd is initialized
thread_create@thread.c:69 Minting fault ep 0x2E5 for thread 'sys'
thread_create@thread.c:124 Created thread 'sys' - stack size 4096 bytes
system_module_init@system_module.c:86 sys is initialized
debug_dump_scheduler@main.c:47 Dumping scheduler (only core 0 TCBs will be displayed)

Dumping all tcbs!
Name                                            State           IP                       Prio    Core
--------------------------------------------------------------------------------------
sys                                             restart         0x11f30 255                     0
hobd                                            restart         0x103e8 255                     0
idle_thread                                     idle            (nil)   0                       0
init                                            running         0x16214 255                     0

thread_fn@system_module.c:44 System ready to start
signal_ready_to_start@system_module.c:31 System starting
thread_fn@hobd_module.c:37 thread is running, about to intentionally fault
main@main.c:82 Received fault on ep 0x27B - badge 0x21
Pagefault from [fault-handler]: write fault at PC: 0x10454 vaddr: 0xdeadbeef, FSR 0x805
debug_dump_scheduler@main.c:47 Dumping scheduler (only core 0 TCBs will be displayed)

Dumping all tcbs!
Name                                            State           IP                       Prio    Core
--------------------------------------------------------------------------------------
sys                                             running         0x10c10 255                     0
hobd                                            blocked on reply        0x10454 255             0
idle_thread                                     idle            (nil)   0                       0
init                                            running         0x16214 255                     0
```
