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
    editor.highlightStart.textAt = -1;
    editor.highlightStart.line = -1;
}

void InitHighlight(int initialTextIndex, int initialLineIndex)
{
    if (editor.highlightStart.textAt == -1)
    {
        Assert(editor.highlightStart.line == -1);
        editor.highlightStart.textAt = initialTextIndex;
        editor.highlightStart.line = initialLineIndex;
    }
}

//TODO: Have cursor 
void AdvanceCursorToEndOfWord(bool forward)
{
    Line line = editor.lines[editor.cursorPos.line];
    bool skipNonAlphaNumericChars = true;
    do 
    {
        editor.cursorPos.textAt += (forward) ? 1 : -1;
        skipNonAlphaNumericChars = IsAlphaNumeric(line.text[editor.cursorPos.textAt - !forward]);
    }
    while (InRange(editor.cursorPos.textAt, 1, line.len - 1) && 
           IsAlphaNumeric(line.text[editor.cursorPos.textAt - !forward]) && 
           skipNonAlphaNumericChars);
}

TextSectionInfo GetHighlightInfo(EditorPos highlightStart, EditorPos highlightEnd)
{
    TextSectionInfo result;
    if (highlightStart.line > highlightEnd.line)
    {
        result.topTextStart = highlightEnd.textAt;
        result.topLine = highlightEnd.line;
        result.bottomTextEnd = highlightStart.textAt;
        result.bottomLine = highlightStart.line;
        result.topLen = editor.lines[result.topLine].len - result.topTextStart;
    }
    else if (highlightStart.line < highlightEnd.line)
    {
        result.topTextStart = highlightStart.textAt;
        result.topLine = highlightStart.line;
        result.bottomTextEnd = highlightEnd.textAt;
        result.bottomLine = highlightEnd.line;
        result.topLen = editor.lines[result.topLine].len - result.topTextStart;
    }
    else
    {
        result.topTextStart = min(highlightStart.textAt, highlightEnd.textAt);
        result.topLine = highlightEnd.line;
        result.bottomTextEnd = max(highlightStart.textAt, highlightEnd.textAt);
        result.bottomLine = result.topLine;
        result.spansOneLine = true;
        result.topLen = result.bottomTextEnd - result.topTextStart;
    }

    return result;
}

void MoveCursorForward()
{
    if (editor.cursorPos.textAt < editor.lines[editor.cursorPos.line].len)
    {
        int prevTextIndex = editor.cursorPos.textAt;
        if (InputHeld(input.leftCtrl))
            AdvanceCursorToEndOfWord(true);
        else
            editor.cursorPos.textAt++;

        if (InputHeld(input.leftShift))
            InitHighlight(prevTextIndex, editor.cursorPos.line);
    }
    else if (editor.cursorPos.line < editor.numLines - 1)
    {
        //Go down a line
        editor.cursorPos.line++;
        editor.cursorPos.textAt = 0;
        if (InputHeld(input.leftCtrl))
            AdvanceCursorToEndOfWord(true);

        if (InputHeld(input.leftShift))
            InitHighlight(editor.lines[editor.cursorPos.line - 1].len, editor.cursorPos.line - 1);
    }
}

void MoveCursorBackward()
{
    if (editor.cursorPos.textAt > 0)
    {
        int prevTextIndex = editor.cursorPos.textAt;
        if (InputHeld(input.leftCtrl))
            AdvanceCursorToEndOfWord(false);
        else
            editor.cursorPos.textAt--;

        if (InputHeld(input.leftShift))
            InitHighlight(prevTextIndex, editor.cursorPos.line);
    }
    else if (editor.cursorPos.line > 0)
    {
        //Go up a line
        editor.cursorPos.line--;
        editor.cursorPos.textAt = editor.lines[editor.cursorPos.line].len;
        if (InputHeld(input.leftCtrl))
            AdvanceCursorToEndOfWord(false);

        if (InputHeld(input.leftShift))
            InitHighlight(0, editor.cursorPos.line + 1);
    }
}

