/* Retro68 port of FujiSerial/FujiSerialAsync.c.
 *
 * The original is a single THINK C source whose `void main()` body is
 * an inline-asm dispatch block, with a second inline-asm block
 * (`_vblRoutines`) that exports a handful of C-callable labels and
 * pulls in #include "LedIndicators.h" (more inline asm). None of that
 * compiles under GCC.
 *
 * This file keeps the C routines verbatim and lets sibling .s files
 * (driver_header.s, vbl_asm.s) and a C fallback (led_indicators.c)
 * provide the asm-defined symbols.
 *
 * USE_AOUT_EXTRAS / USE_IPP_UDP / USE_IPP_TCP are all 0, so MacTCP.h
 * is not needed (Retro68's Multiversal Interfaces does not ship it,
 * tracked separately as issue #2).
 */

#include <stddef.h>

#include <Devices.h>
#include <Files.h>
#include <Serial.h>

#include "retro68_compat.h"
#include "FujiNet.h"
#include "FujiInterfaces.h"

/* Configuration */
#define SANITY_CHECK      1
#define VBL_TICKS         30

/* Menubar "led" indicators (see led_indicators.c). */
#define LED_IDLE       ind_hollow
#define LED_ASYNC_IO   ind_solid
#define LED_BLKED_IO   ind_dot
#define LED_WRONG_TAG  ind_ring
#define LED_ERROR      ind_cross

enum {
    ind_hollow, ind_gray, ind_solid, ind_dot, ind_ring, ind_cross
};

#define VBL_WRIT_INDICATOR(symb) drawIndicatorAt(496, 1, (symb))
#define VBL_READ_INDICATOR(symb) drawIndicatorAt(496, 9, (symb))

pascal void drawIndicatorAt(int x, int y, long which);

/* Provided by vbl_asm.s. */
extern void       fujiStartVBL(DCtlEntry *devCtlEnt);
extern VBLTask   *getVBLTask(void);
extern void       schedVBLTask(void);
extern DCtlEntry *getMainDCE(void);
extern void       complFlushOut(void);
extern void       complReadIn(void);
extern void       ioIsComplete(DCtlEntry *devCtlEnt, OSErr result);
extern Boolean    takeVblMutex(void);
extern void       releaseVblMutex(void);

/* Forward decls. */
OSErr doOpen   (IOParam    *pb, DCtlEntry *dce);
OSErr doPrime  (IOParam    *pb, DCtlEntry *dce);
OSErr doClose  (IOParam    *pb, DCtlEntry *dce);
OSErr doControl(CntrlParam *pb, DCtlEntry *dce);
OSErr doStatus (CntrlParam *pb, DCtlEntry *dce);

/* Called from vbl_asm.s — must be linker-visible. */
void emptyWriteBufDone(IOParam *pb);
void fillReadBufDone  (IOParam *pb);
void fujiVBLTask      (VBLTask *vbl);

/********** helpers **********/

static struct DriverInfo *getDriverInfo(struct FujiSerData *data, short dCtlRefNum)
{
    struct DriverInfo *info;
    for (info = data->drvrInfo; info->refNum; ++info) {
        if (info->refNum == dCtlRefNum) {
            return info;
        }
    }
    info->refNum = dCtlRefNum;
    return info;
}

static void wakeDriversAndReleaseMutex(struct FujiSerData *data)
{
    struct DriverInfo *info;

    data->inWakeUp = true;
    for (info = data->drvrInfo; info->refNum; ++info) {
        IOParam   *pb  = info->pendingPb;
        DCtlEntry *dce = info->pendingDce;

        info->pendingPb = 0;

        if (pb) {
            const OSErr err = doPrime(pb, dce);
            if (err != ioInProgress) {
                ioIsComplete(dce, err);
            }
        }
    }
    data->inWakeUp = false;
    releaseVblMutex();
}

static void fillReadBuffer(struct FujiSerData *data)
{
    data->conn.iopb.ioMisc       = (Ptr) data;
    data->conn.iopb.ioBuffer     = (Ptr) &data->readData;
    data->conn.iopb.ioCompletion = (ProcPtr) complReadIn;
    VBL_READ_INDICATOR(LED_ASYNC_IO);
    PBReadAsync((ParmBlkPtr) &data->conn.iopb);
}

void fillReadBufDone(IOParam *pb)
{
    struct FujiSerData *data = (struct FujiSerData *) pb->ioMisc;
    long indicator = LED_ERROR;

    if (pb->ioResult == noErr) {
        if (data->readData.id == MAC_FUJI_REPLY_TAG) {
            if (data->readData.avail > (short) NELEMENTS(data->readData.payload)) {
                data->readExtraAvail         = data->readData.avail - NELEMENTS(data->readData.payload);
                data->readStorage.ioReqCount = NELEMENTS(data->readData.payload);
            } else {
                data->readStorage.ioReqCount = data->readData.avail;
                data->readExtraAvail         = 0;
            }
            data->readStorage.ioActCount = 0;
            indicator = LED_IDLE;
        } else {
            indicator = LED_WRONG_TAG;
            pb->ioResult = -1;
        }
    }
    VBL_READ_INDICATOR(indicator);
    wakeDriversAndReleaseMutex(data);
}

static void emptyWriteBuffer(struct FujiSerData *data)
{
    data->conn.iopb.ioMisc       = (Ptr) data;
    data->conn.iopb.ioBuffer     = (Ptr) &data->writeData;
    data->conn.iopb.ioCompletion = (ProcPtr) complFlushOut;

    data->writeData.id       = MAC_FUJI_REQUEST_TAG;
    data->writeData.src      = 0;
    data->writeData.dst      = 0;
    data->writeData.reserved = 0;
    data->writeData.length   = data->writeStorage.ioActCount;

    VBL_WRIT_INDICATOR(LED_ASYNC_IO);
    PBWriteAsync((ParmBlkPtr) &data->conn.iopb);
}

