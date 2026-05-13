| FujiSerialStub - .Fuji name-stub driver
|
| Ported from FujiSerial/FujiSerialStub.c (THINK C inline asm) for the
| Retro68 GNU as toolchain. The original was a `void main()` whose entire
| body was an asm block; nothing in this file is real C, so it is now a
| standalone assembly source.
|
| Loaded multiple times (once each for .AOut/.AIn/.BOut/.BIn). The driver
| header's name field (".Fuji") names the dispatch target; each instance
| reads its drvrHndl placeholder, follows the handle to the real .Fuji
| DRVR header, and tail-calls the appropriate Open/Prime/Ctl/Status/Close
| routine.
|
| Layout note (Retro68 specifics): the flat linker script always emits a
| 14-byte entry trampoline at the start of .text. Putting the DRVR header
| in .rsrcheader places it before that trampoline, so the resource starts
| with our header at offset 0. The dispatch code lives in .text after the
| trampoline; the DRVR header's drvrOpen/etc. offsets are computed by the
| linker as `DOpen - FujiStub`, so they correctly skip over the trampoline.

        .global FujiStub
        .global FujiStub_dispatch

        | dWritEnableMask | dReadEnableMask | dStatEnableMask | dCtlEnableMask
        | from <Devices.h> = 0x0100 | 0x0200 | 0x0400 | 0x0800
        .equ DFlags, 0x0F00

        .section .rsrcheader, "ax"
FujiStub:
        | DRVR header (Inside Macintosh: Devices, p I-25)
        .word   DFlags                  | drvrFlags
        .word   0                       | drvrDelay
        .word   0                       | drvrEMask
        .word   0                       | drvrMenu
        .word   DOpen    - FujiStub     | drvrOpen
        .word   DPrime   - FujiStub     | drvrPrime
        .word   DControl - FujiStub     | drvrCtl
        .word   DStatus  - FujiStub     | drvrStatus
        .word   DClose   - FujiStub     | drvrClose
        .byte   5                       | Pascal string length
        .ascii  ".Fuji"                 | driver name
        .even

        .text
DOpen:    bsr.s   Dispatch
DPrime:   bsr.s   Dispatch
DControl: bsr.s   Dispatch
DStatus:  bsr.s   Dispatch
DClose:   bsr.s   Dispatch

drvrHndl:
        .long   0x01234567              | placeholder; patched at install time

Dispatch:
        move.l  (%sp)+, %d0             | pop return addr (=DPrime,DControl,...,drvrHndl)
        move.l  %a2, -(%sp)             | save a2
        lea     DPrime, %a2
        sub.l   %a2, %d0                | d0 = 0,2,4,6,8 for Open,Prime,Ctl,Status,Close
        movea.l drvrHndl, %a2           | follow handle
        movea.l (%a2), %a2              | -> ptr to real .Fuji DRVR header
        move.w  8(%a2,%d0.l), %d0       | DRVR header drvrOpen is at offset 8
        add.l   %a2, %d0                | absolute routine address
        move.l  (%sp)+, %a2             | restore a2
        move.l  %d0, -(%sp)             | push routine address as new return addr
        rts                             | "return" jumps into the dispatched routine
