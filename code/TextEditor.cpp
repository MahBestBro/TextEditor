#include <windows.h> 

#include "TextEditor.h"
#include "TextEditor_input.h"

#define INITIAL_LINE_SIZE 256
#define CURSOR_HEIGHT 18
#define PIXELS_UNDER_BASELINE 5

extern Input input;
extern FontChar fontChars[128];
extern ScreenBuffer screenBuffer;

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

    int pixelRowStart = -min(0, rect.left - limits.left);
    int pixelColStart = -min(0, limits.top  - rect.top);

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
            byte* drawnPixel = pixels + stride * (y + pixelColStart) + x + pixelRowStart;
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
        FontChar fc = fontChars[at[0]];
        int xOffset = fc.left + xAdvance + xCoord;
        int yOffset = yCoord - (fc.height - fc.top) - yAdvance;
        Rect charDims = {xOffset, (int)(xOffset + fc.width), yOffset, (int)(yOffset + fc.height)};

        Draw8bppPixels(charDims, (byte*)fc.pixels, fc.width, colour, limits);

		xAdvance += fc.advance;
    }
}

global_variable Editor editor;

void MoveCursorForward()
{
    if (editor.cursorTextIndex < editor.lines[editor.cursorLineIndex].len)
    {  
        editor.cursorTextIndex++;
        IntRange* highlightRange = &editor.highlightRanges[editor.cursorLineIndex];
        if (InputHeld(input.leftShift))
        {
            if (highlightRange->low == highlightRange->high) 
            {
                if (highlightRange->low == -1)
                    editor.highlightedLineIndicies[editor.numHighlightedLines++] = 
                        editor.cursorLineIndex;
                highlightRange->low  = editor.cursorTextIndex - 1;
                highlightRange->high = editor.cursorTextIndex;
            }
            else
            { 
                int frontIndexDistFromCursor = abs(editor.cursorTextIndex - highlightRange->low);
                int backIndexDistFromCursor =  abs(editor.cursorTextIndex - highlightRange->high);
                if (frontIndexDistFromCursor <= backIndexDistFromCursor ||
                    backIndexDistFromCursor == 0)
					highlightRange->low++;
				else
					highlightRange->high++;
            }
        }
        else
        {
            for (int i = 0; i < editor.numHighlightedLines; ++i)
                editor.highlightRanges[editor.highlightedLineIndicies[i]] = {-1, -1};
            editor.numHighlightedLines = 0;
        }
    }
    else if (editor.cursorLineIndex < editor.numLines - 1)
    {
        //Go down a line
        editor.cursorLineIndex++;
        editor.cursorTextIndex = 0;
        //Handle highlighting
        IntRange* highlightRange = &editor.highlightRanges[editor.cursorLineIndex];
        if (InputHeld(input.leftShift))
        {
            if (highlightRange->low == -1)
            {
                Assert(highlightRange->high == -1);
                editor.highlightedLineIndicies[editor.numHighlightedLines++] = 
                    editor.cursorLineIndex;
                highlightRange->low  = editor.lines[editor.cursorLineIndex].len;
                highlightRange->high = editor.lines[editor.cursorLineIndex].len;
            }
            else
            {
                editor.numHighlightedLines--;
                editor.highlightRanges[editor.cursorLineIndex - 1] = {-1, -1};
            }
        }
        else
        {
            for (int i = 0; i < editor.numHighlightedLines; ++i)
                editor.highlightRanges[editor.highlightedLineIndicies[i]] = {-1, -1};
            editor.numHighlightedLines = 0;
        }
    }
}

