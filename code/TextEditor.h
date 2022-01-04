#include "stdint.h"

#ifndef TEXT_EDITOR_H
#define TEXT_EDITOR_H

#define internal static
#define local_persist static 
#define global_variable static

#define Assert(cond) if (!(cond)) {*(int*)0 = 0;}

#define PIXEL_IN_BYTES 4

typedef uint8_t byte;
typedef uint32_t uint32;

typedef wchar_t wchar;

struct ScreenBuffer
{
    void* memory;
    int width, height;
};

void Draw(ScreenBuffer* screenBuffer, int xOffset, int yOffset);

#endif