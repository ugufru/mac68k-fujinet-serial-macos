| Fake destination .Fuji DRVR for testing the FujiSerialStub dispatch.
|
| Looks like a real DRVR (header + 5 entry routines), but each entry just
| writes a unique value to `lastRoutine` and returns noErr. Used as the
| target the stub trampolines into via its drvrHndl placeholder.

        .global fakeDriver
        .global lastRoutine

        .text
        .even
fakeDriver:
        .word   0                       | drvrFlags
        .word   0                       | drvrDelay
        .word   0                       | drvrEMask
        .word   0                       | drvrMenu
        .word   fakeOpen   - fakeDriver | drvrOpen
        .word   fakePrime  - fakeDriver | drvrPrime
        .word   fakeCtl    - fakeDriver | drvrCtl
        .word   fakeStatus - fakeDriver | drvrStatus
        .word   fakeClose  - fakeDriver | drvrClose
        .byte   5
        .ascii  ".Fuji"
        .even

fakeOpen:
        move.b  #1, lastRoutine
        moveq   #0, %d0
        rts

fakePrime:
        move.b  #2, lastRoutine
        moveq   #0, %d0
        rts

fakeCtl:
        move.b  #3, lastRoutine
        moveq   #0, %d0
        rts

fakeStatus:
        move.b  #4, lastRoutine
        moveq   #0, %d0
        rts

fakeClose:
        move.b  #5, lastRoutine
        moveq   #0, %d0
        rts

        .data
        .even
lastRoutine:
        .byte   0
