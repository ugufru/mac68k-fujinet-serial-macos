| VBL task and I/O completion glue for FujiSerialAsync.
|
| Ported from the `_vblRoutines()` inline asm block in
| FujiSerial/FujiSerialAsync.c. The THINK C original used
| `extern label:` inside an asm block to expose labels as C-callable
| symbols; under Retro68 each symbol is declared .globl in this
| standalone .s file.
|
| Calls into C: emptyWriteBufDone, fillReadBufDone, fujiVBLTask.
| Called from C: fujiStartVBL, getVBLTask, schedVBLTask, getMainDCE,
|                complFlushOut, complReadIn, ioIsComplete,
|                takeVblMutex, releaseVblMutex.
|
| dcePtr, vblTask, mutexFlags live in this resource as writable static
| storage (the driver runs from the system heap and stays locked).

        .equ JIODone,  0x08FC
        .equ vType,    1                | VBL queue element type
        .equ VBL_TICKS, 30              | matches the .c default

        .global fujiStartVBL
        .global getVBLTask
        .global schedVBLTask
        .global getMainDCE
        .global complFlushOut
        .global complReadIn
        .global ioIsComplete
        .global takeVblMutex
        .global releaseVblMutex

        .extern emptyWriteBufDone
        .extern fillReadBufDone
        .extern fujiVBLTask

        .text

| void fujiStartVBL(DCtlEntry *devCtlEnt);
fujiStartVBL:
        lea     dcePtr(%pc), %a0
        tst.l   (%a0)
        bne.s   1f                      | already installed
        move.l  4(%sp), (%a0)           | dcePtr = devCtlEnt
        lea     vblTask(%pc), %a0
        lea     callFujiVBL(%pc), %a1
        move.l  %a1, 6(%a0)             | VBLTask.vblAddr (qLink+qType = 6)
        move.w  #VBL_TICKS, 10(%a0)     | VBLTask.vblCount
        | _VInstall (A-trap 0xA033) with VBLTaskPtr in a0
        .short  0xA033
1:      rts

| VBLTask *getVBLTask(void);
getVBLTask:
        lea     vblTask(%pc), %a0
        move.l  %a0, %d0
        rts

| void schedVBLTask(void);
schedVBLTask:
        lea     vblTask(%pc), %a0
        move.w  #1, 10(%a0)             | force vblCount to 1
        rts

| DCtlEntry *getMainDCE(void);
getMainDCE:
        move.l  dcePtr(%pc), %d0
        rts

| Completion-routine entry conventions (Inside Mac: Devices):
|   a0 -> parameter block, d0 = result code; preserve a2-a7/d3-d7.

complFlushOut:
        lea     emptyWriteBufDone(%pc), %a1
        bra.s   callRoutineC

complReadIn:
        lea     fillReadBufDone(%pc), %a1
        bra.s   callRoutineC

| VBL entry: a0 -> VBLTask. Same preservation rules as completion.
callFujiVBL:
        lea     fujiVBLTask(%pc), %a1
        | fall through

| Marshal a0 as the first C argument and call the routine in a1.
callRoutineC:
        movem.l %a2-%a6/%d3-%d7, -(%sp)
        move.l  %a0, -(%sp)             | push a0 as C arg
        jsr     (%a1)
        addq.l  #4, %sp
        movem.l (%sp)+, %a2-%a6/%d3-%d7
        rts

| void ioIsComplete(DCtlEntry *dce, OSErr result);
|   On entry (after `bsr ioIsComplete` from C): sp+0=ret, sp+4=dce, sp+8=result
|   IODone wants a1=DCtlPtr (it actually wants ParmBlkPtr in a0; classic
|   driver convention is "a0 -> parm block, d0 = result, a1 = DCE"). We
|   mirror the THINK C source verbatim here.
ioIsComplete:
        move.w  10(%sp), %d0            | result (low word of long arg)
        move.l  4(%sp), %a1             | DCtlPtr
        move.l  JIODone, -(%sp)
        rts

| Boolean takeVblMutex(void);  returns 0xFF if we got the lock, else 0.
takeVblMutex:
        moveq   #0, %d0
        lea     mutexFlags(%pc), %a0
        bset    %d0, (%a0)              | Z=1 if bit was clear (lock acquired)
        seq     %d0
        rts

| void releaseVblMutex(void);
releaseVblMutex:
        moveq   #0, %d0
        lea     mutexFlags(%pc), %a0
        bclr    %d0, (%a0)
        rts

| Static storage. Lives in the resource's flat image; the driver is
| sysHeap+locked+preload, so we can write to it freely.
        .data
        .align 2
dcePtr:
        .long   0
vblTask:
        .long   0                       | qLink
        .word   vType                   | qType
        .long   0                       | vblAddr
        .word   0                       | vblCount
        .word   0                       | vblPhase
mutexFlags:
        .word   0
