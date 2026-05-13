# FujiStubTest - runtime test harness for FujiSerialStub.
#
# Builds a Retro68 console application that includes the FujiSerialStub
# DRVR resource (built by the FujiSerialStub target) in its resource fork.

add_application(FujiStubTest
    FujiStubTest.c
    FakeDriver.s
    FujiStubTest.r
    ${CMAKE_BINARY_DIR}/FujiSerialStub/FujiSerialStub.rsrc.bin
    CONSOLE
)

add_dependencies(FujiStubTest FujiSerialStub_rsrc)

set_target_properties(FujiStubTest PROPERTIES
    COMPILE_OPTIONS -ffunction-sections
    LINK_FLAGS "-Wl,-gc-sections"
)
