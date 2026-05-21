/* Retro68 fallback for FujiCommon/LedIndicators.h.
 *
 * The original is 100% inline 68k asm that draws six 4-byte LED icons
 * straight into the framebuffer at (x, y). Retro68 can't host the
 * THINK C asm syntax verbatim, and going through QuickDraw from a VBL
 * task is risky, so this is a small C reimplementation that pokes the
 * screen at the same bit layout.
 *
 * Called from FujiSerialAsync.c via VBL_WRIT_INDICATOR / VBL_READ_INDICATOR.
 * x must be a multiple of 8 (one byte == one horizontal byte of pixels).
 */

#include <MacTypes.h>
#include <LowMem.h>

/* Icon glyphs match LedIndicators.h: 4 bytes each, mirrored vertically to
 * make a 7-pixel-tall LED (rows 0,1,2,3,3,2,1,0). */
static const unsigned char kIconBytes[6][4] = {
    {0x38, 0x44, 0x82, 0x82},   /* ind_hollow */
    {0x38, 0x54, 0xAA, 0xD6},   /* ind_gray   */
    {0x38, 0x7C, 0xFE, 0xFE},   /* ind_solid  */
    {0x38, 0x44, 0xBA, 0xBA},   /* ind_dot    */
    {0x38, 0x7C, 0xEE, 0xC6},   /* ind_ring   */
    {0x38, 0x54, 0x92, 0xFE}    /* ind_cross  */
};

#define ROW_BYTES 64    /* 512-pixel-wide screen / 8 pixels per byte */

pascal void drawIndicatorAt(int x, int y, long which)
{
    unsigned char *top, *bot;
    const unsigned char *glyph;
    int i;

    if ((unsigned long)which >= 6) return;
    glyph = kIconBytes[which];

    top = (unsigned char *) LMGetScrnBase();
    top += (x >> 3);
    top += y * ROW_BYTES;

    /* Bottom half mirrors top half: top row 0 == bottom row 7. */
    bot = top + 7 * ROW_BYTES;

    for (i = 0; i < 4; i++) {
        *top = glyph[i];
        *bot = glyph[i];
        top += ROW_BYTES;
        bot -= ROW_BYTES;
    }
}
