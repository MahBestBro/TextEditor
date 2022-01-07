#include <windows.h> 

#include "TextEditor.h"
#include "TextEditor_input.h"

struct TimedEvent
{
    float elapsedTime;
    float interval;
    void (*OnTrigger)(void);
};

struct IntPair
{
    int x, y;
};

void HandleTimedEvent(TimedEvent* timedEvent, float dt, TimedEvent* nestedEvent)
{
    timedEvent->elapsedTime += dt;
    if (timedEvent->OnTrigger) timedEvent->OnTrigger(); //May not be needed???
    if (timedEvent->elapsedTime >= timedEvent->interval)
    {
        if (nestedEvent)
        {
            nestedEvent->elapsedTime += dt;
            if (nestedEvent->elapsedTime >= nestedEvent->interval)
            {
                if (nestedEvent->OnTrigger) nestedEvent->OnTrigger();
                nestedEvent->elapsedTime = 0.0f;
            }
        }
    }
}


void DrawRect(ScreenBuffer* screenBuffer, 
              int left, int right, int bottom, int top, 
              byte r, byte g, byte b)
{
    //TODO: Update asserts to make sure width and height do not go offscreen
    Assert(left < screenBuffer->width);
    Assert(top < screenBuffer->width * screenBuffer->height);

    int drawWidth = right - left;
    int drawHeight = top - bottom;

    int start = (left + bottom * screenBuffer->width) * PIXEL_IN_BYTES;
	byte* row = (byte*)screenBuffer->memory + start;
	for (int y = 0; y < drawHeight; ++y)
    {
        byte* pixel = row;
        for (int x = 0; x < drawWidth; ++x)
        {
            *pixel = b;
            pixel++;
            *pixel = g;
            pixel++;
            *pixel = r;
            pixel++;
            *pixel = 0;
            pixel++;
        }
		row += screenBuffer->width * PIXEL_IN_BYTES;
    }
}

void Draw8bppPixels(ScreenBuffer* screenBuffer, 
                    int left, int right, int bottom, int top, 
                    byte* pixels)
{
    //TODO: Update asserts to make sure width and height do not go offscreen
    Assert(left < screenBuffer->width);
    Assert(top < screenBuffer->width * screenBuffer->height);

    int drawWidth = right - left;
    int drawHeight = top - bottom;

    int start = (left + bottom * screenBuffer->width) * PIXEL_IN_BYTES;
	byte* row = (byte*)screenBuffer->memory + start;
	for (int y = drawHeight - 1; y >= 0; --y)
    {
        byte* pixel = row;
        for (int x = 0; x < drawWidth; ++x)
        {
            byte* drawnPixel = pixels + drawWidth * y + x;
            *pixel = 255 - *drawnPixel;
            pixel++;
            *pixel = 255 - *drawnPixel;
            pixel++;
            *pixel = 255 - *drawnPixel;
            pixel++;
            *pixel = 255 - *drawnPixel;
            pixel++;
        }
		row += screenBuffer->width * PIXEL_IN_BYTES;
    }
}

IntPair DrawText(ScreenBuffer* screenBuffer, FontChar fontChars[128], char* text, 
                 int xCoord, int yCoord)
{
    char* at = text;
	int xAdvance = 0;
    int yAdvance = 0;
    for (; at[0]; at++)
    {
		if (at[0] == '\n')
        {
            yAdvance += FONT_SIZE;
            xAdvance = 0;
        }
        else if (at[0] == '\t')
        {
            FontChar fc = fontChars[' '];
            int xOffset = fc.left + xAdvance + xCoord;
            int yOffset = yCoord - (fc.height - fc.top) - yAdvance;
            //TODO: change this so that it aligns with tabs
            for (int _ = 0; _ < 4; ++_)
            {
                Draw8bppPixels(screenBuffer, 
                               xOffset, xOffset + fc.width, yOffset, yOffset + fc.height,
                               (byte*)fc.pixels);
                xOffset += fc.left;
            }
            xAdvance += 4 * (fc.advance / 64);
        }
		else
		{
            FontChar fc = fontChars[at[0]];
            int xOffset = fc.left + xAdvance + xCoord;
            int yOffset = yCoord - (fc.height - fc.top) - yAdvance;
            Draw8bppPixels(screenBuffer, 
                           xOffset, xOffset + fc.width, yOffset, yOffset + fc.height,
                           (byte*)fc.pixels);
		    xAdvance += fc.advance / 64;
		}
        
    }

	return {xAdvance, -yAdvance};
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

void AddChar()
{
    text[numChars] = currentChar;
    numChars += (numChars < 255);
}

void RemoveChar()
{
    numChars -= (numChars > 0);
    text[numChars] = 0;
}

TimedEvent cursorBlink = {0.0f, 0.5f, 0};
TimedEvent holdChar = {0.0f, 0.5f, 0};
TimedEvent repeatChar = {0.0f, 0.02f, &AddChar};
TimedEvent holdBackspace = {0.0f, 0.5f, 0};
TimedEvent repeatBackspace = {0.0f, 0.02f, &RemoveChar};

bool capslockOn = false;

void Draw(ScreenBuffer* screenBuffer, FontChar fontChars[128], Input* input, float dt)
{
    bool charKeyPressed = false;
    for (int code = (int)INPUTCODE_SPACE; code < (int)NUM_INPUTS; ++code)
    {
        if (InputDown(input->flags[code]))
        {
            char charOfKeyPressed = InputCodeToChar(
                (InputCode)code, 
                InputHeld(input->leftShift), 
                capslockOn);
            currentChar = charOfKeyPressed;
            AddChar();
        }
        else if (InputHeld(input->flags[code]))
            charKeyPressed = true;
    }

    if (charKeyPressed)
    {
        HandleTimedEvent(&holdChar, dt, &repeatChar);
    }
    else
    {
        holdChar.elapsedTime = 0.0f;

        if (InputDown(input->backspace))
            RemoveChar();
        else if (InputHeld(input->backspace))
            HandleTimedEvent(&holdBackspace, dt, &repeatBackspace);  
        else 
            holdBackspace.elapsedTime = 0.0f;

        if (InputDown(input->capsLock))
            capslockOn = !capslockOn;
    }

    //Draw Background;
    DrawRect(screenBuffer, 0, screenBuffer->width, 0, screenBuffer->height, 255, 255, 255);
    IntPair cursorPos = DrawText(screenBuffer, fontChars, text, 25, 300);

    cursorBlink.elapsedTime += dt;
    if ((!charKeyPressed && !InputHeld(input->backspace)) && 
        cursorBlink.elapsedTime >= cursorBlink.interval)
    {
        holdChar.elapsedTime = 0.0f;
        if (cursorBlink.elapsedTime >= 2 * cursorBlink.interval) 
            cursorBlink.elapsedTime = 0.0f;
    }
    else
    {
        DrawCursor(screenBuffer, 25 + cursorPos.x, 300 + cursorPos.y);
    }
}