//TODO: When moving back to single line whilst highlighting, move back to previous editor.cursorPos.textAt
void MoveCursorUp()
{
    if (editor.cursorPos.line > 0)
    {
        int prevTextIndex = editor.cursorPos.textAt; 
        editor.cursorPos.line--; 
        editor.cursorPos.textAt = min(editor.cursorPos.textAt, editor.lines[editor.cursorPos.line].len);
        if (InputHeld(input.leftShift))
            InitHighlight(prevTextIndex, editor.cursorPos.line + 1);
    }
}

//TODO: When moving back to single line, move back to previous editor.cursorPos.textAt
void MoveCursorDown()
{
    if (editor.cursorPos.line < editor.numLines - 1)
    {
        int prevTextIndex = editor.cursorPos.textAt; 
        editor.cursorPos.line++;
        editor.cursorPos.textAt = min(editor.cursorPos.textAt, editor.lines[editor.cursorPos.line].len);
        if (InputHeld(input.leftShift))
            InitHighlight(prevTextIndex, editor.cursorPos.line - 1);
    }
}

char** GetTextByLines(TextSectionInfo sectionInfo)
{
    const int numLines = sectionInfo.bottomLine - sectionInfo.topLine + 1;
    char** result = HeapAlloc(char*, numLines); 
    result[0] = HeapAlloc(char, sectionInfo.topLen + 1);
	memcpy(result[0], 
		   editor.lines[sectionInfo.topLine].text + sectionInfo.topTextStart,
		   sectionInfo.topLen);
	result[0][sectionInfo.topLen] = 0;
    for (int i = 1; i < numLines - 1; ++i)
    {
        int l = i + sectionInfo.topLine;
        result[i] = HeapAlloc(char, editor.lines[l].len + 1);
        memcpy(result[i], editor.lines[l].text, editor.lines[l].len);
		result[i][editor.lines[l].len] = 0;
    }
    if (!sectionInfo.spansOneLine)
    {
        result[numLines - 1] = HeapAlloc(char, sectionInfo.bottomTextEnd + 1);
        memcpy(result[numLines - 1], 
               editor.lines[sectionInfo.bottomLine].text, 
               sectionInfo.bottomTextEnd);
		result[numLines - 1][sectionInfo.bottomTextEnd] = 0;
    }

    return result;
}

UndoInfo InitUndoInfo(EditorPos undoStart, EditorPos undoEnd, UndoType type, bool wasHighlight)
{
    UndoInfo result;
    result.undoStart = undoStart;
    result.undoEnd = undoEnd;
    result.textByLine = GetTextByLines(GetHighlightInfo(undoStart, undoEnd));
    result.type = type;
    result.wasHighlight = wasHighlight;
    return result;
}



