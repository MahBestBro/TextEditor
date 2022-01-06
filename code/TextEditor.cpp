#include <windows.h> 

#include "TextEditor.h"
#include "TextEditor_input.h"
#include "TextEditor_font.h"

struct TimedEvent
{
    float elapsedTime;
    float interval;
};

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

int DrawText(ScreenBuffer* screenBuffer, FontChar fontChars[128], char* text, int xCoord, int yCoord)
{
    char* at = text;
	int advance = 0;
    for (; at[0]; at++)
    {
        FontChar fc = fontChars[at[0]];

        int glyphWidth = (int)fc.width;
        int glyphHeight = (int)fc.height;

        int xOffset = fc.left + advance + xCoord;
        int yOffset = (yCoord - (fc.height - fc.top)) * screenBuffer->width;
        
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

	return advance;
}

void DrawCursor(ScreenBuffer* screenBuffer, int xCoord, int yCoord)
{
    int cursorWidth = 2;
    int cursorHeight = 13;

    int xPos = xCoord;
    int yPos = (yCoord * screenBuffer->width);

    byte* row = (byte*)screenBuffer->memory + (xPos + yPos) * PIXEL_IN_BYTES;
    for (int y = 0; y < cursorHeight; ++y)
    {
        byte* pixel = row;
        for (int x = 0; x < cursorWidth; ++x)
        {
            *pixel = 0;
            pixel++;
            *pixel = 255;
            pixel++;
            *pixel = 0;
            pixel++;
            *pixel = 0;
            pixel++;
        }

        row += screenBuffer->width * PIXEL_IN_BYTES;
    } 
}

char currentChar = 0;
//TODO: Make this dynamic (or change into lines)
char text[256];
int numChars = 0;

TimedEvent cursorBlink = {0.0f, 0.5f};
TimedEvent holdChar = {0.0f, 0.5f};
TimedEvent repeatChar = {0.0f, 0.02f};

void Draw(ScreenBuffer* screenBuffer, FontChar fontChars[128], Input* input, float dt)
{
    cursorBlink.elapsedTime += dt;

    bool typing = false;
    for (int code = (int)INPUTCODE_SPACE; code < (int)NUM_INPUTS; ++code)
    {
        if (InputDown(input->flags[code]))
        {
            char charOfKeyPressed = InputCodeToChar((InputCode)code, InputHeld(input->leftShift));
            text[numChars++] = charOfKeyPressed;
            currentChar = charOfKeyPressed;
        }
        else if (InputHeld(input->flags[code]))
            typing = true;
    }

    if (!typing)
    {
        numChars -= InputDown(input->backspace);
        text[numChars] = 0;
    }

    DrawBackground(screenBuffer);
    int cursorX = DrawText(screenBuffer, fontChars, text, 25, 300);

    if (!typing && cursorBlink.elapsedTime >= cursorBlink.interval)
    {
        holdChar.elapsedTime = 0.0f;
        if (cursorBlink.elapsedTime >= 2 * cursorBlink.interval) 
            cursorBlink.elapsedTime = 0.0f;
    }
    else
    {
        DrawCursor(screenBuffer, 25 + cursorX, 300);
        if (typing)
        {
            holdChar.elapsedTime += dt;
            if (holdChar.elapsedTime >= holdChar.interval)
            {
                repeatChar.elapsedTime += dt;
                if (repeatChar.elapsedTime >= repeatChar.interval)
                {
                    text[numChars++] = currentChar;
                    repeatChar.elapsedTime = 0.0f;
                }
            }
        }
    }
}
