/*
 * FujiStubTest - runtime smoke test for FujiSerialStub.
 *
 * Loads the embedded DRVR(-15903, .FujiStub) resource, patches its
 * drvrHndl placeholder (0x01234567) to point at a fake destination
 * driver, then directly invokes each of the 5 dispatch slots via
 * function pointers. Isolates the dispatch asm logic - no OS
 * involvement, no device-manager plumbing.
 *
 * Each successful dispatch sets `lastRoutine` (1..5). The harness
 * reports PASS/FAIL for each entry.
 *
 * Future work (separate issue): exercise through PBControl/PBStatus/
 * PBWrite/PBClose once we have a real .Fuji driver to install (the
 * OS name-lookup path would otherwise re-load an un-patched stub copy).
 */

#include <Devices.h>
#include <Resources.h>
#include <Memory.h>
#include <stdio.h>

extern char fakeDriver[];
extern volatile char lastRoutine;

#define STUB_RES_ID  (-15903)

typedef void (*drvr_fn)(void);

static const char *routineName(char r) {
    switch (r) {
        case 1: return "Open";
        case 2: return "Prime";
        case 3: return "Control";
        case 4: return "Status";
        case 5: return "Close";
        default: return "(none)";
    }
}

/*
 * Locate the 0x01234567 drvrHndl placeholder in the loaded stub.
 * The stub's CMakeLists arranges for the DRVR header to start at
 * offset 0, so we search the whole loaded resource.
 */
static long findPlaceholder(unsigned char *data, long len) {
    long i;
    for (i = 0; i + 4 <= len; i++) {
        if (data[i] == 0x01 && data[i+1] == 0x23 &&
            data[i+2] == 0x45 && data[i+3] == 0x67) {
            return i;
        }
    }
    return -1;
}

int main(void) {
    Handle stub;
    long stubLen, placeholder;
    char *stubData;
    static char *fakePtr;
    char **fakeHandle;
    short *header;
    int i;

    printf("FujiStubTest\n");
    printf("============\n\n");

    /* Build a fake "Handle" for the destination driver. The stub treats
       drvrHndl as a Handle: derefs it once, then derefs again to reach
       the DRVR header. So we need a pointer (fakeHandle) to a pointer
       (fakePtr) to the data (fakeDriver). */
    fakePtr = fakeDriver;
    fakeHandle = &fakePtr;

    /* Load the stub DRVR from our resource fork */
    stub = GetResource('DRVR', STUB_RES_ID);
    if (!stub) {
        printf("FAIL: GetResource(DRVR, %d) returned NULL\n", STUB_RES_ID);
        return 1;
    }
    stubLen = GetHandleSize(stub);
    DetachResource(stub);
    HNoPurge(stub);
    HLock(stub);
    stubData = *stub;

    placeholder = findPlaceholder((unsigned char *)stubData, stubLen);
    if (placeholder < 0) {
        printf("FAIL: drvrHndl placeholder not found in %ld-byte stub\n", stubLen);
        return 1;
    }
    *(char ***)(stubData + placeholder) = fakeHandle;
    printf("Stub loaded: %ld bytes\n", stubLen);
    printf("drvrHndl placeholder patched at offset %ld -> fake driver\n\n",
           placeholder);

    /* Read the 5 entry-point offsets from the DRVR header */
    header = (short *)stubData;
    printf("DRVR header offsets: open=%d prime=%d ctl=%d status=%d close=%d\n\n",
           header[4], header[5], header[6], header[7], header[8]);

    /* Direct-call test: invoke each entry as a function pointer. */
    printf("--- Direct-call dispatch test ---\n");
    {
        const char *labels[5] = {"Open", "Prime", "Control", "Status", "Close"};
        int passes = 0;
        for (i = 0; i < 5; i++) {
            drvr_fn fn = (drvr_fn)(stubData + header[4 + i]);
            lastRoutine = 0;
            fn();
            int ok = (lastRoutine == (i + 1));
            printf("  Stub %s entry -> fake routine: %s   %s\n",
                   labels[i], routineName(lastRoutine), ok ? "PASS" : "FAIL");
            if (ok) passes++;
        }
        printf("\n%d / 5 dispatch slots PASS\n", passes);
    }

    printf("\nDone. (Press Return to quit)\n");
    getchar();
    return 0;
}
