# FujiSerialAsync - Retro68 port of the real .Fuji async DRVR (issue #5).
#
# Scope: build-only. Produces FujiSerialAsync.rsrc.bin (DRVR -15904).
# Real-driver harness wiring (OpenDriver / PBControl) is a follow-up.

add_executable(FujiAsync
    driver_header.s
    vbl_asm.s
    FujiSerialAsync.c
    led_indicators.c
)

target_include_directories(FujiAsync PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/../FujiCommon
)

set_target_properties(FujiAsync PROPERTIES
    LINKER_LANGUAGE C
    LINK_FLAGS "-Wl,--mac-flat -Wl,-eFujiAsync -Wl,-gc-sections -nostartfiles -nostdlib"
)

if(REZ_INCLUDE_PATH)
    set(REZ_INCLUDE_FLAG -I${REZ_INCLUDE_PATH})
endif()

add_custom_command(
    OUTPUT FujiSerialAsync.rsrc.bin
    COMMAND ${REZ} ${REZ_INCLUDE_FLAG}
                   ${CMAKE_CURRENT_SOURCE_DIR}/FujiSerialAsync.r
                   -o FujiSerialAsync.rsrc.bin
    DEPENDS FujiAsync FujiSerialAsync.r
)

add_custom_target(FujiSerialAsync_rsrc ALL
    DEPENDS FujiSerialAsync.rsrc.bin
)
