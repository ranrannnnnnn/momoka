set(CMAKE_SYSTEM_NAME               Generic)
set(CMAKE_SYSTEM_PROCESSOR          arm)

set(CMAKE_C_COMPILER_ID GNU)
set(CMAKE_CXX_COMPILER_ID GNU)

# ---------------------------------------------------------------------------
# 工具链定位（相对 CubeMX 原生模板的唯一改动）
#   STM32CubeMX 原生模板假设 arm-none-eabi- 已经在 PATH 里，CLion 直接
#   载入工程时经常因为没有 PATH 而报：
#     "The C compiler 'arm-none-eabi-gcc' is not a full path and was not
#      found in the PATH"
#   这里按优先级自动定位，避免每次手填 CMake options：
#     1) -DTOOLCHAIN_BIN_DIR=... / 环境变量 ARMGCC_DIR（显式指定优先）
#     2) PATH 中的 arm-none-eabi-gcc
#     3) 常见安装位置（Arm GNU Toolchain / GNU Arm Embedded Toolchain）
# ---------------------------------------------------------------------------
set(TOOLCHAIN_BIN_DIR "" CACHE PATH "arm-none-eabi-* 所在目录（留空则自动探测）")

if(CMAKE_HOST_WIN32)
    set(_arm_exe ".exe")
else()
    set(_arm_exe "")
endif()

# 1) 环境变量 ARMGCC_DIR：允许指到安装根目录或 bin 目录
if(NOT TOOLCHAIN_BIN_DIR AND DEFINED ENV{ARMGCC_DIR})
    set(_arm_env "$ENV{ARMGCC_DIR}")
    if(EXISTS "${_arm_env}/arm-none-eabi-gcc${_arm_exe}")
        set(TOOLCHAIN_BIN_DIR "${_arm_env}")
    elseif(EXISTS "${_arm_env}/bin/arm-none-eabi-gcc${_arm_exe}")
        set(TOOLCHAIN_BIN_DIR "${_arm_env}/bin")
    endif()
endif()

# 2) PATH
if(NOT TOOLCHAIN_BIN_DIR)
    find_program(_ARM_GCC_EXECUTABLE NAMES arm-none-eabi-gcc arm-none-eabi-gcc.exe)
    if(_ARM_GCC_EXECUTABLE)
        get_filename_component(TOOLCHAIN_BIN_DIR "${_ARM_GCC_EXECUTABLE}" DIRECTORY)
    endif()
endif()

# 3) 常见安装位置
if(NOT TOOLCHAIN_BIN_DIR)
    file(GLOB _arm_bin_candidates
        "C:/Program Files/Arm/GNU Toolchain*/bin"
        "C:/Program Files (x86)/Arm/GNU Toolchain*/bin"
        "C:/Program Files (x86)/GNU Arm Embedded Toolchain/*/bin"
        "C:/Program Files/GNU Arm Embedded Toolchain/*/bin"
        "C:/Program Files (x86)/GNU Tools ARM Embedded/*/bin"
        "C:/Program Files/GNU Tools ARM Embedded/*/bin"
        "$ENV{LOCALAPPDATA}/Programs/Arm/GNU Toolchain*/bin"
        "/opt/arm-gnu-toolchain*/bin"
        "/opt/gcc-arm-none-eabi*/bin"
    )
    foreach(_dir ${_arm_bin_candidates})
        if(EXISTS "${_dir}/arm-none-eabi-gcc${_arm_exe}")
            set(TOOLCHAIN_BIN_DIR "${_dir}")
            break()
        endif()
    endforeach()
endif()

if(NOT TOOLCHAIN_BIN_DIR)
    message(FATAL_ERROR
        "未找到 arm-none-eabi 工具链。请三选一：\n"
        "  1) 把 .../Arm GNU Toolchain .../bin 加入 PATH；\n"
        "  2) 配置时传 -DTOOLCHAIN_BIN_DIR=\"C:/Program Files/Arm/GNU Toolchain mingw-w64-x86_64-arm-none-eabi/bin\"；\n"
        "  3) 设置环境变量 ARMGCC_DIR 指向安装目录。")
endif()

message(STATUS "ARM toolchain bin: ${TOOLCHAIN_BIN_DIR}")

# 这里的 TOOLCHAIN_PREFIX 是“目录 + 前缀”，即工具链的完整前缀
set(TOOLCHAIN_PREFIX                "${TOOLCHAIN_BIN_DIR}/arm-none-eabi-")

# Some default GCC settings
set(CMAKE_C_COMPILER                "${TOOLCHAIN_PREFIX}gcc${_arm_exe}")
set(CMAKE_ASM_COMPILER              "${CMAKE_C_COMPILER}")
set(CMAKE_CXX_COMPILER              "${TOOLCHAIN_PREFIX}g++${_arm_exe}")
set(CMAKE_LINKER                    "${TOOLCHAIN_PREFIX}g++${_arm_exe}")
set(CMAKE_OBJCOPY                   "${TOOLCHAIN_PREFIX}objcopy${_arm_exe}")
set(CMAKE_SIZE                      "${TOOLCHAIN_PREFIX}size${_arm_exe}")

set(CMAKE_EXECUTABLE_SUFFIX_ASM     ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_C       ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_CXX     ".elf")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# MCU specific flags
set(TARGET_FLAGS "-mcpu=cortex-m3 ")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${TARGET_FLAGS}")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -x assembler-with-cpp -MMD -MP")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fdata-sections -ffunction-sections -fstack-usage")

# The cyclomatic-complexity parameter must be defined for the Cyclomatic complexity feature in STM32CubeIDE to work.
# However, most GCC toolchains do not support this option, which causes a compilation error; for this reason, the feature is disabled by default.
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcyclomatic-complexity")

set(CMAKE_C_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_C_FLAGS_RELEASE "-Os -g0")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_CXX_FLAGS_RELEASE "-Os -g0")

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fno-rtti -fno-exceptions -fno-threadsafe-statics")

set(CMAKE_EXE_LINKER_FLAGS "${TARGET_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T \"${CMAKE_SOURCE_DIR}/STM32F103xx_FLASH.ld\"")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --specs=nano.specs")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Map=${CMAKE_PROJECT_NAME}.map -Wl,--gc-sections")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--print-memory-usage")
set(TOOLCHAIN_LINK_LIBRARIES "m")