void AddChar()
{
    int numCharsAdded = (editor.currentChar == '\t') ? 4 : 1; 
    EditorPos nextPos = {editor.cursorPos.textAt + numCharsAdded, editor.cursorPos.line};
    UndoInfo* currentUndo = &editor.undoStack[editor.numUndos];

    if (editor.currentChar == ' ' || editor.currentChar == '\t' || 
        currentUndo->type != UNDOTYPE_ADDED_TEXT || editor.cursorPos != currentUndo->undoEnd)
    {
        editor.undoStack[++editor.numUndos] = InitUndoInfo(editor.cursorPos, 
                                                           nextPos, 
                                                           UNDOTYPE_ADDED_TEXT,
                                                           false);
    }
    else
    {
        currentUndo->undoEnd = nextPos;
    }


    Line* line = &editor.lines[editor.cursorPos.line];

    line->len += numCharsAdded;
    if (line->len >= line->size)
    {
        line->size *= 2;
        line->text = (char*)realloc(line->text, line->size);
    }
    //Shift right
    int offset = 4 * (editor.currentChar == '\t');
    for (int i = line->len - 1; i > editor.cursorPos.textAt + offset - 1; --i)
        line->text[i] = line->text[i - numCharsAdded];
    
    //Add character(s) and advance cursor
    char c = (editor.currentChar == '\t') ? ' ' : editor.currentChar;
    line->text[editor.cursorPos.textAt] = c;
	line->text[line->len] = 0;
    editor.cursorPos.textAt++;
    if (editor.currentChar == '\t')
    {
        for (int _ = 0; _ < 3; ++_)
        {
            line->text[editor.cursorPos.textAt] = c;
            editor.cursorPos.textAt++;
        }
    }

    if (editor.topChangedLineIndex != -1)
        editor.topChangedLineIndex = min(editor.topChangedLineIndex, editor.cursorPos.line);
    else 
        editor.topChangedLineIndex = editor.cursorPos.line;
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
    UndoInfo* currentUndo = &editor.undoStack[editor.numUndos];

    if (currentUndo->type != UNDOTYPE_REMOVED_TEXT_REVERSE_BUFFER || 
             editor.cursorPos != currentUndo->undoEnd)
    {
        editor.undoStack[++editor.numUndos] = InitUndoInfo(editor.cursorPos, 
                                                           editor.cursorPos, 
                                                           UNDOTYPE_REMOVED_TEXT_REVERSE_BUFFER,
                                                           false);
    }
    else
    {
        //Append to undo text buffer
    }

    Line* line = &editor.lines[editor.cursorPos.line];
    if (editor.cursorPos.textAt > 0)
    {
		line->len--;
		for (int i = editor.cursorPos.textAt - 1; i < line->len; ++i)
		{
			line->text[i] = line->text[i+1];
		}
        line->text[line->len] = 0;
        editor.cursorPos.textAt--; 
    }
    else if (editor.numLines > 1 && editor.cursorPos.line > 0)
    {
        editor.cursorPos.textAt = editor.lines[editor.cursorPos.line - 1].len;
        InsertTextInLine(
            editor.cursorPos.line - 1, 
            editor.lines[editor.cursorPos.line].text, 
            editor.lines[editor.cursorPos.line - 1].len
        );

		//Shift lines up
		for (int i = editor.cursorPos.line; i < editor.numLines; ++i)
		{
			editor.lines[i] = editor.lines[i + 1];
		}
		editor.numLines--;

        editor.cursorPos.line--;
    }

    //NOTE: This feels is too simple to be good
    editor.undoStack[editor.numUndos].undoEnd = editor.cursorPos;

    //update undo info
    if (editor.topChangedLineIndex != -1)
        editor.topChangedLineIndex = min(editor.topChangedLineIndex, editor.cursorPos.line);
    else 
        editor.topChangedLineIndex = editor.cursorPos.line;
}

void RemoveTextSection(TextSectionInfo sectionInfo)
{   
    //Get highlighted text on bottom line
    char* remainingBottomText = nullptr;
    int remainingBottomLen = 0;
    if (!sectionInfo.spansOneLine)
    { 
        remainingBottomLen = 
            editor.lines[sectionInfo.bottomLine].len - sectionInfo.bottomTextEnd;
        remainingBottomText = HeapAlloc(char, remainingBottomLen + 1);
        memcpy(remainingBottomText, 
               editor.lines[sectionInfo.bottomLine].text + sectionInfo.bottomTextEnd, 
               remainingBottomLen);
        remainingBottomText[remainingBottomLen] = 0;
    }    

    //Shift lines below highlited section up
    editor.numLines -= sectionInfo.bottomLine - sectionInfo.topLine;
    for (int i = sectionInfo.topLine + 1; i < editor.numLines; ++i)
    {
        editor.lines[i] = editor.lines[i + sectionInfo.bottomLine - sectionInfo.topLine];
    }

    //Remove Text from top line and connect bottom line text
    //TODO: realloc if not bottom line length > topLine.size
    const int topRemovedLen = sectionInfo.topLen;
    memcpy(
        editor.lines[sectionInfo.topLine].text + sectionInfo.topTextStart, 
        editor.lines[sectionInfo.topLine].text + sectionInfo.topTextStart + topRemovedLen,
        editor.lines[sectionInfo.topLine].len - (sectionInfo.topTextStart + topRemovedLen)
    );
    editor.lines[sectionInfo.topLine].len += remainingBottomLen - topRemovedLen;
    memcpy(editor.lines[sectionInfo.topLine].text + sectionInfo.topTextStart, 
           remainingBottomText, 
           remainingBottomLen);
    editor.lines[sectionInfo.topLine].text[editor.lines[sectionInfo.topLine].len] = 0;

    editor.cursorPos = {sectionInfo.topTextStart, sectionInfo.topLine};
    if (editor.topChangedLineIndex != -1)
        editor.topChangedLineIndex = min(editor.topChangedLineIndex, editor.cursorPos.line);
    else 
        editor.topChangedLineIndex = editor.cursorPos.line;

	if (remainingBottomText) free(remainingBottomText);
}

