# NavHAL arch fragment — x86-64 (bare-metal PC, long mode).
#
# Sourced by the root CMakeLists.txt via `include(cmake/arch/${ARCH_ISA}.cmake)`.
# Freestanding 64-bit kernel: no hosted libc, no PIC, no red zone. SSE is left
# ENABLED in codegen (the SysV float ABI needs xmm); the startup stub turns the
# SSE unit on in long mode before calling C.

set(ARCH_C_FLAGS
    "-m64 -ffreestanding -fno-pic -fno-pie -mno-red-zone -mcmodel=large -fno-stack-protector -fno-asynchronous-unwind-tables")
set(ARCH_ASM_FLAGS  "-m64")
set(ARCH_LINK_FLAGS "-T ${SRC_BOARD}/linker.ld -nostdlib -no-pie -z max-page-size=0x1000")

# Host-run / on-target test build is not wired for x86 yet; leave the slots the
# if(TEST) block reads empty so a non-TEST configure is unaffected.
set(NAVHAL_TEST_LINKER_FLAGS "")
set(NAVHAL_TEST_EXTRA_FLAGS  "")
set(NAVHAL_TEST_NEEDS_LIBGCC FALSE)
