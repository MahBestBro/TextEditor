#ifndef TEXTEDITOR_FONT_H
#define TEXTEDITOR_FONT_H

#include "TextEditor_defs.h"

struct FontChar
{
    uint32 width, height; // Size of glyph
    uint32 left, top;     // Offset from baseline to left/top of glyph
    uint32 advance;       // Offset to advance to next glyph
    uint8* pixels;
};

struct Font
{
    FontChar chars[128];
    int sizeIndex = 4;
    uint32 maxHeight;
    uint32 lineGap;
    uint32 offsetBelowBaseline;
};

global_variable const uint32 fontSizes[] = {8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36, 48, 72};
global_variable Font fontData;

inline int PointsToPix(int points)
{
    return 4 * points / 3;
}

void ResizeFont(int fontSizeIndex);
void ChangeFont(string ttfFileName);

#endif