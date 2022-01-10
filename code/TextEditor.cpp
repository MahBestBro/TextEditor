#include <windows.h> 

#include "TextEditor.h"
#include "TextEditor_input.h"

struct IntPair
{
    int x, y;
};

struct Rect
{
    int left, right, bottom, top;
};

struct Line
{
    char* text;
    int len;
};

struct TimedEvent
{
    float elapsedTime;
    float interval;
    void (*OnTrigger)(void);
};

void HandleTimedEvent(TimedEvent* timedEvent, float dt, TimedEvent* nestedEvent)
{
    timedEvent->elapsedTime += dt;
    if (timedEvent->elapsedTime >= timedEvent->interval)
    {
        if (timedEvent->OnTrigger) timedEvent->OnTrigger(); //May not be needed???
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

void DrawRect(ScreenBuffer* screenBuffer, Rect rect, byte r, byte g, byte b)
{
    Assert(rect.left < screenBuffer->width && rect.right <= screenBuffer->width);
    Assert(rect.left >= 0 && rect.right >= 0);
    Assert(rect.bottom < screenBuffer->height && rect.top <= screenBuffer->height);
    Assert(rect.top >= 0 && rect.bottom >= 0);
    Assert(rect.left <= rect.right);
    Assert(rect.bottom <= rect.top);

    int drawWidth = rect.right - rect.left;
    int drawHeight = rect.top - rect.bottom;

    int start = (rect.left + rect.bottom * screenBuffer->width) * PIXEL_IN_BYTES;
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

void Draw8bppPixels(ScreenBuffer* screenBuffer, Rect rect, byte* pixels)
{
    Assert(rect.left < screenBuffer->width && rect.right <= screenBuffer->width);
    Assert(rect.left >= 0 && rect.right >= 0);
    Assert(rect.bottom < screenBuffer->height && rect.top <= screenBuffer->height);
    Assert(rect.top >= 0 && rect.bottom >= 0);
    Assert(rect.left <= rect.right);
    Assert(rect.bottom <= rect.top);

    int drawWidth = rect.right - rect.left;
    int drawHeight = rect.top - rect.bottom;

    int start = (rect.left + rect.bottom * screenBuffer->width) * PIXEL_IN_BYTES;
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
    Assert(xCoord > 0 && xCoord <= screenBuffer->width);
    Assert(yCoord > 0 && yCoord <= screenBuffer->height);

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
            Rect charDims = {xOffset, (int)(xOffset + fc.width), yOffset, 
                             (int)(yOffset + fc.height)};
            //TODO: change this so that it aligns with tabs
            for (int _ = 0; _ < 4; ++_)
            {
                Draw8bppPixels(screenBuffer, charDims, (byte*)fc.pixels);
                xOffset += fc.left;
            }
            xAdvance += 4 * (fc.advance / 64);
        }
		else
		{
            FontChar fc = fontChars[at[0]];
            int xOffset = fc.left + xAdvance + xCoord;
            int yOffset = yCoord - (fc.height - fc.top) - yAdvance;
            Rect charDims = {xOffset, (int)(xOffset + fc.width), yOffset, 
                             (int)(yOffset + fc.height)};
            Draw8bppPixels(screenBuffer, charDims, (byte*)fc.pixels);
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

int cursorTextIndex = 0;
int cursorLineIndex = 0;

void MoveCursorForward()
{
    cursorTextIndex += (cursorTextIndex < numChars);
}

void MoveCursorBackward()
{
    cursorTextIndex -= (cursorTextIndex > 0);
}

void AddChar()
{
    if (numChars < 255)
    {
        numChars += (currentChar == '\t') ? 4 : 1;
        
        //Shift right
        int offset = 4 * (currentChar == '\t');
        for (int i = numChars - 1; i > cursorTextIndex + offset; --i)
            text[i] = text[i-1];
        
        //Add character(s) and advance cursor
        text[cursorTextIndex] = (currentChar == '\t') ? ' ' : currentChar;
        cursorTextIndex++;
        if (currentChar == '\t')
        {
            for (int _ = 0; _ < 3; ++_)
            {
                text[cursorTextIndex] = (currentChar == '\t') ? ' ' : currentChar;
                cursorTextIndex++;
            }
        }
        
    }
}

void RemoveChar()
{
    if (cursorTextIndex > 0)
    {
		numChars--;
		for (int i = cursorTextIndex - 1; i < numChars; ++i)
		{
			text[i] = text[i+1];
		}
        text[numChars] = 0;
        cursorTextIndex--;
    }
}


TimedEvent cursorBlink = {0.0f, 0.5f, 0};
TimedEvent holdChar = {0.0f, 0.5f, 0};
TimedEvent repeatChar = {0.0f, 0.02f, &AddChar};

TimedEvent holdAction = {0.0f, 0.5f, 0};
TimedEvent repeatAction = {0.0, 0.02f, 0};

bool capslockOn = false;
bool nonCharKeyPressed = false;

void Draw(ScreenBuffer* screenBuffer, FontChar fontChars[128], Input* input, float dt)
{
    //Detect key input and handle char input
    bool charKeyPressed = false;
    bool newCharKeyPressed = false;
    for (int code = (int)KEYS_START; code < (int)NUM_INPUTS; ++code)
    {
        if (code >= (int)CHAR_KEYS_START)
        {
            if (InputDown(input->flags[code]))
            {
                char charOfKeyPressed = InputCodeToChar(
                    (InputCode)code, 
                    InputHeld(input->leftShift), 
                    capslockOn);
                currentChar = charOfKeyPressed;
                AddChar();

                if (charKeyPressed)
                    newCharKeyPressed = true;
                nonCharKeyPressed = false;
            }
            else if (InputHeld(input->flags[code]))
                charKeyPressed = true;
        }
        else
        {
            if (InputDown(input->flags[code]))
                nonCharKeyPressed = true;
        }
        
    }
    
    //Handle non char input and trigger timed events
    if (charKeyPressed && !nonCharKeyPressed)
    {
        if (newCharKeyPressed)
            holdChar.elapsedTime = 0.0f;
        HandleTimedEvent(&holdChar, dt, &repeatChar);
    }
    else
    {
        holdChar.elapsedTime = 0.0f;

        if (InputDown(input->backspace))
        {
            RemoveChar();
            repeatAction.OnTrigger = RemoveChar;
			holdAction.elapsedTime = 0.0f;
        }
        else if (InputDown(input->right))
        {
            MoveCursorForward();
            repeatAction.OnTrigger = MoveCursorForward;
			holdAction.elapsedTime = 0.0f;
        }
        else if (InputDown(input->left))
        {
            MoveCursorBackward();
            repeatAction.OnTrigger = MoveCursorBackward;
			holdAction.elapsedTime = 0.0f;
        }
        

        if (InputHeld(input->backspace) || InputHeld(input->right) || InputHeld(input->left))
            HandleTimedEvent(&holdAction, dt, &repeatAction);
        else
            holdAction.elapsedTime = 0.0f;

        if (InputDown(input->capsLock))
            capslockOn = !capslockOn;

    }

    //Draw Background
    Rect screenDims = {0, screenBuffer->width, 0, screenBuffer->height};
    DrawRect(screenBuffer, screenDims, 255, 255, 255);
    
    //Draw text and get correct position for cursor
    IntPair cursorPos = DrawText(screenBuffer, fontChars, text, 25, 300);
    for (int i = numChars - 1; i >= cursorTextIndex; --i)
    {
        if (text[i] == '\n')
        {
            //Okay we're probably gonna need a line system to do this
        }
        else if (text[i] == '\t')
        {
            FontChar spaceGlyph = fontChars[' '];
            cursorPos.x -= (spaceGlyph.advance / 64) * 4;
        }
        else
        {
            FontChar fc = fontChars[text[i]];
            cursorPos.x -= fc.advance / 64;
        }
    }    

    cursorBlink.elapsedTime += dt;
    bool cursorMoving = charKeyPressed || InputHeld(input->backspace) || ArrowKeysHeld(input->arrowKeys);
    if (!cursorMoving && cursorBlink.elapsedTime >= cursorBlink.interval)
    {
        holdChar.elapsedTime = 0.0f;
        if (cursorBlink.elapsedTime >= 2 * cursorBlink.interval) 
            cursorBlink.elapsedTime = 0.0f;
    }
    else
    {
        if (cursorMoving) cursorBlink.elapsedTime = 0.0f;
        //Draw Cursor
        const int cursorWidth = 2;
        const int cursorHeight = 13;
        int cursorX = 25 + cursorPos.x; 
        int cursorY = 300 + cursorPos.y;
        Rect cursorDims = {cursorX, cursorX + cursorWidth, cursorY, cursorY + cursorHeight};
        DrawRect(screenBuffer, cursorDims, 0, 255, 0);
    }
}
