#include <windows.h> 

#include "TextEditor.h"
#include "TextEditor_input.h"
#include "TextEditor_string.h"

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
    result.text = HeapAlloc(char, result.size);
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

//Can only think that these are the only 3 needed. Chnge this in the future?
enum InitialHeldKeys
{
    CTRL  = 0b001,
    ALT   = 0b010,
    SHIFT = 0b100
};

struct KeyCommand
{
    //initialHeldKeys: Bit flags that correspond to the initial keys to be held
    byte initialHeldKeys;  
    InputCode commandKey;
};

bool KeyCommandDown(KeyCommand keyCommand)
{
    Assert(keyCommand.initialHeldKeys);

    bool result = InputDown(input.flags[keyCommand.commandKey]);
    if ((keyCommand.initialHeldKeys & CTRL) == CTRL)
        result = result && InputHeld(input.leftCtrl);
    if ((keyCommand.initialHeldKeys & ALT) == ALT)
        result = result && InputHeld(input.leftAlt);
    if ((keyCommand.initialHeldKeys & SHIFT) == SHIFT)
        result = result && InputHeld(input.leftShift);
    
    return result;
}

#define QuickSort(arr, len, elSize) qsort(arr, len, elSize, _qsortCompare)

int _qsortCompare(const void* a, const void* b)
{
    return (*(int*)a - *(int*)b);
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

void ClearHighlights()
{
    editor.initialHighlightTextIndex = -1;
    editor.initialHighlightLineIndex = -1;
}

void InitHighlight(int initialTextIndex, int initialLineIndex)
{
    if (editor.initialHighlightTextIndex == -1)
    {
        Assert(editor.initialHighlightLineIndex == -1);
        editor.initialHighlightTextIndex = initialTextIndex;
        editor.initialHighlightLineIndex = initialLineIndex;
    }
}

//TODO: Have cursor 
void AdvanceCursorToEndOfWord(bool forward)
{
    Line line = editor.lines[editor.cursorLineIndex];
    bool skipNonAlphaNumericChars = true;
    do 
    {
        editor.cursorTextIndex += (forward) ? 1 : -1;
        skipNonAlphaNumericChars = IsAlphaNumeric(line.text[editor.cursorTextIndex - !forward]);
    }
    while (InRange(editor.cursorTextIndex, 1, line.len - 1) && 
           IsAlphaNumeric(line.text[editor.cursorTextIndex - !forward]) && 
           skipNonAlphaNumericChars);
}

HighlightInfo GetHighlightInfo()
{
    HighlightInfo result;
    if (editor.initialHighlightLineIndex > editor.cursorLineIndex)
    {
        result.topHighlightStart = editor.cursorTextIndex;
        result.topLine = editor.cursorLineIndex;
        result.bottomHighlightEnd = editor.initialHighlightTextIndex;
        result.bottomLine = editor.initialHighlightLineIndex;
        result.topHighlightLen = editor.lines[result.topLine].len - result.topHighlightStart;
    }
    else if (editor.initialHighlightLineIndex < editor.cursorLineIndex)
    {
        result.topHighlightStart = editor.initialHighlightTextIndex;
        result.topLine = editor.initialHighlightLineIndex;
        result.bottomHighlightEnd = editor.cursorTextIndex;
        result.bottomLine = editor.cursorLineIndex;
        result.topHighlightLen = editor.lines[result.topLine].len - result.topHighlightStart;
    }
    else
    {
        result.topHighlightStart = min(editor.initialHighlightTextIndex, editor.cursorTextIndex);
        result.topLine = editor.cursorLineIndex;
        result.bottomHighlightEnd = max(editor.initialHighlightTextIndex, editor.cursorTextIndex);
        result.bottomLine = result.topLine;
        result.spansOneLine = true;
        result.topHighlightLen = result.bottomHighlightEnd - result.topHighlightStart;
    }

    return result;
}

void MoveCursorForward()
{
    if (editor.cursorTextIndex < editor.lines[editor.cursorLineIndex].len)
    {
        int prevTextIndex = editor.cursorTextIndex;
        if (InputHeld(input.leftCtrl))
            AdvanceCursorToEndOfWord(true);
        else
            editor.cursorTextIndex++;

        if (InputHeld(input.leftShift))
            InitHighlight(prevTextIndex, editor.cursorLineIndex);
    }
    else if (editor.cursorLineIndex < editor.numLines - 1)
    {
        //Go down a line
        editor.cursorLineIndex++;
        editor.cursorTextIndex = 0;
        if (InputHeld(input.leftCtrl))
            AdvanceCursorToEndOfWord(true);

        if (InputHeld(input.leftShift))
            InitHighlight(editor.lines[editor.cursorLineIndex - 1].len, editor.cursorLineIndex - 1);
    }
}

void MoveCursorBackward()
{
    if (editor.cursorTextIndex > 0)
    {
        int prevTextIndex = editor.cursorTextIndex;
        if (InputHeld(input.leftCtrl))
            AdvanceCursorToEndOfWord(false);
        else
            editor.cursorTextIndex--;

        if (InputHeld(input.leftShift))
            InitHighlight(prevTextIndex, editor.cursorLineIndex);
    }
    else if (editor.cursorLineIndex > 0)
    {
        //Go up a line
        editor.cursorLineIndex--;
        editor.cursorTextIndex = editor.lines[editor.cursorLineIndex].len;
        if (InputHeld(input.leftCtrl))
            AdvanceCursorToEndOfWord(false);

        if (InputHeld(input.leftShift))
            InitHighlight(0, editor.cursorLineIndex + 1);
    }
}

//TODO: When moving back to single line whilst highlighting, move back to previous editor.cursorTextIndex
void MoveCursorUp()
{
    if (editor.cursorLineIndex > 0)
    {
        int prevTextIndex = editor.cursorTextIndex; 
        editor.cursorLineIndex--; 
        editor.cursorTextIndex = min(editor.cursorTextIndex, editor.lines[editor.cursorLineIndex].len);
        if (InputHeld(input.leftShift))
            InitHighlight(prevTextIndex, editor.cursorLineIndex + 1);
    }
}

//TODO: When moving back to single line, move back to previous editor.cursorTextIndex
void MoveCursorDown()
{
    if (editor.cursorLineIndex < editor.numLines - 1)
    {
        int prevTextIndex = editor.cursorTextIndex; 
        editor.cursorLineIndex++;
        editor.cursorTextIndex = min(editor.cursorTextIndex, editor.lines[editor.cursorLineIndex].len);
        if (InputHeld(input.leftShift))
            InitHighlight(prevTextIndex, editor.cursorLineIndex - 1);
    }
}   

void AddChar()
{
    Line* line = &editor.lines[editor.cursorLineIndex];

    int numCharsAdded = (editor.currentChar == '\t') ? 4 : 1; 
    line->len += numCharsAdded;
    if (line->len >= line->size)
    {
        line->size *= 2;
        line->text = (char*)realloc(line->text, line->size);
    }
    //Shift right
    int offset = 4 * (editor.currentChar == '\t');
    for (int i = line->len - 1; i > editor.cursorTextIndex + offset - 1; --i)
        line->text[i] = line->text[i - numCharsAdded];
    
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

    if (editor.topChangedLineIndex != -1)
        editor.topChangedLineIndex = min(editor.topChangedLineIndex, editor.cursorLineIndex);
    else 
        editor.topChangedLineIndex = editor.cursorLineIndex;
}

//TODO: Prevent Overflow
void InsertTextInLine(int lineIndex, char* text, int textStart)
{
    size_t textLen = StringLen(text);
    for (int i = 0; i < textLen; ++i)
    {
        int lineAt = textStart + i;
        editor.lines[lineIndex].text[lineAt + textLen] = editor.lines[lineIndex].text[lineAt];
        editor.lines[lineIndex].text[lineAt] = text[i];
    }
    int prevLen = editor.lines[lineIndex].len;
    editor.lines[lineIndex].len += (int)textLen;
    editor.lines[lineIndex].text[editor.lines[lineIndex].len] = 0;
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
        editor.cursorTextIndex = editor.lines[editor.cursorLineIndex - 1].len;
        InsertTextInLine(
            editor.cursorLineIndex - 1, 
            editor.lines[editor.cursorLineIndex].text, 
            editor.lines[editor.cursorLineIndex - 1].len
        );

		//Shift lines up
		for (int i = editor.cursorLineIndex; i < editor.numLines; ++i)
		{
			editor.lines[i] = editor.lines[i + 1];
		}
		editor.numLines--;

        editor.cursorLineIndex--;
    }

    if (editor.topChangedLineIndex != -1)
        editor.topChangedLineIndex = min(editor.topChangedLineIndex, editor.cursorLineIndex);
    else 
        editor.topChangedLineIndex = editor.cursorLineIndex;
}

