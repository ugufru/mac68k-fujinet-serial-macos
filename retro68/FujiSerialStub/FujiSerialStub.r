#include "Types.r"

/*
 * Wraps the flat-binary FujiStub into a 'DRVR' resource.
 *
 * Resource ID -15903 matches the THINK C project.
 * Attributes follow the conventional pattern for in-memory drivers:
 *   sysHeap   (lives in system heap so DAs/drivers see it)
 *   locked    (won't be relocated while running)
 *   preload   (loaded immediately when its file is opened)
 *
 * The original THINK C project specified attribute byte 40 (0x28 = locked +
 * preload). We additionally set sysHeap because that's what the comment in
 * FujiSerialStub.c said the project setting was meant to express.
 */

data 'DRVR' (-15903, ".FujiStub", sysHeap, locked, preload) {
    $$Read("FujiStub")
};
