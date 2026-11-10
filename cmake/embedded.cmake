# ===========================================================================
#  cmake/embedded.cmake —— 构建产物与烧录辅助
#
#  由 CMakeLists.txt 末尾 include，负责两件事：
#    1) 链接完成后自动生成 .hex / .bin，并打印体积
#    2) 提供 flash / erase / reset_target 三个 CMake 目标（走 OpenOCD）
#
#  与 stm32_clion 工程保持一致的配置方式：
#    - openocd 自动查找（C:/openocd-…/bin、$ENV{OPENOCD_HOME}/bin、
#      $ENV{PROGRAMFILES}/OpenOCD/bin、PATH）
#    - 找不到时只告警、不中断配置，可用 -DOPENOCD_EXE=... 指定
# ===========================================================================

set(_elf_path "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>")
set(_out_dir  "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>")

# ---------------------------------------------------------------------------
# 1. .elf  →  .hex  (Intel HEX，ST-LINK Utility / STM32CubeProgrammer 可直接烧)
#          →  .bin  (纯二进制，IAP / 串口烧录常用)
# ---------------------------------------------------------------------------
if(NOT CMAKE_OBJCOPY)
    message(WARNING "没有找到 arm-none-eabi-objcopy，将不生成 .hex/.bin")
else()
    set(_post_build
        COMMAND "${CMAKE_OBJCOPY}" -O ihex   "${_elf_path}" "${_out_dir}/${CMAKE_PROJECT_NAME}.hex"
        COMMAND "${CMAKE_OBJCOPY}" -O binary "${_elf_path}" "${_out_dir}/${CMAKE_PROJECT_NAME}.bin"
    )
    if(CMAKE_SIZE)
        list(APPEND _post_build COMMAND "${CMAKE_SIZE}" "${_elf_path}")
    endif()

    add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        ${_post_build}
        COMMENT "生成 ${CMAKE_PROJECT_NAME}.hex / ${CMAKE_PROJECT_NAME}.bin"
        VERBATIM
    )
endif()

# ---------------------------------------------------------------------------
# 2. OpenOCD：烧录 / 擦除 / 复位
#    路径可用 -DOPENOCD_EXE=... 覆盖，留空则自动查找常见位置。
# ---------------------------------------------------------------------------
# 自愈：早期版本可能往缓存里写过空的 OPENOCD_EXE，而 find_program 见到
# 变量“已有值”（哪怕是空串）就会跳过搜索。先清掉这种空值再查找。
if(DEFINED OPENOCD_EXE AND NOT OPENOCD_EXE)
    unset(OPENOCD_EXE CACHE)
endif()

# 注意：这里【不能】先写 set(OPENOCD_EXE "" CACHE FILEPATH ...)。
# find_program 只要发现变量已经有值（哪怕是空串）就会跳过搜索，
# 那样自动查找会永远失败。让 find_program 自己建缓存项即可：
# 用户用 -DOPENOCD_EXE=... 指定时变量已存在，搜索自动跳过。
find_program(OPENOCD_EXE
    NAMES openocd openocd.exe
    HINTS
        "C:/openocd-v0.12.0-i686-w64-mingw32/bin"
        "$ENV{OPENOCD_HOME}/bin"
        "$ENV{PROGRAMFILES}/OpenOCD/bin"
    DOC "openocd 可执行文件（自动查找；可用 -DOPENOCD_EXE=... 覆盖）"
)

if(NOT OPENOCD_EXE)
    message(WARNING
        "未找到 openocd，flash / erase / reset_target 目标不会被创建。\n"
        "指定路径后重新配置即可：\n"
        "  -DOPENOCD_EXE=C:/openocd-v0.12.0-i686-w64-mingw32/bin/openocd.exe")
    return()
endif()

message(STATUS "OpenOCD  : ${OPENOCD_EXE}")
set(_ocd_cfg "${CMAKE_SOURCE_DIR}/cmake/openocd.cfg")

# 烧录 + 校验 + 复位运行 —— 日常最常用的一个
add_custom_target(flash
    COMMAND "${OPENOCD_EXE}" -f "${_ocd_cfg}"
            -c "program \"${_elf_path}\" verify reset exit"
    DEPENDS ${CMAKE_PROJECT_NAME}
    COMMENT "OpenOCD：烧录 + 校验 + 复位运行"
    VERBATIM
)

# 全片擦除（芯片被读保护、或要彻底清空时用）
add_custom_target(erase
    COMMAND "${OPENOCD_EXE}" -f "${_ocd_cfg}"
            -c "init; reset halt; flash erase_sector 0 0 last; reset run; shutdown"
    COMMENT "OpenOCD：全片擦除"
    VERBATIM
)

# 只复位运行，不重新烧录
add_custom_target(reset_target
    COMMAND "${OPENOCD_EXE}" -f "${_ocd_cfg}" -c "init; reset run; shutdown"
    COMMENT "OpenOCD：复位并运行"
    VERBATIM
)