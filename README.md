# sel4-hobd-prototype
Prototype HOBD system running on seL4

## TODO

- Add cmake configs for simulation features
- Remove hard-coded debugging configs

## Building

```bash
mkdir build/

cd build/

../scripts/build
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

init platform_init@platform.c:76 Platform is initialized
init root_task_init@root_task.c:108 Created global fault ep 0x277
init root_task_init@root_task.c:110 Root task is initialized
init thread_create@thread.c:69 Minting fault ep 0x2B0 for thread 'hobd-module'
init thread_create@thread.c:124 Created thread 'hobd-module' - stack size 4096 bytes
init hobd_module_init@hobd_module.c:110 hobd-module is initialized
init main@main.c:63 Dumping scheduler

Dumping all tcbs!
Name                                            State           IP                       Prio    Core
--------------------------------------------------------------------------------------
hobd-module                                     restart         0x103e8 255                     0
idle_thread                                     idle            (nil)   0                       0
init                                            running         0x14770 255                     0

hobd-module thread_fn@hobd_module.c:42 thread is running, about to intentionally fault
hobd-module main@main.c:78 Received fault on ep 0x277 - badge 0x61
Pagefault from [fault-handler]: write fault at PC: 0x10450 vaddr: 0xdeadbeef, FSR 0x805
```