void MoveCursorBackward()
{
    if (editor.cursorTextIndex > 0)
    {
        editor.cursorTextIndex--;
        //Handle Highlighting
        IntRange* highlightRange = &editor.highlightRanges[editor.cursorLineIndex];
        if (InputHeld(input.leftShift))
        {
            if (highlightRange->low == highlightRange->high) 
            {
                if (highlightRange->low == -1)
                    editor.highlightedLineIndicies[editor.numHighlightedLines++] = 
                        editor.cursorLineIndex;
                highlightRange->high = editor.cursorTextIndex + 1;
                highlightRange->low  = editor.cursorTextIndex;
            }
            else
            { 
                int frontIndexDistFromCursor = abs(editor.cursorTextIndex - highlightRange->low);
                int backIndexDistFromCursor =  abs(editor.cursorTextIndex - highlightRange->high);
				if (backIndexDistFromCursor <= frontIndexDistFromCursor ||
                    frontIndexDistFromCursor == 0)
					highlightRange->high--;
				else
					highlightRange->low--;
            } 
        }
        else
        {
            for (int i = 0; i < editor.numHighlightedLines; ++i)
                editor.highlightRanges[editor.highlightedLineIndicies[i]] = {-1, -1};
            editor.numHighlightedLines = 0;
        }
    }
    else if (editor.cursorLineIndex > 0)
    {
        //Go up a line
        editor.cursorLineIndex--;
        editor.cursorTextIndex = editor.lines[editor.cursorLineIndex].len;
        //Handle highlighting
        if (InputHeld(input.leftShift))
        {
            IntRange* highlightRange = &editor.highlightRanges[editor.cursorLineIndex];
            if (highlightRange->low == -1)
            {
                Assert(highlightRange->high == -1);
                editor.highlightedLineIndicies[editor.numHighlightedLines++] = 
                    editor.cursorLineIndex;
                highlightRange->low  = editor.lines[editor.cursorLineIndex].len;
                highlightRange->high = editor.lines[editor.cursorLineIndex].len;
            }
            else
            {
                editor.numHighlightedLines--;
                editor.highlightRanges[editor.cursorLineIndex + 1] = {-1, -1};
            }
        }
        else
        {
            for (int i = 0; i < editor.numHighlightedLines; ++i)
                editor.highlightRanges[editor.highlightedLineIndicies[i]] = {-1, -1};
            editor.numHighlightedLines = 0;
        }
    }
}

//TODO: When moving back to single line, move back to previous editor.cursorTextIndex
void MoveCursorUp()
{
    if (editor.cursorLineIndex > 0)
    {
        int prevTextIndex = editor.cursorTextIndex; 
        editor.cursorLineIndex--; 
        editor.cursorTextIndex = min(editor.cursorTextIndex, editor.lines[editor.cursorLineIndex].len);
        if (InputHeld(input.leftShift))
        {
            IntRange* upperHighlightRange = &editor.highlightRanges[editor.cursorLineIndex];
            IntRange* lowerHighlightRange = &editor.highlightRanges[editor.cursorLineIndex + 1];
            if (upperHighlightRange->low == -1)
            {
                Assert(upperHighlightRange->high == -1);
	    		if (lowerHighlightRange->low == -1)
                {
	    			editor.highlightedLineIndicies[editor.numHighlightedLines++] = 
                        editor.cursorLineIndex + 1;
                }
                editor.highlightedLineIndicies[editor.numHighlightedLines++] = 
                    editor.cursorLineIndex;
                upperHighlightRange->low  = editor.cursorTextIndex;
                upperHighlightRange->high = editor.lines[editor.cursorLineIndex].len;
                lowerHighlightRange->low  = 0;
                lowerHighlightRange->high = max(prevTextIndex, lowerHighlightRange->high);
            }
            else
            {
                editor.numHighlightedLines--;
                *lowerHighlightRange = {-1, -1};
                upperHighlightRange->high = editor.cursorTextIndex;
            }
        }
        else
        {
            for (int i = 0; i < editor.numHighlightedLines; ++i)
                editor.highlightRanges[editor.highlightedLineIndicies[i]] = {-1, -1};
            editor.numHighlightedLines = 0;
        }
    }
}

//TODO: When moving back to single line, move back to previous editor.cursorTextIndex
void MoveCursorDown()
{
    if (editor.cursorLineIndex < editor.numLines - 1)
    {
        editor.cursorLineIndex++;
        editor.cursorTextIndex = min(editor.cursorTextIndex, editor.lines[editor.cursorLineIndex].len);
        if (InputHeld(input.leftShift))
        {
            IntRange* upperHighlightRange = &editor.highlightRanges[editor.cursorLineIndex - 1];
            IntRange* lowerHighlightRange = &editor.highlightRanges[editor.cursorLineIndex];
            if (lowerHighlightRange->low == -1)
            {
                Assert(lowerHighlightRange->high == -1);
	    		if (upperHighlightRange->low == -1)
	    			editor.highlightedLineIndicies[editor.numHighlightedLines++] = 
                        editor.cursorLineIndex - 1;
                editor.highlightedLineIndicies[editor.numHighlightedLines++] = 
                    editor.cursorLineIndex;
                lowerHighlightRange->low  = 0;
                lowerHighlightRange->high = editor.cursorTextIndex;
                if (upperHighlightRange->low == -1) 
                    upperHighlightRange->low = editor.cursorTextIndex;
                upperHighlightRange->high = editor.lines[editor.cursorLineIndex - 1].len;
            }
            else
            {
                editor.numHighlightedLines--;
                *upperHighlightRange = {-1, -1};
                lowerHighlightRange->low = editor.cursorTextIndex;
            }
        }
        else
        {
            for (int i = 0; i < editor.numHighlightedLines; ++i)
                editor.highlightRanges[editor.highlightedLineIndicies[i]] = {-1, -1};
            editor.numHighlightedLines = 0;
        }
    }
}   

