set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker/STM32H563ZITX_FLASH.ld)

add_executable(unic_firmware
    app/main.c
    drivers/rcc.c
    drivers/gpio.c
    drivers/usart.c
    drivers/eth_mdio.c
    drivers/cmsis/cmsis-device-h5/Source/Templates/system_stm32h5xx.c
    drivers/cmsis/cmsis-device-h5/Source/Templates/gcc/startup_stm32h563xx.s
)

target_include_directories(unic_firmware PRIVATE
    drivers
    drivers/cmsis/cmsis-device-h5/Include
    drivers/cmsis/cmsis-core/Core/Include
)

target_compile_definitions(unic_firmware PRIVATE STM32H563xx)

set_target_properties(unic_firmware PROPERTIES SUFFIX ".elf")

set(MCU_FLAGS
    -mcpu=cortex-m33
    -mthumb
    -mfpu=fpv5-sp-d16
    -mfloat-abi=hard
)

target_compile_options(unic_firmware PRIVATE
    ${MCU_FLAGS}
    -ffunction-sections
    -fdata-sections
    -fno-common
    -Wall
    -Wextra
    -O0
    -g3
)

target_link_options(unic_firmware PRIVATE
    ${MCU_FLAGS}
    -T${LINKER_SCRIPT}
    -Wl,--gc-sections
    -Wl,-Map=$<TARGET_FILE_DIR:unic_firmware>/unic_firmware.map
    --specs=nano.specs
    --specs=nosys.specs
)

set_target_properties(unic_firmware PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})

add_custom_command(TARGET unic_firmware POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:unic_firmware> $<TARGET_FILE_DIR:unic_firmware>/unic_firmware.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:unic_firmware> $<TARGET_FILE_DIR:unic_firmware>/unic_firmware.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:unic_firmware>
    COMMENT "unic_firmware: generating .hex/.bin and size report"
)
