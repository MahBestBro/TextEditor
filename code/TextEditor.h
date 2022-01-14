#include "TextEditor_defs.h"
#include "TextEditor_input.h"

#ifndef TEXT_EDITOR_H
#define TEXT_EDITOR_H

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

inline int StringLen(const char* string)
{
    int result = 0;
    for (; string[0]; string++)
        result++;
    return result;
}

void Draw(float dt);
void Print(const char* message);

#endif
