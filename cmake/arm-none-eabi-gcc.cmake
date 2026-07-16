# Toolchain file for bare-metal STM32H563ZI builds using the arm-none-eabi-gcc
# bundled with STM32CubeIDE (no separate toolchain install required).
#
# The default path below embeds STM32CubeIDE's version-hashed plugin directory
# and will break on a CubeIDE update. Override it without editing this file:
#   cmake --preset default -DARM_TOOLCHAIN_BIN=C:/path/to/new/tools/bin

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(ARM_TOOLCHAIN_BIN
    "C:/ST/STM32CubeIDE_2.1.1/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.win32_1.0.100.202602081740/tools/bin"
    CACHE PATH "Directory containing arm-none-eabi-gcc and friends")

set(CMAKE_C_COMPILER   "${ARM_TOOLCHAIN_BIN}/arm-none-eabi-gcc.exe")
set(CMAKE_ASM_COMPILER "${ARM_TOOLCHAIN_BIN}/arm-none-eabi-gcc.exe")
set(CMAKE_OBJCOPY      "${ARM_TOOLCHAIN_BIN}/arm-none-eabi-objcopy.exe" CACHE FILEPATH "")
set(CMAKE_SIZE         "${ARM_TOOLCHAIN_BIN}/arm-none-eabi-size.exe" CACHE FILEPATH "")

# A hosted "int main(){}" test executable can't link for a bare-metal target
# without our own linker script and startup file, so make CMake's compiler
# sanity check build a static library instead of a full executable.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
