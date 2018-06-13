# TODO

set(KernelARMPlatform "sabre" CACHE STRING "")
set(KernelArch "arm" CACHE STRING "")
set(KernelArmSel4Arch "aarch32" CACHE STRING "")
set(KernelVerificationBuild OFF CACHE BOOL "")
set(KernelIPCBufferLocation "threadID_register" CACHE STRING "")
set(KernelMaxNumNodes "1" CACHE STRING "")
set(KernelOptimisation "-O2" CACHE STRING "")
set(KernelRetypeFanOutLimit "256" CACHE STRING "")
set(KernelBenchmarks "none" CACHE STRING "")
set(KernelDangerousCodeInjection OFF CACHE BOOL "")
set(KernelFastpath ON CACHE BOOL "")
set(KernelPrinting ON CACHE BOOL "")
set(KernelNumDomains 1 CACHE STRING "")
set(KernelMaxNumBootinfoUntypedCap 166 CACHE STRING "")

set(LibSel4PlatSupportUseDebugPutChar true CACHE BOOL "")