void emptyWriteBufDone(IOParam *pb)
{
    struct FujiSerData *data = (struct FujiSerData *) pb->ioMisc;
    long wrIndicator = LED_ERROR;

    if (pb->ioResult == noErr) {
        data->writeStorage.ioActCount = 0;
        wrIndicator = LED_IDLE;

        if (data->readStorage.ioActCount == data->readStorage.ioReqCount) {
            VBL_WRIT_INDICATOR(wrIndicator);
            fillReadBuffer(data);
            return;
        }
    }

    VBL_WRIT_INDICATOR(wrIndicator);
    wakeDriversAndReleaseMutex(data);
}

void fujiVBLTask(VBLTask *vbl)
{
    const DCtlEntry    *devCtlEnt = getMainDCE();
    struct FujiSerData *data      = *(FujiSerDataHndl) devCtlEnt->dCtlStorage;

    vbl->vblCount = data->vblCount;

    if (takeVblMutex()) {
        if (data->conn.iopb.ioResult == noErr) {
            if (data->writeStorage.ioActCount > 0) {
                emptyWriteBuffer(data);
                return;
            } else if (data->readStorage.ioActCount == data->readStorage.ioReqCount) {
                fillReadBuffer(data);
                return;
            }
        }
        wakeDriversAndReleaseMutex(data);
    }
}

static void bufferCopy(struct StorageSpec *src, struct StorageSpec *dst)
{
    const long srcLeft = src->ioReqCount - src->ioActCount;
    long       dstLeft = dst->ioReqCount - dst->ioActCount;

#if SANITY_CHECK
    if (dst->ioReqCount < 0 || dst->ioActCount < 0 ||
        src->ioReqCount < 0 || src->ioActCount < 0 ||
        srcLeft < 0 || dstLeft < 0) {
        SysBeep(10);
        dstLeft = 0;
    }
#endif

    if (dstLeft > srcLeft) dstLeft = srcLeft;

#if SANITY_CHECK
    if (dstLeft > 500) {
        SysBeep(10);
        dstLeft = 0;
    }
#endif

    if (dstLeft > 0) {
        BlockMove(src->ioBuffer + src->ioActCount,
                  dst->ioBuffer + dst->ioActCount,
                  dstLeft);
    }
    src->ioActCount += dstLeft;
    dst->ioActCount += dstLeft;
}

/********** Device Manager entry points **********/

OSErr doOpen(IOParam *pb, DCtlEntry *dce)
{
    struct FujiSerData *data;
    (void) pb;

    if (dce->dCtlStorage == 0L) return openErr;

    HLock(dce->dCtlStorage);

    data = *(FujiSerDataHndl) dce->dCtlStorage;
    if (data->conn.iopb.ioRefNum == 0L) return portNotCf;

    data->conn.iopb.ioResult = noErr;

    if (data->vblCount == 0) data->vblCount = VBL_TICKS;

    data->readStorage.ioBuffer    = data->readData.payload;
    data->readStorage.ioReqCount  = 0;
    data->readStorage.ioActCount  = 0;

    data->writeStorage.ioBuffer   = data->writeData.payload;
    data->writeStorage.ioReqCount = NELEMENTS(data->writeData.payload);
    data->writeStorage.ioActCount = 0;

    fujiStartVBL(dce);
    return noErr;
}

OSErr doPrime(IOParam *pb, DCtlEntry *devCtlEnt)
{
    struct FujiSerData *data = *(FujiSerDataHndl) devCtlEnt->dCtlStorage;
    OSErr err = ioInProgress;

    if (data->inWakeUp || takeVblMutex()) {
        if (data->conn.iopb.ioResult != noErr) {
            err = data->conn.iopb.ioResult;
        } else {
            const unsigned char cmd = pb->ioTrap & 0x00FF;
            struct StorageSpec *src = 0, *dst = 0;
            if (cmd == aRdCmd) {
                src = &data->readStorage;
                dst = (struct StorageSpec *) &pb->ioBuffer;
            } else if (cmd == aWrCmd) {
                src = (struct StorageSpec *) &pb->ioBuffer;
                dst = &data->writeStorage;
            }
            if (src) bufferCopy(src, dst);

            if (pb->ioActCount == pb->ioReqCount) {
                err = noErr;
                if (cmd == aWrCmd) data->bytesWritten += pb->ioActCount;
                else               data->bytesRead    += pb->ioActCount;
            }
        }
        if (!data->inWakeUp) releaseVblMutex();
    }

    if (err == ioInProgress) {
        struct DriverInfo *info = getDriverInfo(data, pb->ioRefNum);
        info->pendingDce = devCtlEnt;
        info->pendingPb  = pb;
        schedVBLTask();
    }

    pb->ioResult = err;
    return err;
}

OSErr doControl(CntrlParam *pb, DCtlEntry *dce)
{
    (void) pb; (void) dce;
    return noErr;
}

OSErr doStatus(CntrlParam *pb, DCtlEntry *devCtlEnt)
{
    struct FujiSerData *data = *(FujiSerDataHndl) devCtlEnt->dCtlStorage;

    if (pb->csCode == 2) {
        pb->csParam[0] = 0;
        pb->csParam[1] = (data->readStorage.ioReqCount - data->readStorage.ioActCount)
                       + data->readExtraAvail;
    }
    return noErr;
}

OSErr doClose(IOParam *pb, DCtlEntry *devCtlEnt)
{
    (void) pb; (void) devCtlEnt;
    return noErr;
}
