# TODO

set(KernelARMPlatform "sabre" CACHE STRING "")
set(KernelArch "arm" CACHE STRING "")
set(KernelArmSel4Arch "aarch32" CACHE STRING "")
set(KernelIPCBufferLocation "threadID_register" CACHE STRING "")
set(KernelOptimisation "-O2" CACHE STRING "")
set(KernelBenchmarks "none" CACHE STRING "")
set(KernelDangerousCodeInjection OFF CACHE BOOL "")
set(KernelFastpath ON CACHE BOOL "")
set(ElfloaderImage "elf" CACHE STRING "")

set(KernelVerificationBuild OFF CACHE BOOL "" FORCE)
set(KernelDebugBuild ON CACHE BOOL "" FORCE)
set(KernelPrinting ON CACHE BOOL "" FORCE)
set(LibUtilsDefaultZfLogLevel "0" CACHE STRING "" FORCE)
set(LibSel4PlatSupportUseDebugPutChar true CACHE BOOL "")

set(KernelMaxNumNodes "1" CACHE STRING "")
set(KernelNumDomains 1 CACHE STRING "")
set(KernelMaxNumBootinfoUntypedCap 200 CACHE STRING "")
set(KernelRetypeFanOutLimit "256" CACHE STRING "")
set(LibSel4UtilsStackSize "32768" CACHE STRING "" FORCE)
set(LibSel4UtilsCSpaceSizeBits "17" CACHE STRING "" FORCE)