void AddChar()
{
    Line* line = &editor.lines[editor.cursorLineIndex];

    line->len += (editor.currentChar == '\t') ? 4 : 1;
    if (line->len >= line->size)
    {
        line->size *= 2;
        line->text = (char*)realloc(line->text, line->size);
    }
    //Shift right
    int offset = 4 * (editor.currentChar == '\t');
    for (int i = line->len - 1; i > editor.cursorTextIndex + offset; --i)
        line->text[i] = line->text[i-1];
    
    //Add character(s) and advance cursor
    char c = (editor.currentChar == '\t') ? ' ' : editor.currentChar;
    line->text[editor.cursorTextIndex] = c;
	line->text[line->len] = 0;
    editor.cursorTextIndex++;
    if (editor.currentChar == '\t')
    {
        for (int _ = 0; _ < 3; ++_)
        {
            line->text[editor.cursorTextIndex] = c;
            editor.cursorTextIndex++;
        }
    }
}

void ReplaceHighlightedText()
{
    //Move cursor and find top and bottom lines
    int lastLineIndex = 0, secondLastLineIndex = 0; 
    int bottomLineIndex = editor.cursorLineIndex;
    int topLineIndex = editor.cursorLineIndex; 
    if (editor.numHighlightedLines > 1)
    {
        lastLineIndex = editor.highlightedLineIndicies[editor.numHighlightedLines-1];
        secondLastLineIndex = editor.highlightedLineIndicies[editor.numHighlightedLines-2];
        const int topHighlightIndex = editor.highlightedLineIndicies[0];

        if (lastLineIndex > secondLastLineIndex || 
            editor.highlightRanges[topHighlightIndex].low != editor.cursorTextIndex)
        {   
            bottomLineIndex = lastLineIndex;
            topLineIndex = topHighlightIndex;
            editor.cursorTextIndex = editor.highlightRanges[topLineIndex].low;
            editor.cursorLineIndex = topLineIndex;
        }
        else
        {
            bottomLineIndex = editor.highlightedLineIndicies[0];
            topLineIndex = lastLineIndex;
        }
    }
    editor.cursorTextIndex++;

    //Get highlighted text on bottom line
    IntRange bottomHighlight = editor.highlightRanges[bottomLineIndex];
    const int remainingBottomLen = editor.lines[bottomLineIndex].len - bottomHighlight.high;
    char* remainingBottomText = (char*)malloc(remainingBottomLen + 1);
    memcpy(remainingBottomText, editor.lines[bottomLineIndex].text + bottomHighlight.high, 
           remainingBottomLen);
    remainingBottomText[remainingBottomLen] = 0;

    //Shift lines below highlited section up
	editor.numLines -= editor.numHighlightedLines - 1;
    for (int i = topLineIndex + 1; i < editor.numLines; ++i)
    {
        editor.lines[i] = editor.lines[i + editor.numHighlightedLines - 1];
    }

    //Remove Text from top line and connect bottom line text
    //TODO: realloc if not bottom line length > topLine.size
    IntRange topHighlight = editor.highlightRanges[topLineIndex];
    const int topRemovedLen = topHighlight.high - topHighlight.low;
    bool multipleHighlights = (editor.numHighlightedLines > 1);
    editor.lines[topLineIndex].len += remainingBottomLen * (multipleHighlights) - topRemovedLen+1;
    editor.lines[topLineIndex].text[topHighlight.low] = editor.currentChar;
    for (int i = 0; i < remainingBottomLen; ++i)
    {
        editor.lines[topLineIndex].text[i + topHighlight.low + 1] = remainingBottomText[i];
    }
    editor.lines[topLineIndex].text[editor.lines[topLineIndex].len] = 0;

    //Clear highlight ranges
    for (int i = 0; i < editor.numHighlightedLines; ++i)
        editor.highlightRanges[editor.highlightedLineIndicies[i]] = {-1, -1};
    editor.numHighlightedLines = 0; 
}

