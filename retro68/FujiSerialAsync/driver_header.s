| FujiSerialAsync - DRVR header + dispatch trampolines.
|
| Ported from the inline asm dispatcher in FujiSerial/FujiSerialAsync.c
| (the body of `void main()`). DOpen/DPrime/DControl/DStatus/DClose each
| call the matching C routine (doOpen, doPrime, ...) defined in
| FujiSerialAsync.c, then run the IOReturn fixup logic that the Device
| Manager expects (immediate vs queued, JIODone dispatch on completion).
|
| Layout follows retro68/FujiSerialStub/FujiSerialStub.s: DRVR header
| in .rsrcheader so it lands at offset 0 of the flat binary, ahead of
| Retro68's 14-byte entry trampoline in .text. Dispatch offsets are
| linker-computed (`DOpen - FujiAsync`), so we don't have to know what
| the trampoline size is.

        .global FujiAsync
        .extern doOpen
        .extern doPrime
        .extern doControl
        .extern doStatus
        .extern doClose

        | dWritEnableMask|dReadEnableMask|dStatEnableMask|dCtlEnableMask|dNeedLockMask
        .equ DFlags,    0x4F00
        .equ JIODone,   0x08FC          | jIODone vector in low memory
        .equ noQueueBit, 9              | bit 9 of ioTrap.w
        .equ killCode,   1              | KillIO csCode
        .equ ioTrap_off, 6              | offset of ioTrap in IOParam/CntrlParam
        .equ csCode_off, 26             | offset of csCode in CntrlParam

        .section .rsrcheader, "ax"
FujiAsync:
        .word   DFlags                  | drvrFlags
        .word   60                      | drvrDelay (periodic ticks)
        .word   0                       | drvrEMask
        .word   0                       | drvrMenu
        .word   DOpen    - FujiAsync    | drvrOpen
        .word   DPrime   - FujiAsync    | drvrPrime
        .word   DControl - FujiAsync    | drvrCtl
        .word   DStatus  - FujiAsync    | drvrStatus
        .word   DClose   - FujiAsync    | drvrClose
        .byte   5                       | Pascal string length
        .ascii  ".Fuji"                 | driver name
        .even

        .text

| Device Manager calls each entry with a0=ParmBlkPtr, a1=DCtlPtr.
| C signature: OSErr doXxx(IOParam *pb, DCtlEntry *dce);
| GCC m68k ABI passes args on the stack: sp+4=arg1, sp+8=arg2.

DOpen:
        movem.l %a0-%a1, -(%sp)         | save across C call
        movem.l %a0-%a1, -(%sp)         | push as C args
        bsr.w   doOpen
        addq.l  #8, %sp
        movem.l (%sp)+, %a0-%a1
        rts                             | open is always immediate

DPrime:
        movem.l %a0-%a1, -(%sp)
        movem.l %a0-%a1, -(%sp)
        bsr.w   doPrime
        addq.l  #8, %sp
        movem.l (%sp)+, %a0-%a1
        bra.w   IOReturn

DControl:
        movem.l %a0-%a1, -(%sp)
        movem.l %a0-%a1, -(%sp)
        bsr.w   doControl
        addq.l  #8, %sp
        movem.l (%sp)+, %a0-%a1
        cmpi.w  #killCode, csCode_off(%a0)
        bne.w   IOReturn
        rts                             | KillIO must always RTS

DStatus:
        movem.l %a0-%a1, -(%sp)
        movem.l %a0-%a1, -(%sp)
        bsr.w   doStatus
        addq.l  #8, %sp
        movem.l (%sp)+, %a0-%a1
        bra.w   IOReturn

DClose:
        movem.l %a0-%a1, -(%sp)
        movem.l %a0-%a1, -(%sp)
        bsr.w   doClose
        addq.l  #8, %sp
        movem.l (%sp)+, %a0-%a1
        rts                             | close is always immediate

IOReturn:
        move.w  ioTrap_off(%a0), %d1
        btst    #noQueueBit, %d1
        beq.s   Queued                  | not-set => queued call

NotQueued:
        tst.w   %d0
        ble.s   ImmedRTS
        clr.w   %d0                     | "in progress" not passed back
ImmedRTS:
        move.w  %d0, 16(%a0)            | IOParam.ioResult
        rts

Queued:
        tst.w   %d0
        ble.s   MyIODone
        clr.w   %d0
        rts                             | still in progress, no IODone

MyIODone:
        move.l  JIODone, -(%sp)         | push jIODone vector
        rts                             | rts jumps to IODone