void Backspace()
{
    bool deletingHighlightedText = editor.highlightStart.textAt != -1;
    UndoType undoType = (deletingHighlightedText) ? 
                        UNDOTYPE_REMOVED_TEXT_SECTION : UNDOTYPE_REMOVED_TEXT_REVERSE_BUFFER;
    

    if (editor.highlightStart.textAt != -1)
    {
        editor.undoStack[++editor.numUndos] = InitUndoInfo(editor.highlightStart, 
                                                           editor.cursorPos, 
                                                           UNDOTYPE_REMOVED_TEXT_SECTION,
                                                           true);
        TextSectionInfo highlightInfo = GetHighlightInfo(editor.highlightStart, editor.cursorPos);
        editor.undoStack[editor.numUndos].textByLine = GetTextByLines(highlightInfo);
        
        RemoveTextSection(highlightInfo);
        ClearHighlights();
    }
    else
    {
        RemoveChar();
    }
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
    if (editor.numLines < MAX_LINES)
    { 
        editor.undoStack[++editor.numUndos] = InitUndoInfo(editor.cursorPos,
                                                           editor.cursorPos,
                                                           UNDOTYPE_ADDED_TEXT,
                                                           editor.highlightStart.textAt != -1);
        if (editor.highlightStart.textAt != -1)
        {
            TextSectionInfo highlightInfo = GetHighlightInfo(editor.highlightStart, 
                                                             editor.cursorPos);
	    	editor.undoStack[editor.numUndos].type = UNDOTYPE_OVERWRITE;
            editor.undoStack[editor.numUndos].textByLine = GetTextByLines(highlightInfo);

            RemoveTextSection(highlightInfo);
        }

        int prevLineIndex = editor.cursorPos.line;
        editor.cursorPos.line++;

        InsertLineAt(editor.cursorPos.line);
        
        int copiedLen = editor.lines[prevLineIndex].len - editor.cursorPos.textAt;
        memcpy(
            editor.lines[editor.cursorPos.line].text, 
            editor.lines[prevLineIndex].text + editor.cursorPos.textAt, 
            copiedLen
        );
        editor.lines[prevLineIndex].len -= copiedLen;
        editor.lines[prevLineIndex].text[editor.cursorPos.textAt] = 0;
        editor.lines[editor.cursorPos.line].len = copiedLen;
        editor.lines[editor.cursorPos.line].text[copiedLen] = 0;

        editor.cursorPos.textAt = 0;

        editor.undoStack[editor.numUndos].undoEnd = editor.cursorPos;
    }

	
}

void HighlightEntireFile()
{
    editor.highlightStart.textAt = 0;
    editor.highlightStart.line = 0;
    editor.cursorPos.textAt = editor.lines[editor.numLines - 1].len;
    editor.cursorPos.line = editor.numLines - 1;
}