void RemoveHighlightedText()
{   
    HighlightInfo highlightInfo = GetHighlightInfo();

    //Get highlighted text on bottom line
    char* remainingBottomText = nullptr;
    int remainingBottomLen = 0;
    if (!highlightInfo.spansOneLine)
    { 
        remainingBottomLen = 
            editor.lines[highlightInfo.bottomLine].len - highlightInfo.bottomHighlightEnd;
        remainingBottomText = HeapAlloc(char, remainingBottomLen + 1);
        memcpy(remainingBottomText, 
               editor.lines[highlightInfo.bottomLine].text + highlightInfo.bottomHighlightEnd, 
               remainingBottomLen);
        remainingBottomText[remainingBottomLen] = 0;
    }    

    //Shift lines below highlited section up
    editor.numLines -= highlightInfo.bottomLine - highlightInfo.topLine;
    for (int i = highlightInfo.topLine + 1; i < editor.numLines; ++i)
    {
        editor.lines[i] = editor.lines[i + highlightInfo.bottomLine - highlightInfo.topLine];
    }

    //Remove Text from top line and connect bottom line text
    //TODO: realloc if not bottom line length > topLine.size
    const int topRemovedLen = highlightInfo.topHighlightLen;
    memcpy(
        editor.lines[highlightInfo.topLine].text + highlightInfo.topHighlightStart, 
        editor.lines[highlightInfo.topLine].text + highlightInfo.topHighlightStart + topRemovedLen,
        editor.lines[highlightInfo.topLine].len - (highlightInfo.topHighlightStart + topRemovedLen)
    );
    editor.lines[highlightInfo.topLine].len += remainingBottomLen - topRemovedLen;
    memcpy(editor.lines[highlightInfo.topLine].text + highlightInfo.topHighlightStart, 
           remainingBottomText, 
           remainingBottomLen);
    editor.lines[highlightInfo.topLine].text[editor.lines[highlightInfo.topLine].len] = 0;

    editor.cursorLineIndex = highlightInfo.topLine;
	editor.cursorTextIndex = highlightInfo.topHighlightStart;
    if (editor.topChangedLineIndex != -1)
        editor.topChangedLineIndex = min(editor.topChangedLineIndex, editor.cursorLineIndex);
    else 
        editor.topChangedLineIndex = editor.cursorLineIndex;

	ClearHighlights();

	if (remainingBottomText) free(remainingBottomText);
}

