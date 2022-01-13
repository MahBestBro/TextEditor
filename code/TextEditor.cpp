#include <windows.h> 

#include "TextEditor.h"
#include "TextEditor_input.h"

#define INITIAL_LINE_SIZE 256
#define CURSOR_HEIGHT 18

extern Input input;
extern FontChar fontChars[128];
extern ScreenBuffer screenBuffer;

struct IntPair
{
    int x, y;
};

struct Colour
{
    byte r, g, b;
};

struct Rect
{
    int left, right, bottom, top;
};

struct Line
{
    char* text;
    int size;
    int len;
    bool changed;
};

Line InitLine(const char* text = NULL)
{
    Line result;
	result.size = INITIAL_LINE_SIZE;
    result.text = (char*)malloc(result.size);
    if (text)
    {
        result.len = StringLen(text);
        memcpy(result.text, text, result.len);
    }
    else
    {
        result.len = 0;
    }
    result.text[result.len] = 0;
    return result;  
}

struct TimedEvent
{
    float interval;
    void (*OnTrigger)(void) = 0;
    float elapsedTime = 0.0f;
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

void IntToString(int val, char* buffer)
{
	char* at = buffer;
    int len = 0;
    do 
    {
        *at = (val % 10) + '0';
        val /= 10;
		len++;
        at++;
	} while (val > 0);

    for (int i = 0; i < len / 2; ++i)
    {
        char temp = buffer[len - i - 1];
        buffer[len - i - 1] = buffer[i];
        buffer[i] = temp; 
    }

    buffer[len] = 0;
}

void DrawRect(Rect rect, Colour colour)
{
    Assert(rect.left <= rect.right);
    Assert(rect.bottom <= rect.top);

    rect.left   = Clamp(rect.left,   0, screenBuffer.width);
    rect.right  = Clamp(rect.right,  0, screenBuffer.width);
    rect.bottom = Clamp(rect.bottom, 0, screenBuffer.height);
    rect.top    = Clamp(rect.top,    0, screenBuffer.height);

    int drawWidth = rect.right - rect.left;
    int drawHeight = rect.top - rect.bottom;

    int start = (rect.left + rect.bottom * screenBuffer.width) * PIXEL_IN_BYTES;
	byte* row = (byte*)screenBuffer.memory + start;
	for (int y = 0; y < drawHeight; ++y)
    {
        byte* pixel = row;
        for (int x = 0; x < drawWidth; ++x)
        {
            *pixel = colour.b;
            pixel++;
            *pixel = colour.g;
            pixel++;
            *pixel = colour.r;
            pixel++;
            *pixel = 0;
            pixel++;
        }
		row += screenBuffer.width * PIXEL_IN_BYTES;
    }
}

void Draw8bppPixels(Rect rect, byte* pixels, Colour colour,
                    int xCutoff, int yCutoff)
{
    //Assert(rect.left < screenBuffer.width && rect.right <= screenBuffer.width);
    //Assert(rect.left >= 0 && rect.right >= 0);
    //Assert(rect.bottom < screenBuffer.height && rect.top <= screenBuffer.height);
    //Assert(rect.top >= 0 && rect.bottom >= 0);
    Assert(rect.left <= rect.right);
    Assert(rect.bottom <= rect.top);

    rect.left   = Clamp(rect.left,   xCutoff, screenBuffer.width);
    rect.right  = Clamp(rect.right,  xCutoff, screenBuffer.width);
    rect.bottom = Clamp(rect.bottom, yCutoff, screenBuffer.height);
    rect.top    = Clamp(rect.top,    yCutoff, screenBuffer.height);

    int drawWidth = rect.right - rect.left;
    int drawHeight = rect.top - rect.bottom;

    int start = (rect.left + rect.bottom * screenBuffer.width) * PIXEL_IN_BYTES;
	byte* row = (byte*)screenBuffer.memory + start;
	for (int y = drawHeight - 1; y >= 0; --y)
    {
        byte* pixel = row;
        for (int x = 0; x < drawWidth; ++x)
        {
            byte* drawnPixel = pixels + drawWidth * y + x;
            byte drawnR = (byte)(*drawnPixel / 255.0f * colour.r);
            byte drawnB = (byte)(*drawnPixel / 255.0f * colour.b);
            byte drawnG = (byte)(*drawnPixel / 255.0f * colour.g);
            *pixel = 255 - *drawnPixel + drawnB;
            pixel++;
            *pixel = 255 - *drawnPixel + drawnG;
            pixel++;
            *pixel = 255 - *drawnPixel + drawnR;
            pixel++;
            *pixel = 0;
            pixel++;
        }
		row += screenBuffer.width * PIXEL_IN_BYTES;
    }
}

//TODO: Fix bug where w's and v's glitch at edge of screen (my guess is the width > the advance)
void DrawText(char* text, int xCoord, int yCoord, Colour colour, int xCutoff = 0, int yCutoff = 0)
{
    char* at = text; 
	int xAdvance = 0;
    int yAdvance = 0;
    for (; at[0]; at++)
    {
        if (at[0] == '\t')
        {
            FontChar fc = fontChars[' '];
            int xOffset = fc.left + xAdvance + xCoord;
            int yOffset = yCoord - (fc.height - fc.top) - yAdvance;
            Rect charDims = {xOffset, (int)(xOffset + fc.width), yOffset, 
                             (int)(yOffset + fc.height)};
            //TODO: change this so that it aligns with tabs
            for (int _ = 0; _ < 4; ++_)
            {
                Draw8bppPixels(charDims, (byte*)fc.pixels, colour, xCutoff, yCutoff);
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
            Draw8bppPixels(charDims, (byte*)fc.pixels, colour, xCutoff, yCutoff);
		    xAdvance += fc.advance / 64;
		}
        
    }
}

char currentChar = 0;
const int MAX_LINES = 256;
Line fileText[MAX_LINES];
int numLines = 1;
bool textInitialised = false;

int cursorTextIndex = 0;
int cursorLineIndex = 0;

IntPair textOffset = {};

void MoveCursorForward()
{
    if (cursorTextIndex < fileText[cursorLineIndex].len)
    {
        cursorTextIndex++;
    }
    else if (cursorLineIndex < numLines - 1)
    {
        //Go down a line
        cursorLineIndex++;
        cursorTextIndex = 0;
    }
}

void MoveCursorBackward()
{
    if (cursorTextIndex > 0)
    {
        cursorTextIndex--;
    }
    else if (cursorLineIndex > 0)
    {
        //Go up a line
        cursorLineIndex--;
        cursorTextIndex = fileText[cursorLineIndex].len;
    }
}

void MoveCursorUp()
{
    cursorLineIndex -= (cursorLineIndex > 0);
    cursorTextIndex = min(cursorTextIndex, fileText[cursorLineIndex].len);
}

void MoveCursorDown()
{
    cursorLineIndex += (cursorLineIndex < numLines);
    cursorTextIndex = min(cursorTextIndex, fileText[cursorLineIndex].len);
}


void AddChar()
{
    Line* line = &fileText[cursorLineIndex];
    if (line->len < INITIAL_LINE_SIZE)
    {  
        line->len += (currentChar == '\t') ? 4 : 1;

        //Shift right
        int offset = 4 * (currentChar == '\t');
        for (int i = line->len - 1; i > cursorTextIndex + offset; --i)
            line->text[i] = line->text[i-1];
        
        //Add character(s) and advance cursor
        //TODO: realloc line.text if line.len >= line.size
        line->text[cursorTextIndex] = (currentChar == '\t') ? ' ' : currentChar;
		line->text[line->len] = 0;
        cursorTextIndex++;
        if (currentChar == '\t')
        {
            for (int _ = 0; _ < 3; ++_)
            {
                line->text[cursorTextIndex] = (currentChar == '\t') ? ' ' : currentChar;
                cursorTextIndex++;
            }
        }
        
    }
}

void RemoveChar()
{
	//TODO: Fix bug where previous lines are deleted from existance
    Line* line = &fileText[cursorLineIndex];
    if (cursorTextIndex > 0)
    {
		line->len--;
		for (int i = cursorTextIndex - 1; i < line->len; ++i)
		{
			line->text[i] = line->text[i+1];
		}
        line->text[line->len] = 0;
        cursorTextIndex--;
    }
    else
    {
        if (numLines > 1)
        {
            cursorLineIndex--;
            numLines--;
            cursorTextIndex = fileText[cursorLineIndex].len;
        }
    }
}

void AddLine()
{
    if (numLines < MAX_LINES)
    {
        int prevLineIndex = cursorLineIndex;
        cursorLineIndex++;
        int copiedLen = fileText[prevLineIndex].len - cursorTextIndex;
        memcpy(
            fileText[cursorLineIndex].text, 
            fileText[prevLineIndex].text + cursorTextIndex, 
            copiedLen
        );
        fileText[prevLineIndex].len -= copiedLen;
        fileText[prevLineIndex].text[cursorTextIndex] = 0;
        fileText[cursorLineIndex].len = copiedLen;
        fileText[cursorLineIndex].text[fileText[prevLineIndex].len] = 0;
        cursorTextIndex = 0;
        numLines++;
    }
}


TimedEvent cursorBlink = {0.5f};
TimedEvent holdChar = {0.5f};
TimedEvent repeatChar = {0.02f, &AddChar};

TimedEvent holdAction = {0.5f};
TimedEvent repeatAction = {0.02f};

bool capslockOn = false;
bool nonCharKeyPressed = false;

void Draw(float dt)
{
    if (!textInitialised)
    {
        for (int i = 0; i < MAX_LINES; ++i)
            fileText[i] = InitLine();
		textInitialised = true;
    }

    //Detect key input and handle char input
    bool charKeyDown = false;
    bool charKeyPressed = false;
    bool newCharKeyPressed = false;
    for (int code = (int)KEYS_START; code < (int)NUM_INPUTS; ++code)
    {
        if (code >= (int)CHAR_KEYS_START)
        {
            if (InputDown(input.flags[code]))
            {
                char charOfKeyPressed = InputCodeToChar(
                (InputCode)code, 
                InputHeld(input.leftShift), 
                capslockOn);
                currentChar = charOfKeyPressed;
                AddChar();

                charKeyDown = true;
                if (charKeyPressed)
                    newCharKeyPressed = true;
                nonCharKeyPressed = false;
            }
            else if (InputHeld(input.flags[code]))
            {
                charKeyPressed = true;
                if (charKeyDown)
                    newCharKeyPressed = true;
            }
        }
        else if (InputDown(input.flags[code]))
        {
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

        byte inputFlags[] = 
        {
            input.enter,
            input.backspace,
            input.right,
            input.left,
            input.up,
            input.down
        };

        void (*inputCallbacks[])(void) = 
        {
            AddLine, 
            RemoveChar, 
            MoveCursorForward, 
            MoveCursorBackward, 
            MoveCursorUp, 
            MoveCursorDown
        };

        bool inputHeld = false;
        for (int i = 0; i < ArrayLen(inputFlags); ++i)
        {
            if (InputDown(inputFlags[i]))
            {
                repeatAction.OnTrigger = inputCallbacks[i];
                inputCallbacks[i]();
                holdAction.elapsedTime = 0.0f;
            }
            else if (InputHeld(inputFlags[i]))
            {
                inputHeld = true;
            }
        } 

        if (inputHeld)
            HandleTimedEvent(&holdAction, dt, &repeatAction);
        else
            holdAction.elapsedTime = 0.0f;

        if (InputDown(input.capsLock))
            capslockOn = !capslockOn;

    }

    //Draw Background
    Rect screenDims = {0, screenBuffer.width, 0, screenBuffer.height};
    DrawRect(screenDims, {255, 255, 255});
    
    //Get correct position for cursor
    const IntPair start = {52, 600}; 
    IntPair cursorPos = start;
    for (int i = 0; i < cursorTextIndex; ++i)
        cursorPos.x += fontChars[fileText[cursorLineIndex].text[i]].advance / 64;
    cursorPos.y -= cursorLineIndex * CURSOR_HEIGHT;
    cursorPos.y -= 5; //Offset from baseline

    if (cursorPos.x > screenBuffer.width)
        textOffset.x = max(textOffset.x, cursorPos.x - screenBuffer.width);
    else
        textOffset.x = 0;

    //Draw text
    for (int i = 0; i < numLines; ++i)
    {
        int x = start.x - textOffset.x;
        int y = start.y - i * CURSOR_HEIGHT - textOffset.y;
        DrawText(fileText[i].text, x, y, {0}, start.x);

        char lineNumText[8];
        IntToString(i + 1, lineNumText);
        int lineNumOffset = (fontChars[' '].advance / 64) * 4;
        DrawText(lineNumText, start.x - lineNumOffset, y, {255, 0, 0});
    }

    cursorBlink.elapsedTime += dt;
    bool cursorMoving = charKeyPressed || InputHeld(input.backspace) || ArrowKeysHeld(input.arrowKeys);
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
        const int CURSOR_WIDTH = 2;
        Rect cursorDims = {cursorPos.x, cursorPos.x + CURSOR_WIDTH, 
                           cursorPos.y, cursorPos.y + CURSOR_HEIGHT};
        DrawRect(cursorDims, {0, 255, 0});
    }
}