//TODO: Double check if this is susceptable to overflow attacks and Unix Compatability
void CopyHighlightedText()
{
    if (editor.highlightStart.textAt == -1) 
    {
        Assert(editor.highlightStart.line == -1);
        return;
    }

    TextSectionInfo highlightInfo = GetHighlightInfo(editor.highlightStart, editor.cursorPos);

    size_t copySize = highlightInfo.topLen + 2 * (!highlightInfo.spansOneLine);
    for (int i = highlightInfo.topLine + 1; i < highlightInfo.bottomLine ; ++i)
    {
        copySize += editor.lines[i].len + 2;
    }
    if (!highlightInfo.spansOneLine) copySize += highlightInfo.bottomTextEnd;

    char* copiedText = HeapAlloc(char, copySize + 1);
    memcpy(copiedText, 
           editor.lines[highlightInfo.topLine].text + highlightInfo.topTextStart, 
           highlightInfo.topLen);
	int at = highlightInfo.topLen;
	if (!highlightInfo.spansOneLine)
	{
		copiedText[highlightInfo.topLen]     = '\r';
		copiedText[highlightInfo.topLen + 1] = '\n';
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
               highlightInfo.bottomTextEnd);
    copiedText[copySize] = 0;

    CopyToClipboard(copiedText, copySize);

    free(copiedText);
}

void InsertText(char** textAslines, TextSectionInfo sectionInfo)
{
    const int numLines = sectionInfo.bottomLine - sectionInfo.topLine;
    //Set up the lines before adding text to prevent pushing down the same lines being added
    for (int i = 1; i < numLines; ++i)
        InsertLineAt(sectionInfo.topLine + i);

    //Paste text
    InsertTextInLine(sectionInfo.topLine, textAslines[0], sectionInfo.topTextStart);
    for (int i = 1; i < numLines; ++i)
    {
        InsertTextInLine(sectionInfo.topLine + i, textAslines[i], 0);
    }
    
    editor.cursorPos.line += numLines - 1;
    editor.cursorPos.textAt = editor.lines[editor.cursorPos.line].len;
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

        TextSectionInfo sectionInfo;
        sectionInfo.topTextStart = editor.cursorPos.textAt;
        sectionInfo.topLen = StringLen(linesToPaste[0]);
        sectionInfo.topLine = editor.cursorPos.line;
        sectionInfo.bottomTextEnd = StringLen(linesToPaste[numLinesToPaste - 1]);
        sectionInfo.bottomTextEnd = editor.cursorPos.line + numLinesToPaste - 1;
        sectionInfo.spansOneLine = numLinesToPaste == 1;
        InsertText(linesToPaste, sectionInfo);

        for (int i = 0; i < numLinesToPaste; ++i)
            free(linesToPaste[i]);
        free(linesToPaste);
    }   
}