void RemoveChar()
{
    Line* line = &editor.lines[editor.cursorLineIndex];
    if (editor.cursorTextIndex > 0)
    {
		line->len--;
		for (int i = editor.cursorTextIndex - 1; i < line->len; ++i)
		{
			line->text[i] = line->text[i+1];
		}
        line->text[line->len] = 0;
        editor.cursorTextIndex--; 
    }
    else if (editor.numLines > 1 && editor.cursorLineIndex > 0)
    {
        //TODO: Prevent Overflow
        int copiedLen = editor.lines[editor.cursorLineIndex].len;
        Line* nextLine = &editor.lines[editor.cursorLineIndex - 1];
		editor.cursorTextIndex = nextLine->len;
        memcpy(
            nextLine->text + nextLine->len, 
            editor.lines[editor.cursorLineIndex].text, 
            copiedLen
        );
        nextLine->len += copiedLen;
        nextLine->text[nextLine->len] = 0;

		//Shift lines up
		for (int i = editor.cursorLineIndex; i < editor.numLines; ++i)
		{
			editor.lines[i] = editor.lines[i + 1];
		}
		editor.numLines--;

        editor.cursorLineIndex--;
    }
}

void RemoveHighlightedText()
{   
    //Move cursor and find top and bottom lines
    int lastLineIndex = 0, secondLastLineIndex = 0; 
    int bottomLineIndex = editor.cursorLineIndex;
    int topLineIndex = editor.cursorLineIndex; 
    if (editor.numHighlightedLines > 1)
    {
        lastLineIndex = editor.highlightedLineIndicies[editor.numHighlightedLines-1];
        secondLastLineIndex = editor.highlightedLineIndicies[editor.numHighlightedLines-2];
        const int topHighlightIndex = editor.highlightedLineIndicies[0];

        if (lastLineIndex > secondLastLineIndex || 
            editor.highlightRanges[topHighlightIndex].low != editor.cursorTextIndex)
        {
            bottomLineIndex = lastLineIndex;
            topLineIndex = topHighlightIndex;
            editor.cursorTextIndex = editor.highlightRanges[topLineIndex].low;
            editor.cursorLineIndex = topLineIndex;
        }
        else
        {
            bottomLineIndex = editor.highlightedLineIndicies[0];
            topLineIndex = lastLineIndex;
        }
    }

    Assert(editor.cursorTextIndex >= 0);
    

    //Get highlighted text on bottom line
    IntRange bottomHighlight = editor.highlightRanges[bottomLineIndex];
    const int remainingBottomLen = editor.lines[bottomLineIndex].len - bottomHighlight.high;
    char* remainingBottomText = (char*)malloc(remainingBottomLen + 1);
    memcpy(remainingBottomText, editor.lines[bottomLineIndex].text + bottomHighlight.high, 
           remainingBottomLen);
    remainingBottomText[remainingBottomLen] = 0;

    //Shift lines below highlited section up
	editor.numLines -= editor.numHighlightedLines - 1;
    Assert(editor.cursorLineIndex < editor.numLines);
    for (int i = topLineIndex + 1; i < editor.numLines; ++i)
    {
        editor.lines[i] = editor.lines[i + editor.numHighlightedLines - 1];
    }

    //Remove Text from top line and connect bottom line text
    //TODO: realloc if not bottom line length > topLine.size
    IntRange topHighlight = editor.highlightRanges[topLineIndex];
    const int topRemovedLen = topHighlight.high - topHighlight.low;
    bool multipleHighlights = (editor.numHighlightedLines > 1);
    editor.lines[topLineIndex].len += remainingBottomLen * (multipleHighlights) - topRemovedLen;
    for (int i = 0; i < remainingBottomLen; ++i)
    {
        editor.lines[topLineIndex].text[i + topHighlight.low] = remainingBottomText[i];
    }
    editor.lines[topLineIndex].text[editor.lines[topLineIndex].len] = 0;
    free(remainingBottomText);

    //Clear highlight ranges
    for (int i = 0; i < editor.numHighlightedLines; ++i)
        editor.highlightRanges[editor.highlightedLineIndicies[i]] = {-1, -1};
    editor.numHighlightedLines = 0;
}

void Backspace()
{
    if (editor.numHighlightedLines)
        RemoveHighlightedText();
    else
        RemoveChar();
}

