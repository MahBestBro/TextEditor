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

struct ColourRGBA
{
    byte r, g, b, a;
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

int TextPixelLength(char* text, int len)
{
    int result = 0;
    for (int i = 0; i < len; i++)
        result += fontChars[text[i]].advance;
    return result;
}

void DrawRect(Rect rect, Colour colour, Rect limits = {0})
{
    Assert(rect.left <= rect.right);
    Assert(rect.bottom <= rect.top);

    if (!limits.right) limits.right = screenBuffer.width;
    if (!limits.top) limits.top = screenBuffer.height;

    rect.left   = Clamp(rect.left,   limits.left, limits.right);
    rect.right  = Clamp(rect.right,  limits.left, limits.right);
    rect.bottom = Clamp(rect.bottom, limits.bottom, limits.top);
    rect.top    = Clamp(rect.top,    limits.bottom, limits.top);

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

void DrawAlphaRect(Rect rect, ColourRGBA colour, Rect limits = {0})
{
    Assert(rect.left <= rect.right);
    Assert(rect.bottom <= rect.top);

    if (!limits.right) limits.right = screenBuffer.width;
    if (!limits.top) limits.top = screenBuffer.height;

    rect.left   = Clamp(rect.left,   limits.left, limits.right);
    rect.right  = Clamp(rect.right,  limits.left, limits.right);
    rect.bottom = Clamp(rect.bottom, limits.bottom, limits.top);
    rect.top    = Clamp(rect.top,    limits.bottom, limits.top);

    int drawWidth = rect.right - rect.left;
    int drawHeight = rect.top - rect.bottom;

    int start = (rect.left + rect.bottom * screenBuffer.width) * PIXEL_IN_BYTES;
	byte* row = (byte*)screenBuffer.memory + start;
	for (int y = 0; y < drawHeight; ++y)
    {
        byte* pixel = row;
        for (int x = 0; x < drawWidth; ++x)
        {
            float alphaA = colour.a / 255.f;
            float alphaB = 1.f;
            float drawnAlpha = alphaA + alphaB * (1.f - alphaA);

            byte pixelB = pixel[0];
            byte pixelG = pixel[1];
            byte pixelR = pixel[2];
            byte drawnR = (byte)((colour.r*alphaA + pixelR*alphaB*(1 - alphaA)) / drawnAlpha);  
            byte drawnG = (byte)((colour.g*alphaA + pixelG*alphaB*(1 - alphaA)) / drawnAlpha);  
            byte drawnB = (byte)((colour.b*alphaA + pixelB*alphaB*(1 - alphaA)) / drawnAlpha);  
            
            *pixel = drawnB;
            pixel++;
            *pixel = drawnG;
            pixel++;
            *pixel = drawnR;
            pixel++;
            *pixel = 0;
            pixel++;
        }
		row += screenBuffer.width * PIXEL_IN_BYTES;
    }

}

//TODO: Allow drawing backwards for text on left going offscreen
void Draw8bppPixels(Rect rect, byte* pixels, int stride, Colour colour, Rect limits)
{
    Assert(rect.left <= rect.right);
    Assert(rect.bottom <= rect.top);

    if (!limits.right) limits.right = screenBuffer.width;
    if (!limits.top) limits.top = screenBuffer.height;

    rect.left   = Clamp(rect.left,   limits.left, limits.right);
    rect.right  = Clamp(rect.right,  limits.left, limits.right);
    rect.bottom = Clamp(rect.bottom, limits.bottom, limits.top);
    rect.top    = Clamp(rect.top,    limits.bottom, limits.top);

    int drawWidth = rect.right - rect.left;
    int drawHeight = rect.top - rect.bottom;

    int start = (rect.left + rect.bottom * screenBuffer.width) * PIXEL_IN_BYTES;
	byte* row = (byte*)screenBuffer.memory + start;
	for (int y = drawHeight - 1; y >= 0; --y)
    {
        byte* pixel = row;
        for (int x = 0; x < drawWidth; ++x)
        {
            byte* drawnPixel = pixels + stride * y + x;
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

void DrawText(char* text, int xCoord, int yCoord, Colour colour, Rect limits = {0})
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
                Draw8bppPixels(charDims, (byte*)fc.pixels, fc.width, colour, limits);
                xOffset += fc.left;
            }
            xAdvance += 4 * (fc.advance);
        }
		else
		{
            FontChar fc = fontChars[at[0]];
            int xOffset = fc.left + xAdvance + xCoord;
            int yOffset = yCoord - (fc.height - fc.top) - yAdvance;
            Rect charDims = {xOffset, (int)(xOffset + fc.width), yOffset, 
                             (int)(yOffset + fc.height)};
            Draw8bppPixels(charDims, (byte*)fc.pixels, fc.width, colour, limits);
		    xAdvance += fc.advance;
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

int highlightFrontIndex = -1;
int highlightBackIndex = -1;

IntPair textOffset = {};

void MoveCursorForward()
{
    if (cursorTextIndex < fileText[cursorLineIndex].len)
    {  
        cursorTextIndex++;
        if (InputHeld(input.leftShift))
        {
            Assert(!Xor(highlightFrontIndex == -1, highlightBackIndex == -1));
            if (highlightFrontIndex == highlightBackIndex) 
            {
                highlightFrontIndex = cursorTextIndex - 1;
                highlightBackIndex  = cursorTextIndex;
            }
            else
            { 
                int frontIndexDistFromCursor = abs(cursorTextIndex - highlightFrontIndex);
                int backIndexDistFromCursor =  abs(cursorTextIndex - highlightBackIndex);
                if (frontIndexDistFromCursor <= backIndexDistFromCursor ||
                    backIndexDistFromCursor == 0)
					highlightFrontIndex++;
				else
					highlightBackIndex++;
            }
        }
        else
        {
            highlightFrontIndex = -1;
            highlightBackIndex  = -1;
        }
    }
    else if (cursorLineIndex < numLines - 1)
    {
        //Go down a line
        cursorLineIndex++;
        cursorTextIndex = 0;
        //TODO: Handle highlighting
    }
}

void MoveCursorBackward()
{
    if (cursorTextIndex > 0)
    {
        cursorTextIndex--;
        if (InputHeld(input.leftShift))
        {
            Assert(!Xor(highlightFrontIndex == -1, highlightBackIndex == -1));
            if (highlightFrontIndex == highlightBackIndex) 
            {
                highlightBackIndex  = cursorTextIndex + 1;
                highlightFrontIndex = cursorTextIndex;
            }
            else
            { 
                int frontIndexDistFromCursor = abs(cursorTextIndex - highlightFrontIndex);
                int backIndexDistFromCursor =  abs(cursorTextIndex - highlightBackIndex);
				if (backIndexDistFromCursor <= frontIndexDistFromCursor ||
                    frontIndexDistFromCursor == 0)
					highlightBackIndex--;
				else
					highlightFrontIndex--;
            } 
        }
        else
        {
            highlightFrontIndex = -1;
            highlightBackIndex  = -1;
        }
    }
    else if (cursorLineIndex > 0)
    {
        //Go up a line
        cursorLineIndex--;
        cursorTextIndex = fileText[cursorLineIndex].len;
        //TODO: Handle highlighting
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

    line->len += (currentChar == '\t') ? 4 : 1;
    if (line->len >= line->size)
    {
        line->size *= 2;
        line->text = (char*)realloc(line->text, line->size);
    }
    //Shift right
    int offset = 4 * (currentChar == '\t');
    for (int i = line->len - 1; i > cursorTextIndex + offset; --i)
        line->text[i] = line->text[i-1];
    
    //Add character(s) and advance cursor
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

void RemoveChar()
{
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
    else if (numLines > 1)
    {
        //Move bottom line to top
        int prevLineIndex = cursorLineIndex;
        cursorLineIndex--;
        int copiedLen = fileText[prevLineIndex].len;
        int prevLen = fileText[cursorLineIndex].len;
        memcpy(
            fileText[cursorLineIndex].text + fileText[cursorLineIndex].len, 
            fileText[prevLineIndex].text, 
            copiedLen
        );
        fileText[prevLineIndex].len = 0;
        fileText[prevLineIndex].text[0] = 0;
        fileText[cursorLineIndex].len += copiedLen;
        fileText[cursorLineIndex].text[fileText[cursorLineIndex].len] = 0;

        numLines--;
        cursorTextIndex = prevLen;
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
        fileText[cursorLineIndex].text[copiedLen] = 0;
        
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
        cursorPos.x += fontChars[fileText[cursorLineIndex].text[i]].advance;
    cursorPos.y -= cursorLineIndex * CURSOR_HEIGHT;
    cursorPos.y -= 5; //Offset from baseline

    bool isWideChar = fontChars[currentChar].width > fontChars[currentChar].advance;
    int xRightLimit = screenBuffer.width - 10;
    if (cursorPos.x >= xRightLimit)
        textOffset.x = max(textOffset.x, cursorPos.x - xRightLimit); 
    else
        textOffset.x = 0;
	cursorPos.x -= textOffset.x;

    for (int i = 0; i < numLines; ++i)
    {
        //Draw text
        int x = start.x - textOffset.x;
        int y = start.y - i * CURSOR_HEIGHT - textOffset.y;
        DrawText(fileText[i].text, x, y, {0}, {start.x, xRightLimit, 0, 0});

        //Draw Line num
        char lineNumText[8];
        IntToString(i + 1, lineNumText);
        int lineNumOffset = (fontChars[' '].advance) * 4;
        DrawText(lineNumText, start.x - lineNumOffset, y, {255, 0, 0});
    }

    //Draw highlighted text
    if (highlightFrontIndex != -1 && highlightBackIndex != -1)
    {
        const int highlightedTextSize = highlightBackIndex - highlightFrontIndex;
        char* lineText = fileText[cursorLineIndex].text;
        int x = start.x + TextPixelLength(lineText, highlightFrontIndex) - textOffset.x;
        int y = cursorPos.y - cursorLineIndex * CURSOR_HEIGHT - textOffset.y;
        int xOffset = TextPixelLength(lineText + highlightFrontIndex, highlightedTextSize);
        DrawAlphaRect({x, x + xOffset, y, y + CURSOR_HEIGHT}, {0, 255, 0, 255 / 3});
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