void Backspace()
{
    if (editor.initialHighlightTextIndex != -1)
        RemoveHighlightedText();
    else
        RemoveChar();
}

//Inserts line without messing around with the cursor indicies
void InsertLineAt(int lineIndex)
{
    //Shift lines down
	editor.numLines++;
    for (int i = editor.numLines - 1; i > lineIndex; --i)
    {
        editor.lines[i] = editor.lines[i-1];
    }
	editor.lines[lineIndex] = InitLine();
}

void Enter()
{
    if (editor.initialHighlightTextIndex != -1)
    {
        RemoveHighlightedText();
    }

    if (editor.numLines < MAX_LINES)
    {
        int prevLineIndex = editor.cursorLineIndex;
        editor.cursorLineIndex++;

       InsertLineAt(editor.cursorLineIndex);
        
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

void HighlightEntireFile()
{
    editor.initialHighlightTextIndex = 0;
    editor.initialHighlightLineIndex = 0;
    editor.cursorTextIndex = editor.lines[editor.numLines - 1].len;
    editor.cursorLineIndex = editor.numLines - 1;
}

//TODO: Double check if this is susceptable to overflow attacks and Unix Compatability
void CopyHighlightedText()
{
    if (editor.initialHighlightTextIndex == -1) 
    {
        Assert(editor.initialHighlightLineIndex == -1);
        return;
    }

    HighlightInfo highlightInfo = GetHighlightInfo();

    size_t copySize = highlightInfo.topHighlightLen + 2 * (!highlightInfo.spansOneLine);
    for (int i = highlightInfo.topLine + 1; i < highlightInfo.bottomLine ; ++i)
    {
        copySize += editor.lines[i].len + 2;
    }
    if (!highlightInfo.spansOneLine) copySize += highlightInfo.bottomHighlightEnd;

    char* copiedText = HeapAlloc(char, copySize + 1);
    memcpy(copiedText, 
           editor.lines[highlightInfo.topLine].text + highlightInfo.topHighlightStart, 
           highlightInfo.topHighlightLen);
	int at = highlightInfo.topHighlightLen;
	if (!highlightInfo.spansOneLine)
	{
		copiedText[highlightInfo.topHighlightLen]     = '\r';
		copiedText[highlightInfo.topHighlightLen + 1] = '\n';
		at += 2;
	}
    for (int i = highlightInfo.topLine + 1; i < highlightInfo.bottomLine; ++i)
    {
        memcpy(copiedText + at, editor.lines[i].text, editor.lines[i].len);
        copiedText[at + editor.lines[i].len]     = '\r'; 
        copiedText[at + editor.lines[i].len + 1] = '\n'; 
        at += editor.lines[i].len + 2;
    }
    if (!highlightInfo.spansOneLine) 
        memcpy(copiedText + at, 
               editor.lines[highlightInfo.bottomLine].text, 
               highlightInfo.bottomHighlightEnd);
    copiedText[copySize] = 0;

    CopyToClipboard(copiedText, copySize);

    free(copiedText);
}

//TODO: handle replacing highlight and string overflow
void Paste()
{
    char* textToPaste = GetClipboardText();
    if (textToPaste != nullptr)
    {
        int numLinesToPaste = 0;
        char** linesToPaste = SplitStringByLines(textToPaste, &numLinesToPaste);
        free(textToPaste);

        //Set up the lines before adding text to prevent pushint down the same lines being added
        for (int i = 1; i < numLinesToPaste; ++i)
            InsertLineAt(editor.cursorLineIndex + i);

        //Paste text
        InsertTextInLine(editor.cursorLineIndex, linesToPaste[0], editor.cursorTextIndex);
        free(linesToPaste[0]);
        for (int i = 1; i < numLinesToPaste; ++i)
        {
            InsertTextInLine(editor.cursorLineIndex + i, linesToPaste[i], 0);
            free(linesToPaste[i]);
        }
        
        editor.cursorLineIndex += numLinesToPaste - 1;
        editor.cursorTextIndex = editor.lines[editor.cursorLineIndex].len;

        free(linesToPaste);
    }   
}

//TODO: Investigate Performance of this
void CutHighlightedText()
{
    CopyHighlightedText();
    RemoveHighlightedText();
    ClearHighlights();
}


//TODO: Resize editor.lines if file too big
void OpenFile()
{
    size_t fileNameLen = 0;
    char* fileName = ShowFileDialogAndGetFileName(false, &fileNameLen);
    char* file = ReadEntireFile(fileName);
    if (file)
    {
        editor.fileName = HeapAlloc(char, fileNameLen + 1);
        memcpy(editor.fileName, fileName, fileNameLen);
        editor.fileName[fileNameLen] = 0;

        int numLines = 0;
        char** fileLines = SplitStringByLines(file, &numLines);

        editor.numLines = numLines;
        for (int i = 0; i < numLines; ++i)
        {
            int lineLen = StringLen(fileLines[i]);
            memcpy(editor.lines[i].text, fileLines[i], lineLen);
            editor.lines[i].len = lineLen;
            editor.lines[i].text[lineLen] = 0;
            free(fileLines[i]);
        }

        free(fileLines);
        free(fileName);

        editor.topChangedLineIndex = -1;
    }
}

void SaveFile(char* fileName, size_t fileNameLen)
{
    if (editor.topChangedLineIndex == -1) return; 

	bool overwrite = fileName && editor.fileName && (strcmp(fileName, editor.fileName) == 0);
    
    size_t textToWriteLen = 0;
    int32 writeStart = 0;
    for (int i = 0; i < editor.numLines; ++i)
    {
        //TODO: Make UNIX compatible
        if (i >= editor.topChangedLineIndex)
            textToWriteLen += editor.lines[i].len + 2 * (i != editor.numLines - 1);
        else
        {
            Assert(i < editor.numLines - 1);
            writeStart += editor.lines[i].len + 2;
        }
    }

    char* textToWrite = HeapAlloc(char, textToWriteLen + 1);
    uint64 at = 0;
    for (int i = editor.topChangedLineIndex; i < editor.numLines; ++i)
    {
        memcpy(textToWrite + at, editor.lines[i].text, editor.lines[i].len);
        at += editor.lines[i].len;
        //TODO: Make UNIX compatible
        if (i < editor.numLines - 1)
        {
            textToWrite[at]     = '\r';
            textToWrite[at + 1] = '\n';
        }
        at += 2;
    }
    textToWrite[textToWriteLen] = 0;
    
    if(WriteToFile(fileName, textToWrite, textToWriteLen, overwrite, writeStart))
    {
        if (!overwrite)
        {
            editor.fileName = HeapRealloc(char, editor.fileName, fileNameLen + 1);
            memcpy(editor.fileName, fileName, fileNameLen);
            editor.fileName[fileNameLen] = 0;
        }
    }
    else
    {
        //Log
    }

    free(textToWrite);
    free(fileName);

    editor.topChangedLineIndex = -1;
}

void SaveAs()
{
	size_t fileNameLen = 0;
	char* fileName = ShowFileDialogAndGetFileName(true, &fileNameLen);
    SaveFile(fileName, fileNameLen);
}

void Save()
{
    if (editor.fileName)
		SaveFile(editor.fileName, StringLen(editor.fileName));
	else
    {
        Assert(editor.topChangedLineIndex != -1);
		SaveAs();
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
                if (editor.initialHighlightTextIndex != -1)
                    RemoveHighlightedText();
                AddChar();

                holdAction.elapsedTime = 0.0f;

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
            else if (InputUp(input.flags[code]))
            {
                newCharKeyPressed = true;
            }
        }
        else if (InputDown(input.flags[code]))
        {
            nonCharKeyPressed = true;
        }
        else if (InputHeld(input.leftCtrl))
        {
            break;
        }
        
    }
    
    if (charKeyPressed && !nonCharKeyPressed && !InputHeld(input.leftCtrl) && 
        InputHeld(input.flags[CharToInputCode(editor.currentChar)]))
    {
        if (newCharKeyPressed)
            holdChar.elapsedTime = 0.0f;
        HandleTimedEvent(&holdChar, dt, &repeatChar);

        if (!InputHeld(input.leftShift))
            ClearHighlights();
    }
    else
    {
        holdChar.elapsedTime = 0.0f;

        //TODO: Move these arrays out of loop at some point
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
            Enter, 
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

                //NOTE: This i < 2 check may get bad in the future
                if (i < 2 || !InputHeld(input.leftShift))
                    ClearHighlights();
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

        if (!inputHeld)
        {
            //TODO: Move these arrays out of loop at some point
            KeyCommand keyCommands[] =
            {
                {CTRL, INPUTCODE_A},
                {CTRL, INPUTCODE_C},
                {CTRL, INPUTCODE_O},
                {CTRL, INPUTCODE_S},
                {CTRL | SHIFT, INPUTCODE_S},
                {CTRL, INPUTCODE_V},
                {CTRL, INPUTCODE_X},
            };

            void (*keyCommandCallbacks[])(void) = 
            {
                HighlightEntireFile,
                CopyHighlightedText,
                OpenFile,
                Save,
                SaveAs,
                Paste,
                CutHighlightedText
            };

            for (int i = 0; i < ArrayLen(keyCommands); ++i)
            {
                if (KeyCommandDown(keyCommands[i]))
                    keyCommandCallbacks[i]();
            }
        }
        

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

    //All this shit here seems very dodgy, maybe refactor or just find a better method
    int xRightLimit = screenBuffer.width - 10;
    if (cursorPos.x >= xRightLimit)
        editor.textOffset.x = max(editor.textOffset.x, cursorPos.x - xRightLimit);
    else
        editor.textOffset.x = 0;

    char cursorChar = editor.lines[editor.cursorLineIndex].text[editor.cursorTextIndex];
    int xLeftLimit = start.x + editor.textOffset.x;
    if (cursorPos.x < xLeftLimit)
        editor.textOffset.x -= fontChars[cursorChar].advance;
	cursorPos.x -= editor.textOffset.x;

    int yBottomLimit = CURSOR_HEIGHT;
    if (cursorPos.y < yBottomLimit)
        editor.textOffset.y = max(editor.textOffset.y, yBottomLimit - cursorPos.y);
    else
        editor.textOffset.y = 0;
        
    int yTopLimit = start.y - editor.textOffset.y;
    if (cursorPos.y > yTopLimit)
        editor.textOffset.y -= CURSOR_HEIGHT;
    cursorPos.y += editor.textOffset.y;

    Rect textBounds = {start.x, xRightLimit, yBottomLimit, 0};

    for (int i = 0; i < editor.numLines; ++i)
    {
        //Draw text
        int x = start.x - editor.textOffset.x;
        int y = start.y - i * CURSOR_HEIGHT + editor.textOffset.y;
        DrawText(editor.lines[i].text, x, y, {0}, textBounds);

        //Draw Line num
        char lineNumText[8];
        IntToString(i + 1, lineNumText);
        int lineNumOffset = (fontChars[' '].advance) * 4;
        DrawText(lineNumText, start.x - lineNumOffset, y, {255, 0, 0}, {0, 0, yBottomLimit, 0});
    }

    //Draw highlighted text
    if (editor.initialHighlightTextIndex != -1) 
    {
        Assert(editor.initialHighlightLineIndex != -1);
        HighlightInfo highlightInfo = GetHighlightInfo();
        
        //Draw top line highlight
        char* topHighlightText = 
            editor.lines[highlightInfo.topLine].text + highlightInfo.topHighlightStart;;
        const int topHighlightPixelLength = 
            TextPixelLength(topHighlightText, highlightInfo.topHighlightLen);
		int topXOffset = 
            TextPixelLength(editor.lines[highlightInfo.topLine].text, highlightInfo.topHighlightStart);
        int topX = start.x + topXOffset - editor.textOffset.x;
        int topY = start.y - highlightInfo.topLine * CURSOR_HEIGHT - PIXELS_UNDER_BASELINE + 
                   editor.textOffset.y;
        if (editor.lines[highlightInfo.topLine].len == 0) topXOffset = fontChars[' '].advance;
        DrawAlphaRect(
            {topX, topX + topHighlightPixelLength, topY, topY + CURSOR_HEIGHT},
            {0, 255, 0, 255 / 3}, 
            textBounds
        );

        //Draw inbetween highlights
        for (int i = highlightInfo.topLine + 1; i < highlightInfo.bottomLine; ++i)
        {
            int highlightLen = editor.lines[i].len;
            char* lineText = editor.lines[i].text;
            int highlightedPixelLength = TextPixelLength(editor.lines[i].text, editor.lines[i].len); 
            if (editor.lines[i].len == 0) highlightedPixelLength = fontChars[' '].advance;
            int x = start.x - editor.textOffset.x;
            int y = start.y - i * CURSOR_HEIGHT - PIXELS_UNDER_BASELINE + editor.textOffset.y;
            DrawAlphaRect(
                {x, x + highlightedPixelLength, y, y + CURSOR_HEIGHT}, 
                {0, 255, 0, 255 / 3}, 
                textBounds
            );
        }

        //Draw bottom line highlight
        if (!highlightInfo.spansOneLine)
        {
            int bottomHighlightPixelLength = 
                TextPixelLength(editor.lines[highlightInfo.bottomLine].text, 
                                highlightInfo.bottomHighlightEnd);
            int bottomX = start.x - editor.textOffset.x;
            int bottomY = start.y - highlightInfo.bottomLine * CURSOR_HEIGHT 
                          - PIXELS_UNDER_BASELINE + editor.textOffset.y;
            if (editor.lines[highlightInfo.bottomLine].len == 0) 
                bottomHighlightPixelLength = fontChars[' '].advance;
            DrawAlphaRect(
                {bottomX, bottomX + bottomHighlightPixelLength, bottomY, bottomY + CURSOR_HEIGHT}, 
                {0, 255, 0, 255 / 3}, 
                textBounds
            );
        }
        
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