void AddLine()
{
    if (editor.numHighlightedLines)
    {
        RemoveHighlightedText();
    }

    if (editor.numLines < MAX_LINES)
    {
        int prevLineIndex = editor.cursorLineIndex;
        editor.cursorLineIndex++;

        //Shift lines down
        for (int i = editor.numLines; i > editor.cursorLineIndex; --i)
        {
            editor.lines[i] = editor.lines[i-1];
        }
		editor.lines[editor.cursorLineIndex] = InitLine();
        editor.numLines++;
        
        int copiedLen = editor.lines[prevLineIndex].len - editor.cursorTextIndex;
        memcpy(
            editor.lines[editor.cursorLineIndex].text, 
            editor.lines[prevLineIndex].text + editor.cursorTextIndex, 
            copiedLen
        );
        editor.lines[prevLineIndex].len -= copiedLen;
        editor.lines[prevLineIndex].text[editor.cursorTextIndex] = 0;
        editor.lines[editor.cursorLineIndex].len = copiedLen;
        editor.lines[editor.cursorLineIndex].text[copiedLen] = 0;
        
        editor.cursorTextIndex = 0;
    }
}


TimedEvent cursorBlink = {0.5f};
TimedEvent holdChar = {0.5f};
TimedEvent repeatChar = {0.02f, &AddChar};

TimedEvent holdAction = {0.5f};
TimedEvent repeatAction = {0.02f};

bool capslockOn = false;
bool nonCharKeyPressed = false;

void Init()
{
    for (int i = 0; i < MAX_LINES; ++i)
    {
        editor.lines[i] = InitLine();
        editor.highlightRanges[i] = {-1, -1};
    }
}

void Draw(float dt)
{
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
                editor.currentChar = charOfKeyPressed;
                if (editor.numHighlightedLines)
                    ReplaceHighlightedText();
                else
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
            Backspace, 
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
    const IntPair start = {52, screenBuffer.height - CURSOR_HEIGHT}; 
    IntPair cursorPos = start;
    for (int i = 0; i < editor.cursorTextIndex; ++i)
        cursorPos.x += fontChars[editor.lines[editor.cursorLineIndex].text[i]].advance;
    cursorPos.y -= editor.cursorLineIndex * CURSOR_HEIGHT;
    cursorPos.y -= PIXELS_UNDER_BASELINE; 

    bool isWideChar = fontChars[editor.currentChar].width > fontChars[editor.currentChar].advance;
    int xRightLimit = screenBuffer.width - 10;
    if (cursorPos.x >= xRightLimit)
        editor.textOffset.x = max(editor.textOffset.x, cursorPos.x - xRightLimit); 
    else
        editor.textOffset.x = 0;
	cursorPos.x -= editor.textOffset.x;

    int yBottomLimit = CURSOR_HEIGHT;
    if (cursorPos.y < yBottomLimit)
        editor.textOffset.y = max(editor.textOffset.y, yBottomLimit - cursorPos.y);
    else
        editor.textOffset.y = 0;
    cursorPos.y += editor.textOffset.y;

    for (int i = 0; i < editor.numLines; ++i)
    {
        //Draw text
        int x = start.x - editor.textOffset.x;
        int y = start.y - i * CURSOR_HEIGHT + editor.textOffset.y;
        DrawText(editor.lines[i].text, x, y, {0}, {start.x, xRightLimit, yBottomLimit, 0});

        //Draw Line num
        char lineNumText[8];
        IntToString(i + 1, lineNumText);
        int lineNumOffset = (fontChars[' '].advance) * 4;
        DrawText(lineNumText, start.x - lineNumOffset, y, {255, 0, 0}, {0, 0, yBottomLimit, 0});
    }

    //Draw highlighted text
    for (int i = 0; i < editor.numHighlightedLines; ++i)
    {
        int l = editor.highlightedLineIndicies[i];
        const int textSize = editor.highlightRanges[l].high - editor.highlightRanges[l].low;
        char* lineText = editor.lines[l].text;
        int highlightedPixelLength = TextPixelLength(lineText, editor.highlightRanges[l].low); 
        int x = start.x + highlightedPixelLength - editor.textOffset.x;
        int y = start.y - l * CURSOR_HEIGHT - PIXELS_UNDER_BASELINE - editor.textOffset.y;
        int xOffset = TextPixelLength(lineText + editor.highlightRanges[l].low, textSize);
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
