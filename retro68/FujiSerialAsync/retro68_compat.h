#pragma once

/* Constants used by FujiSerialAsync that Retro68's Multiversal Interfaces
 * do not define. Values match Inside Macintosh: Devices and the THINK C
 * MacHeaders.
 */

#ifndef dWritEnableMask
#define dWritEnableMask  0x0100
#endif
#ifndef dReadEnableMask
#define dReadEnableMask  0x0200
#endif
#ifndef dStatEnableMask
#define dStatEnableMask  0x0400
#endif
#ifndef dCtlEnableMask
#define dCtlEnableMask   0x0800
#endif
#ifndef dNeedLockMask
#define dNeedLockMask    0x4000
#endif

#ifndef portNotCf
#define portNotCf        (-28)
#endif
#ifndef ioInProgress
#define ioInProgress     1
#endif
