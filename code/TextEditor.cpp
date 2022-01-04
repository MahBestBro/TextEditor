#include "TextEditor.h"
#include "TextEditor_input.h"
#include "TextEditor_font.h"

void DrawBackground(ScreenBuffer* screenBuffer)
{
    int width = screenBuffer->width;
    int height = screenBuffer->height;

    int pitch = width * PIXEL_IN_BYTES;
    byte* row = (byte*)screenBuffer->memory;
    for (int y = 0; y < height; ++y)
    {
        byte* pixel = (byte*)row;
        for (int x = 0; x < width; ++x)
        {
            //Pixel in memory: 0xBBGGRRxx
            *pixel = 255;
            pixel++;
            *pixel = 255;
            pixel++;
            *pixel = 255;
            pixel++;
            *pixel = 0;
            pixel++;
        }

        row += pitch;
    }
}

void DrawText(ScreenBuffer* screenBuffer, FontChar fontChars[128], char* text, int xCoord, int yCoord)
{
    char* at = text;
	int advance = 0;
    for (; at[0]; at++)
    {
        FontChar fc = fontChars[at[0]];

        int glyphWidth = (int)fc.width;
        int glyphHeight = (int)fc.height;

        int xOffset = fc.left + advance + xCoord;
        int yOffset = ((fc.height - fc.top) + yCoord) * screenBuffer->width;
        
        Assert(xOffset < screenBuffer->width);
        Assert(yOffset < screenBuffer->width * screenBuffer->height);
		byte* row = (byte*)screenBuffer->memory + (xOffset + yOffset) * PIXEL_IN_BYTES;
		for (int y = glyphHeight - 1; y >= 0; --y)
        {
            byte* pixel = row;
            for (int x = 0; x < glyphWidth; ++x)
            {
                byte* glyphPixel = (byte*)fc.pixels + fc.width * y + x;
                *pixel = 255 - *glyphPixel;
                pixel++;
                *pixel = 255 - *glyphPixel;
                pixel++;
                *pixel = 255 - *glyphPixel;
                pixel++;
                *pixel = 255 - *glyphPixel;
                pixel++;
                
            }
			row += screenBuffer->width * PIXEL_IN_BYTES;
        }

		advance += fc.advance / 64;
    }

	int test = 0;
}

void Draw(ScreenBuffer* screenBuffer, FontChar fontChars[128], int xOffset, int yOffset)
{
    DrawBackground(screenBuffer);

    DrawText(screenBuffer, fontChars, "ur mom\t", 300, 300);
}