//TODO: Investigate Performance of this
void CutHighlightedText()
{
    CopyHighlightedText();
    RemoveTextSection(GetHighlightInfo(editor.highlightStart, editor.cursorPos));
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

void Undo()
{
    if (editor.lastActionWasUndo && editor.numUndos == 0) return;

    editor.numUndos -= (editor.lastActionWasUndo);
    UndoInfo undoInfo = editor.undoStack[editor.numUndos];
    TextSectionInfo sectionInfo = GetHighlightInfo(undoInfo.undoStart, undoInfo.undoEnd);

    switch(undoInfo.type)
    {
        case UNDOTYPE_ADDED_TEXT:
            RemoveTextSection(sectionInfo);
            break;

        case UNDOTYPE_REMOVED_TEXT_SECTION:
            InsertText(undoInfo.textByLine, sectionInfo);
            break;

        case UNDOTYPE_REMOVED_TEXT_REVERSE_BUFFER:
            UNIMPLEMENTED("This is for when user is backspacing non highlighted text");
            break;

        case UNDOTYPE_OVERWRITE:
            UNIMPLEMENTED("TODO: Handle undo for all instances of overwriting text");
            break;
    }
    editor.cursorPos = undoInfo.undoStart;
    editor.undoStack[editor.numUndos].undoEnd = undoInfo.undoStart; //This is hacky af but it works? 
    editor.lastActionWasUndo = true;
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
                char charOfKeyPressed = InputCodeToChar((InputCode)code, 
                                                        InputHeld(input.leftShift), 
                                                        capslockOn);
                editor.currentChar = charOfKeyPressed;
                if (editor.highlightStart.textAt != -1)
                    RemoveTextSection(GetHighlightInfo(editor.highlightStart, editor.cursorPos));
                AddChar();

                holdAction.elapsedTime = 0.0f;

                editor.lastActionWasUndo = false;

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
            editor.lastActionWasUndo = false;
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
                {
                    ClearHighlights();
                }
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
                {CTRL, INPUTCODE_Z},
            };

            void (*keyCommandCallbacks[])(void) = 
            {
                HighlightEntireFile,
                CopyHighlightedText,
                OpenFile,
                Save,
                SaveAs,
                Paste,
                CutHighlightedText,
                Undo
            };

            for (int i = 0; i < ArrayLen(keyCommands); ++i)
            {
                if (KeyCommandDown(keyCommands[i]))
                {
                    keyCommandCallbacks[i]();
                }
            }
        }
        

    }

    //Draw Background
    Rect screenDims = {0, screenBuffer.width, 0, screenBuffer.height};
    DrawRect(screenDims, {255, 255, 255});
    
    //Get correct position for cursor
    const IntPair start = {52, screenBuffer.height - CURSOR_HEIGHT}; 
    IntPair cursorDrawPos = start;
    for (int i = 0; i < editor.cursorPos.textAt; ++i)
        cursorDrawPos.x += fontChars[editor.lines[editor.cursorPos.line].text[i]].advance;
    cursorDrawPos.y -= editor.cursorPos.line * CURSOR_HEIGHT;
    cursorDrawPos.y -= PIXELS_UNDER_BASELINE; 

    //All this shit here seems very dodgy, maybe refactor or just find a better method
    int xRightLimit = screenBuffer.width - 10;
    if (cursorDrawPos.x >= xRightLimit)
        editor.textOffset.x = max(editor.textOffset.x, cursorDrawPos.x - xRightLimit);
    else
        editor.textOffset.x = 0;

    char cursorChar = editor.lines[editor.cursorPos.line].text[editor.cursorPos.textAt];
    int xLeftLimit = start.x + editor.textOffset.x;
    if (cursorDrawPos.x < xLeftLimit)
        editor.textOffset.x -= fontChars[cursorChar].advance;
	cursorDrawPos.x -= editor.textOffset.x;

    int yBottomLimit = CURSOR_HEIGHT;
    if (cursorDrawPos.y < yBottomLimit)
        editor.textOffset.y = max(editor.textOffset.y, yBottomLimit - cursorDrawPos.y);
    else
        editor.textOffset.y = 0;
        
    int yTopLimit = start.y - editor.textOffset.y;
    if (cursorDrawPos.y > yTopLimit)
        editor.textOffset.y -= CURSOR_HEIGHT;
    cursorDrawPos.y += editor.textOffset.y;

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
    if (editor.highlightStart.textAt != -1) 
    {
        Assert(editor.highlightStart.line != -1);
        TextSectionInfo highlightInfo = GetHighlightInfo(editor.highlightStart, editor.cursorPos);
        
        //Draw top line highlight
        char* topHighlightText = 
            editor.lines[highlightInfo.topLine].text + highlightInfo.topTextStart;
        const int topHighlightPixelLength = 
            TextPixelLength(topHighlightText, highlightInfo.topLen);
		int topXOffset = 
            TextPixelLength(editor.lines[highlightInfo.topLine].text, highlightInfo.topTextStart);
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
                                highlightInfo.bottomTextEnd);
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
        Rect cursorDims = {cursorDrawPos.x, cursorDrawPos.x + CURSOR_WIDTH, 
                           cursorDrawPos.y, cursorDrawPos.y + CURSOR_HEIGHT};
        DrawRect(cursorDims, {0, 255, 0});
    }
}
