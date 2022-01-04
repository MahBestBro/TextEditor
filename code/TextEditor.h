#include "stdint.h"

#ifndef TEXT_EDITOR_H
#define TEXT_EDITOR_H

#define internal static
#define local_persist static 
#define global_variable static

#define Assert(cond) if (!(cond)) {*(int*)0 = 0;}

#define PIXEL_IN_BYTES 4

typedef uint8_t byte;
typedef uint8_t uchar;
typedef uint8_t uint8;
typedef uint32_t uint32;

typedef int32_t int32;

typedef wchar_t wchar;

struct ScreenBuffer
{
    void* memory;
    int width, height;
};

struct FontChar
{
    uint32 width, height; // Size of glyph
    uint32 left, top;     // Offset from baseline to left/top of glyph
    uint32 advance;       // Offset to advance to next glyph
    void* pixels;
};

void Draw(ScreenBuffer* screenBuffer, FontChar fontChars[128], int xOffset, int yOffset);

#endif
