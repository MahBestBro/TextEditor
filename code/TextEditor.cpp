#include "TextEditor.h"
#include "TextEditor_input.h"

void Draw(ScreenBuffer* screenBuffer, int xOffset, int yOffset)
{
    int width = screenBuffer->width;
    int height = screenBuffer->height;

    int pitch = width * PIXEL_IN_BYTES;
    byte* row = (byte*)screenBuffer->memory;
    for (int y = 0; y < screenBuffer->height; ++y)
    {
        byte* pixel = (byte*)row;
        for (int x = 0; x < screenBuffer->width; ++x)
        {
            //Pixel in memory: 0xBBGGRRxx
            *pixel = (byte)(x + xOffset);
            pixel++;
            *pixel = (byte)(y + yOffset);
            pixel++;
            *pixel = 0;
            pixel++;
            *pixel = 0;
            pixel++;
        }

        row += pitch;
    }
}