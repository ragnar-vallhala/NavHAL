# x86-64 clock + timebase demo. Freestanding ELF64 booted in QEMU via a GRUB
# multiboot2 ISO (tools/qemu/run.sh); like the hello sample it does not flash a
# board, so it omits samples/flash.cmake / FLASHER.

add_executable(${SAMPLE} main.c)
set_target_properties(${SAMPLE} PROPERTIES SUFFIX ".elf")

target_link_libraries(${SAMPLE} PRIVATE
  -Wl,--start-group
    -Wl,--whole-archive hal -Wl,--no-whole-archive
    -lgcc
  -Wl,--end-group
)

add_custom_target(run
  COMMAND ${CMAKE_SOURCE_DIR}/tools/qemu/run.sh $<TARGET_FILE:${SAMPLE}>
  DEPENDS ${SAMPLE}
  USES_TERMINAL
  COMMENT "Booting ${SAMPLE} in QEMU (quit: Ctrl-A then X)")

add_custom_command(TARGET ${SAMPLE} POST_BUILD
  COMMAND ${CMAKE_BINARY_SIZE} $<TARGET_FILE:${SAMPLE}>
  COMMENT "Calculating size of ELF")
