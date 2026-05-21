#include "Types.r"

/* Wraps the flat-binary FujiAsync into a 'DRVR' resource.
 *
 * Resource ID -15904 matches the THINK C project (.Fuji main driver).
 * sysHeap+locked+preload mirrors the conventional in-memory-driver
 * pattern used by FujiSerialStub. */

data 'DRVR' (-15904, ".Fuji", sysHeap, locked, preload) {
    $$Read("FujiAsync")
};
