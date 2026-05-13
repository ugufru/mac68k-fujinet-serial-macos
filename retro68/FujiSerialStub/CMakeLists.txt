# FujiSerialStub - Retro68 port of the .Fuji name-stub DRVR.
#
# Build pattern follows Retro68's Samples/WDEF: produce a flat binary with
# --mac-flat, then wrap it via Rez into a 'DRVR' resource fork.

add_executable(FujiStub FujiSerialStub.s)

set_target_properties(FujiStub PROPERTIES
    LINKER_LANGUAGE C
    LINK_FLAGS "-Wl,--mac-flat -Wl,-eFujiStub -Wl,-gc-sections -nostartfiles -nostdlib"
)

if(REZ_INCLUDE_PATH)
    set(REZ_INCLUDE_FLAG -I${REZ_INCLUDE_PATH})
endif()

add_custom_command(
    OUTPUT FujiSerialStub.rsrc.bin
    COMMAND ${REZ} ${REZ_INCLUDE_FLAG}
                   ${CMAKE_CURRENT_SOURCE_DIR}/FujiSerialStub.r
                   -o FujiSerialStub.rsrc.bin
    DEPENDS FujiStub FujiSerialStub.r
)

add_custom_target(FujiSerialStub_rsrc ALL
    DEPENDS FujiSerialStub.rsrc.bin
)
