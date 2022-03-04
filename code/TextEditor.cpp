#include <windows.h>

#include "TextEditor.h"
#include "TextEditor_input.h"
#include "TextEditor_string.h"
#include "TextEditor_config.h"

#define TEXT_X_OFFSET 52
#define CURSOR_HEIGHT 18
#define PIXELS_UNDER_BASELINE 5

extern Input input;
extern FontChar fontChars[128];
extern ScreenBuffer screenBuffer;

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
	if (InputDown(input.letterKeys['L' - 'A']))
	{
		Assert(true);
	}

    Assert(keyCommand.initialHeldKeys);

    bool commandKeyDown = InputDown(input.flags[keyCommand.commandKey]);
    
    byte heldInitialKeys = 0b000;
    heldInitialKeys |= (byte)InputHeld(input.leftCtrl);
    heldInitialKeys |= (byte)InputHeld(input.leftAlt) << 1;
    heldInitialKeys |= (byte)InputHeld(input.leftShift) << 2;
    
    return commandKeyDown && (heldInitialKeys == keyCommand.initialHeldKeys);
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

//
//DRAWING FUNCTIONS
//

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
            
            byte* greyScalePixel = pixels + stride * (y + pixelColStart) + x + pixelRowStart;
            
            byte scaledR =  (byte)(*greyScalePixel / 255.0f * colour.r);
            byte scaledB =  (byte)(*greyScalePixel / 255.0f * colour.b);
            byte scaledG =  (byte)(*greyScalePixel / 255.0f * colour.g);
            
            float alphaA = *greyScalePixel / 255.f;
            float alphaB = 1.f;
            float drawnAlpha = alphaA + alphaB * (1.f - alphaA);
            byte pixelB = pixel[0];
            byte pixelG = pixel[1];
            byte pixelR = pixel[2];
            byte drawnR = (byte)((scaledR*alphaA + pixelR*alphaB*(1 - alphaA)) / drawnAlpha);
            byte drawnG = (byte)((scaledG*alphaA + pixelG*alphaB*(1 - alphaA)) / drawnAlpha);
            byte drawnB = (byte)((scaledB*alphaA + pixelB*alphaB*(1 - alphaA)) / drawnAlpha);
            
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

//Consider making this non c-string dependent
//TE because DrawText already in windows.h hnnnngh
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

//
//EDITOR HELPER FUNCTIONS
//

global_variable Editor editor;

void InitHighlight(int initialTextIndex, int initialLineIndex)
{
    if (editor.highlightStart.textAt == -1)
    {
        Assert(editor.highlightStart.line == -1);
        editor.highlightStart.textAt = initialTextIndex;
        editor.highlightStart.line = initialLineIndex;
    }
}

void ClearHighlights()
{
    editor.highlightStart.textAt = -1;
    editor.highlightStart.line = -1;
}

void AdvanceCursorToEndOfWord(bool forward)
{
    Line line = editor.lines[editor.cursorPos.line];
    
    bool (*ShouldAdvance)(char) = nullptr;
    bool skipOverSpace = true;
    do 
    {
        editor.cursorPos.textAt += (forward) ? 1 : -1;
        if (!IsInvisChar(line.text[editor.cursorPos.textAt - !forward]))
            skipOverSpace = false;
        
        if (!skipOverSpace)
        {
            ShouldAdvance = (IsPunctuation(line.text[editor.cursorPos.textAt - !forward])) ?
                             IsPunctuation : IsAlphaNumeric;
        }
    }
    while (InRange(editor.cursorPos.textAt, 1, line.len - 1) && 
          (skipOverSpace || ShouldAdvance(line.text[editor.cursorPos.textAt - !forward])));
}

TextSectionInfo GetTextSectionInfo(EditorPos start, EditorPos end)
{
    TextSectionInfo result;
    if (start.line > end.line)
    {
        result.top.textAt = end.textAt;
        result.top.line = end.line;
        result.bottom.textAt = start.textAt;
        result.bottom.line = start.line;
        result.topLen = editor.lines[result.top.line].len - result.top.textAt;
    }
    else if (start.line < end.line)
    {
        result.top.textAt = start.textAt;
        result.top.line = start.line;
        result.bottom.textAt = end.textAt;
        result.bottom.line = end.line;
        result.topLen = editor.lines[result.top.line].len - result.top.textAt;
    }
    else
    {
        result.top.textAt = min(start.textAt, end.textAt);
        result.top.line = end.line;
        result.bottom.textAt = max(start.textAt, end.textAt);
        result.bottom.line = result.top.line;
        result.spansOneLine = true;
        result.topLen = result.bottom.textAt - result.top.textAt;
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

void SetTopChangedLine(int newLineIndex)
{
    if (editor.topChangedLineIndex != -1)
        editor.topChangedLineIndex = min(editor.topChangedLineIndex, newLineIndex);
    else 
        editor.topChangedLineIndex = editor.cursorPos.line;
}

char** GetTextByLines(TextSectionInfo sectionInfo)
{
    const int numLines = sectionInfo.bottom.line - sectionInfo.top.line + 1;
    char** result = HeapAlloc(char*, numLines); 
    result[0] = HeapAlloc(char, sectionInfo.topLen + 1);
	memcpy(result[0], 
		   editor.lines[sectionInfo.top.line].text + sectionInfo.top.textAt,
		   sectionInfo.topLen);
	result[0][sectionInfo.topLen] = 0;
    for (int i = 1; i < numLines - 1; ++i)
    {
        int l = i + sectionInfo.top.line;
        result[i] = HeapAlloc(char, editor.lines[l].len + 1);
        memcpy(result[i], editor.lines[l].text, editor.lines[l].len);
		result[i][editor.lines[l].len] = 0;
    }
    if (!sectionInfo.spansOneLine)
    {
        result[numLines - 1] = HeapAlloc(char, sectionInfo.bottom.textAt + 1);
        memcpy(result[numLines - 1], 
               editor.lines[sectionInfo.bottom.line].text, 
               sectionInfo.bottom.textAt);
		result[numLines - 1][sectionInfo.bottom.textAt] = 0;
    }

    return result;
}

void AppendTextToLine(int lineIndex, char* text)
{
	int appendStart = editor.lines[lineIndex].len;
    size_t textLen = StringLen(text);

    editor.lines[lineIndex].len += (int)textLen;
    ResizeDynamicArray(&editor.lines[lineIndex].text, 
                       editor.lines[lineIndex].len, 
                       sizeof(char), 
					   &editor.lines[lineIndex].size);
    
    memcpy(editor.lines[lineIndex].text + appendStart, text, textLen);
    editor.lines[lineIndex].text[editor.lines[lineIndex].len] = 0;
}

void RemoveTextSection(TextSectionInfo sectionInfo)
{   
    //Get highlighted text on bottom line
    char* remainingBottomText = nullptr;
    int remainingBottomLen = 0;
    if (!sectionInfo.spansOneLine)
    { 
        remainingBottomLen = 
            editor.lines[sectionInfo.bottom.line].len - sectionInfo.bottom.textAt;
        remainingBottomText = HeapAlloc(char, remainingBottomLen + 1);
        memcpy(remainingBottomText, 
               editor.lines[sectionInfo.bottom.line].text + sectionInfo.bottom.textAt, 
               remainingBottomLen);
        remainingBottomText[remainingBottomLen] = 0;
    }    

    //Shift lines below highlited section up
    editor.numLines -= sectionInfo.bottom.line - sectionInfo.top.line;
    for (int i = sectionInfo.top.line + 1; i < editor.numLines; ++i)
    {
        editor.lines[i] = editor.lines[i + sectionInfo.bottom.line - sectionInfo.top.line];
    }

    //Remove Text from top line and connect bottom line text
    const int topRemovedLen = sectionInfo.topLen;
    memcpy(
        editor.lines[sectionInfo.top.line].text + sectionInfo.top.textAt, 
        editor.lines[sectionInfo.top.line].text + sectionInfo.top.textAt + topRemovedLen,
        editor.lines[sectionInfo.top.line].len - (sectionInfo.top.textAt + topRemovedLen)
    );
    editor.lines[sectionInfo.top.line].len += remainingBottomLen - topRemovedLen;
    ResizeDynamicArray(&editor.lines[sectionInfo.top.line].text, 
                       editor.lines[sectionInfo.top.line].len,
                       sizeof(char),
                       &editor.lines[sectionInfo.top.line].size);
    
    memcpy(editor.lines[sectionInfo.top.line].text + sectionInfo.top.textAt, 
           remainingBottomText, 
           remainingBottomLen);
    editor.lines[sectionInfo.top.line].text[editor.lines[sectionInfo.top.line].len] = 0;
    

    editor.cursorPos = {sectionInfo.top.textAt, sectionInfo.top.line};
    SetTopChangedLine(editor.cursorPos.line);

	if (remainingBottomText) free(remainingBottomText);
}

void AddToUndoStack(EditorPos undoStart, EditorPos undoEnd, UndoType type, bool redo = false)
{
    UndoInfo undo;
    undo.start = undoStart;
    undo.end = undoEnd;
    undo.prevCursorPos = editor.cursorPos;
    undo.type = type;
    if (type == UNDOTYPE_REMOVED_TEXT_SECTION || type == UNDOTYPE_OVERWRITE)
    {
        //TODO: remove all the outside code that sets numLines for us that isn't necessary
        TextSectionInfo section = GetTextSectionInfo(undoStart, undoEnd);
        undo.textByLine = GetTextByLines(section);
        undo.numLines = section.bottom.line - section.top.line + 1;
    }

    if (redo)
        editor.redoStack[editor.numRedos++] = undo;
    else
        editor.undoStack[editor.numUndos++] = undo;
}

void ResetUndoStack(bool redo = false)
{
    UndoInfo* stack = (redo) ? editor.redoStack : editor.undoStack;
    int* numInStack = (redo) ? &editor.numRedos : &editor.numUndos;

    for (int i = 0; i < *numInStack; ++i)
    {
        if (stack[i].numLines != -1)
        {
            if (stack[i].textByLine)
            {
                for (int j = 0; j < stack[i].numLines; ++j)
                    free(stack[i].textByLine[j]);
                free(stack[i].textByLine);
            }
        }

        if (stack[i].reverseBuffer.buffer)
            free(stack[i].reverseBuffer.buffer);
    }

    *numInStack = 0;
}

void InsertTextInLine(int lineIndex, char* text, int textStart)
{
    size_t textLen = StringLen(text);

    editor.lines[lineIndex].len += (int)textLen;
    ResizeDynamicArray(&editor.lines[lineIndex].text,
                       editor.lines[lineIndex].len, 
                       sizeof(char), 
                       &editor.lines[lineIndex].size);
    
    int spaceLeft = editor.lines[lineIndex].size - editor.lines[lineIndex].len;
	int numToMove = min(editor.lines[lineIndex].len - textStart, spaceLeft);
    memmove(editor.lines[lineIndex].text + textStart + textLen, 
            editor.lines[lineIndex].text + textStart, 
			numToMove);
    memcpy(editor.lines[lineIndex].text + textStart, text, textLen);
    
    editor.lines[lineIndex].text[editor.lines[lineIndex].len] = 0;
}

void RemoveTextInLine(int lineIndex, int textStart, int textEnd)
{
    Assert(textStart <= textEnd);
	editor.lines[lineIndex].len -= textEnd - textStart;
    memcpy(editor.lines[lineIndex].text + textStart, 
           editor.lines[lineIndex].text + textEnd, 
		   editor.lines[lineIndex].len);
    editor.lines[lineIndex].text[editor.lines[lineIndex].len] = 0;
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

void InsertText(char** textAsLines, TextSectionInfo sectionInfo)
{
    const int numLines = sectionInfo.bottom.line - sectionInfo.top.line + 1;
    //Set up the lines before adding text to prevent pushing down the same lines being added
    for (int i = 1; i < numLines; ++i)
        InsertLineAt(sectionInfo.top.line + i);

    //Insert text
    //NOTE: Do not be fooled! remainderLen is not always sectionInfo.topLen.
    int remainderLen = editor.lines[sectionInfo.top.line].len - sectionInfo.top.textAt;
    char* remainderText = HeapAlloc(char, remainderLen + 1);
    memcpy(remainderText, 
            editor.lines[sectionInfo.top.line].text + sectionInfo.top.textAt,
            remainderLen + 1);
    remainderText[remainderLen] = 0;

    editor.lines[sectionInfo.top.line].len -= remainderLen;

    InsertTextInLine(sectionInfo.top.line, textAsLines[0], sectionInfo.top.textAt);
    for (int i = 1; i < numLines; ++i)
    {
        InsertTextInLine(sectionInfo.top.line + i, textAsLines[i], 0);
    }
    AppendTextToLine(sectionInfo.bottom.line, remainderText);
    
    free(remainderText);

    editor.cursorPos.line += numLines - 1;
    int lastLineLen = StringLen(textAsLines[numLines - 1]);
	//This does not work in all situations (namely Undo()), remove perhaps?
    if (sectionInfo.spansOneLine)
        editor.cursorPos.textAt += lastLineLen;
    else
        editor.cursorPos.textAt = lastLineLen;

    SetTopChangedLine(sectionInfo.top.line);
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

//TODO: Double check memory leaks
void HandleUndoInfo(UndoInfo undoInfo, bool isRedo)
{
    UndoInfo* stack = (!isRedo) ? editor.redoStack : editor.undoStack;
    int* numInStack = (!isRedo) ? &editor.numRedos : &editor.numUndos; 
    TextSectionInfo sectionInfo = GetTextSectionInfo(undoInfo.start, undoInfo.end);

    ClearHighlights();

    AddToUndoStack(undoInfo.start, undoInfo.end, UNDOTYPE_ADDED_TEXT, !isRedo);
    stack[*numInStack - 1].wasHighlight = undoInfo.wasHighlight; //hnnngghhhh

    switch(undoInfo.type)
    {
        case UNDOTYPE_ADDED_TEXT:
        {
            stack[*numInStack - 1].type = UNDOTYPE_REMOVED_TEXT_SECTION;
            stack[*numInStack - 1].textByLine = GetTextByLines(sectionInfo);
            RemoveTextSection(sectionInfo);
        } break;

        case UNDOTYPE_REMOVED_TEXT_SECTION:
        {
            stack[*numInStack - 1].type = UNDOTYPE_ADDED_TEXT;

            InsertText(undoInfo.textByLine, sectionInfo);
            
            if (!isRedo && undoInfo.wasHighlight)
            {
                editor.highlightStart = (undoInfo.prevCursorPos == sectionInfo.bottom) ? 
                                        sectionInfo.top : sectionInfo.bottom;
            }

            for (int i = 0; i < sectionInfo.bottom.line - sectionInfo.top.line + 1; ++i)
                free(undoInfo.textByLine[i]);
            free(undoInfo.textByLine);
        } break;

        case UNDOTYPE_REMOVED_TEXT_REVERSE_BUFFER:
        {    
            Assert(!isRedo);
            stack[*numInStack - 1].type = UNDOTYPE_ADDED_TEXT;

            //NOTE: Do not be fooled! remainderLen is not always sectionInfo.topLen.
            int remainderLen = editor.lines[sectionInfo.top.line].len - sectionInfo.top.textAt;
            char* remainderText = HeapAlloc(char, remainderLen + 1);
            memcpy(remainderText, 
                   editor.lines[sectionInfo.top.line].text + sectionInfo.top.textAt,
                   remainderLen + 1);
            remainderText[remainderLen] = 0;

            EditorPos at = sectionInfo.top;
            editor.lines[at.line].len -= remainderLen; //1337 epik hax0r cheet to avoid inserting at every line
            for (int i = undoInfo.reverseBuffer.len - 1; i >= 0; --i)
            {
                if (undoInfo.reverseBuffer.buffer[i] == '\n')
                {
                    editor.lines[at.line].text[at.textAt] = 0;
                    at.line++;
                    InsertLineAt(at.line);
                    at.textAt = 0;
                }
                else
                {
                    AppendToDynamicArray(editor.lines[at.line].text, 
                                         editor.lines[at.line].len,
                                         undoInfo.reverseBuffer.buffer[i], 
                                         editor.lines[at.line].size);
                    at.textAt++;
                }
            }

            if (!IsEmptyString(remainderText))
            {
                InsertTextInLine(sectionInfo.bottom.line, 
                                 remainderText, 
                                 sectionInfo.bottom.textAt);
            }

            free(remainderText);
        } break;

        case UNDOTYPE_OVERWRITE:
        {
            stack[*numInStack - 1].type = UNDOTYPE_OVERWRITE;

            Assert(undoInfo.numLines != -1);
            RemoveTextSection(sectionInfo);

            EditorPos insertStart = {sectionInfo.top.textAt, sectionInfo.top.line};
			EditorPos insertEnd = insertStart;
			int bottomInsertLen = StringLen(undoInfo.textByLine[undoInfo.numLines - 1]);
            insertEnd.textAt = (undoInfo.numLines == 1) * insertStart.textAt + bottomInsertLen;
            insertEnd.line += undoInfo.numLines - 1;
            InsertText(undoInfo.textByLine, GetTextSectionInfo(insertStart, insertEnd));

            stack[*numInStack - 1].start = insertStart;
            stack[*numInStack - 1].end = insertEnd;
            stack[*numInStack - 1].numLines = sectionInfo.bottom.line - sectionInfo.top.line + 1;

            if (!isRedo)
            {
                editor.highlightStart = (undoInfo.prevCursorPos == insertEnd) ? 
                                        insertStart : insertEnd;
            }
            
        } break;

        //TODO: This assumes that multine cursors are all at the same text index on each line, make this handle different test indicies
        case UNDOTYPE_MULTILINE_ADD:
        {
            stack[*numInStack - 1].type = UNDOTYPE_MULTILINE_REMOVE;
            stack[*numInStack - 1].numLines = undoInfo.numLines;
			stack[*numInStack - 1].textByLine = undoInfo.textByLine;

            for (int i = 0; i < undoInfo.numLines; ++i)
            {
                int line = i + sectionInfo.top.line;
                int end = sectionInfo.top.textAt + StringLen(undoInfo.textByLine[i]);
                RemoveTextInLine(line, sectionInfo.top.textAt, end);
            }
        } break;

        //TODO: This assumes that multine cursors are all at the same text index on each line, make this handle different test indicies
        case UNDOTYPE_MULTILINE_REMOVE:
        {
            stack[*numInStack - 1].type = UNDOTYPE_MULTILINE_ADD;
			stack[*numInStack - 1].numLines = undoInfo.numLines;
			stack[*numInStack - 1].textByLine = undoInfo.textByLine;

            for (int i = 0; i < undoInfo.numLines; ++i)
            {
                int line = i + sectionInfo.top.line;
                InsertTextInLine(line, undoInfo.textByLine[i], sectionInfo.top.textAt);
            }
        } break;
    }
     
    editor.cursorPos = undoInfo.prevCursorPos;
}

EditorPos GetEditorPosAtMouse()
{
    EditorPos result;
    int mouseLine = (screenBuffer.height - input.mousePixelPos.y) / CURSOR_HEIGHT;
    result.line = min(mouseLine, editor.numLines - 1);
    
    int linePixLen = TEXT_X_OFFSET;
    result.textAt = 0;
    Line line = editor.lines[result.line];
    while (linePixLen < input.mousePixelPos.x && result.textAt < line.len)
    {
        linePixLen += fontChars[line.text[result.textAt]].advance;
        result.textAt++;
    }

    return result;
}

void HighlightWordAt(EditorPos pos)
{
    char startingChar = editor.lines[pos.line].text[pos.textAt];
    if (startingChar == 0) return;

    int highlightTextAtStart = pos.textAt;
    int newCursorTextAt = pos.textAt;
    Line line = editor.lines[pos.line]; 

    bool (*correctChar)(char) = IsPunctuation; 
    if (IsAlphaNumeric(startingChar)) correctChar = IsAlphaNumeric;
    else if (IsInvisChar(startingChar)) correctChar = IsInvisChar;

    while (correctChar(line.text[highlightTextAtStart]) && highlightTextAtStart > 0)
        highlightTextAtStart--;

    while(correctChar(line.text[newCursorTextAt]) && newCursorTextAt < line.len)
        newCursorTextAt++;

	editor.highlightStart = {highlightTextAtStart + (highlightTextAtStart > 0), pos.line};
	editor.cursorPos = {newCursorTextAt, pos.line};
}

//
//KEY-MAPPED FUNCTIONS
//


void AddChar()
{
    UndoInfo* currentUndo = &editor.undoStack[editor.numUndos - 1];

    Line* line = &editor.lines[editor.cursorPos.line];

    char lastTypedChar = line->text[editor.cursorPos.textAt - 1];
    char secondLastTypedChar = line->text[editor.cursorPos.textAt - 2];
    bool startOfNewWord = (editor.currentChar == ' ' && lastTypedChar != ' ') || 
        (editor.currentChar != ' ' && lastTypedChar == ' ' && secondLastTypedChar == ' ');

    if (startOfNewWord || editor.currentChar == '\t' || currentUndo->type != UNDOTYPE_ADDED_TEXT || 
        line->len == 0 || editor.highlightStart.textAt != -1 || 
        editor.cursorPos != currentUndo->end)
    {
        AddToUndoStack(editor.cursorPos, editor.cursorPos, UNDOTYPE_ADDED_TEXT);
        ResetUndoStack(true);
    }

    if (editor.highlightStart.textAt != -1)
    {
        TextSectionInfo highlightInfo = 
            GetTextSectionInfo(editor.highlightStart, editor.cursorPos);
        editor.undoStack[editor.numUndos - 1].type = UNDOTYPE_OVERWRITE;
        editor.undoStack[editor.numUndos - 1].textByLine = GetTextByLines(highlightInfo);
        editor.undoStack[editor.numUndos - 1].numLines = 
            highlightInfo.bottom.line - highlightInfo.top.line + 1;
        
		RemoveTextSection(highlightInfo);
        editor.undoStack[editor.numUndos - 1].start = editor.cursorPos;
		line = &editor.lines[editor.cursorPos.line]; //cursor has moved so get it again
    }

    int numCharsAdded = 0; 
    char textToInsert[5]; //this'll likely never need to be greater than 5
    switch(editor.currentChar)
    {
        case '\t':
        {
            numCharsAdded = 4 - editor.cursorPos.textAt % 4;
            for (int i = 0; i < numCharsAdded; ++i)
                textToInsert[i] = ' ';
        } break;

        case '{':
        case '[':
        case '(':
        {
            numCharsAdded = 2;
            textToInsert[0] = editor.currentChar;
            textToInsert[1] = GetOtherBracket(editor.currentChar);
        } break;

        case '\'':
        case '"':
        {
            numCharsAdded = 2;
            textToInsert[0] = editor.currentChar;
            textToInsert[1] = editor.currentChar;
        } break;

        default:
        {
            numCharsAdded = 1;
            textToInsert[0] = editor.currentChar;
        } break;
    }
    Assert(numCharsAdded > 0);
    textToInsert[numCharsAdded] = 0;
    InsertTextInLine(editor.cursorPos.line, textToInsert, editor.cursorPos.textAt);

    editor.cursorPos.textAt += (editor.currentChar == '\t') ? numCharsAdded : 1;

    editor.undoStack[editor.numUndos - 1].end = editor.cursorPos;

    SetTopChangedLine(editor.cursorPos.line);
}

void RemoveChar()
{
    Line* line = &editor.lines[editor.cursorPos.line];
    UndoInfo* currentUndo = &editor.undoStack[editor.numUndos - 1];
	ResizableString* reverseBuffer;

    if (currentUndo->type != UNDOTYPE_REMOVED_TEXT_REVERSE_BUFFER || 
        editor.cursorPos != currentUndo->end)
    {
        AddToUndoStack(editor.cursorPos, editor.cursorPos, UNDOTYPE_REMOVED_TEXT_REVERSE_BUFFER);
		reverseBuffer = &editor.undoStack[editor.numUndos - 1].reverseBuffer;
		reverseBuffer->buffer = HeapAlloc(char, reverseBuffer->size);
        ResetUndoStack(true);
    }
	else
	{
		//This feels redundant lol
		reverseBuffer = &editor.undoStack[editor.numUndos - 1].reverseBuffer;
	}

    if (editor.cursorPos.textAt > 0)
    {
        //Append to undo text buffer
        AppendToDynamicArray(reverseBuffer->buffer, 
                             reverseBuffer->len, 
                             line->text[editor.cursorPos.textAt - 1],
                             reverseBuffer->size);
        
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
        //Append to undo text buffer
        AppendToDynamicArray(reverseBuffer->buffer, reverseBuffer->len, '\n', reverseBuffer->size);

        editor.cursorPos.textAt = editor.lines[editor.cursorPos.line - 1].len;
        AppendTextToLine(editor.cursorPos.line - 1, editor.lines[editor.cursorPos.line].text);

		//Shift lines up
		for (int i = editor.cursorPos.line; i < editor.numLines; ++i)
		{
			editor.lines[i] = editor.lines[i + 1];
		}
		editor.numLines--;

        editor.cursorPos.line--;
    }

    editor.undoStack[editor.numUndos - 1].end = editor.cursorPos;

    reverseBuffer->buffer[reverseBuffer->len] = 0;

    SetTopChangedLine(editor.cursorPos.line);
}

void Backspace()
{
    if (editor.highlightStart.textAt != -1)
    {
        AddToUndoStack(editor.highlightStart, editor.cursorPos, UNDOTYPE_REMOVED_TEXT_SECTION);
        editor.undoStack[editor.numUndos - 1].wasHighlight = true;
        TextSectionInfo highlightInfo = GetTextSectionInfo(editor.highlightStart, editor.cursorPos);
        editor.undoStack[editor.numUndos - 1].textByLine = GetTextByLines(highlightInfo);
        ResetUndoStack(true);
        
        RemoveTextSection(highlightInfo);
        ClearHighlights();
    }
    else
    {
        RemoveChar();
    }
}

void UnTab()
{
    TextSectionInfo highlight;
    highlight.bottom.line = -1;
    int lineAt = editor.cursorPos.line;
    bool isMultiline = editor.highlightStart.line != -1;
    if (isMultiline)
    {
        highlight = GetTextSectionInfo(editor.highlightStart, editor.cursorPos);
        lineAt = highlight.top.line;
    }

    AddToUndoStack({-1, editor.cursorPos.line}, {-1, editor.cursorPos.line}, UNDOTYPE_REMOVED_TEXT_SECTION);
    //Free what was initially there as it'll likely be incorrect, maybe refactor this?
    free(editor.undoStack[editor.numUndos - 1].textByLine[0]);
    free(editor.undoStack[editor.numUndos - 1].textByLine);

    if (editor.highlightStart.line != -1)
    {
        int numLines = highlight.bottom.line - highlight.top.line + 1;
        editor.undoStack[editor.numUndos - 1].textByLine = HeapAlloc(char*, numLines);
        editor.undoStack[editor.numUndos - 1].numLines = numLines;
    }
	else
	{
		editor.undoStack[editor.numUndos - 1].textByLine = HeapAlloc(char*, 1);
	}

    do
    {
        int numSpacesAtFront = 0;
        while (editor.lines[lineAt].text[numSpacesAtFront] == ' ')
            numSpacesAtFront++;
        int numRemoved = (numSpacesAtFront - 1) % 4 + 1;

		if (lineAt == editor.cursorPos.line)
		{
			editor.undoStack[editor.numUndos - 1].start.textAt = 0;
			editor.undoStack[editor.numUndos - 1].end.textAt = 0;
		}

        if (numSpacesAtFront > 0)
        {
            int destIndex = numSpacesAtFront - numRemoved;
            memcpy(editor.lines[lineAt].text + destIndex, 
                   editor.lines[lineAt].text + numSpacesAtFront, 
                   editor.lines[lineAt].len - numRemoved);
            editor.lines[lineAt].len -= numRemoved;
            editor.lines[lineAt].text[editor.lines[lineAt].len] = 0;

            editor.undoStack[editor.numUndos - 1].start.textAt = 
                min(destIndex, editor.undoStack[editor.numUndos - 1].start.textAt);
            //If first line, set undo positions accordingly
            if (lineAt == editor.cursorPos.line) 
            {
                editor.undoStack[editor.numUndos - 1].start.textAt = destIndex;
                editor.undoStack[editor.numUndos - 1].end.textAt = numSpacesAtFront;
            }
            editor.undoStack[editor.numUndos - 1].start.textAt = 
                min(destIndex, editor.undoStack[editor.numUndos - 1].start.textAt);

            if (editor.cursorPos.line == lineAt && editor.cursorPos.textAt > destIndex)
                editor.cursorPos.textAt -= min(numRemoved, editor.cursorPos.textAt - destIndex);
            if (editor.highlightStart.line == lineAt)
                editor.highlightStart.textAt -= min(numRemoved, editor.highlightStart.textAt - destIndex);

            SetTopChangedLine(lineAt);
        }

        int textByLineIndex = lineAt - editor.cursorPos.line;
        editor.undoStack[editor.numUndos - 1].textByLine[textByLineIndex] = HeapAlloc(char, numRemoved + 1);
        for (int i = 0; i < numRemoved; ++i)
            editor.undoStack[editor.numUndos - 1].textByLine[textByLineIndex][i] = ' ';
        editor.undoStack[editor.numUndos - 1].textByLine[textByLineIndex][numRemoved] = 0;

        lineAt++;
    } while (lineAt <= highlight.bottom.line);

    //If more than one line, set proper undo info
    if (isMultiline)
    {
        editor.undoStack[editor.numUndos - 1].type = UNDOTYPE_MULTILINE_REMOVE;
        editor.undoStack[editor.numUndos - 1].end.line = highlight.bottom.line;
    }
}

void Enter()
{
    if (editor.numLines < MAX_LINES)
    { 
        AddToUndoStack(editor.cursorPos, editor.cursorPos, UNDOTYPE_ADDED_TEXT);
        ResetUndoStack(true);
        if (editor.highlightStart.textAt != -1)
        {
            TextSectionInfo highlightInfo = GetTextSectionInfo(editor.highlightStart, 
                                                             editor.cursorPos);
	    	editor.undoStack[editor.numUndos - 1].type = UNDOTYPE_OVERWRITE;
            editor.undoStack[editor.numUndos - 1].textByLine = GetTextByLines(highlightInfo);
            editor.undoStack[editor.numUndos - 1].numLines = 
                highlightInfo.bottom.line - highlightInfo.top.line;

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

        SetTopChangedLine((copiedLen > 0) ? prevLineIndex : editor.cursorPos.line);

        editor.undoStack[editor.numUndos - 1].end = editor.cursorPos;
    }

	
}

void HighlightCurrentLine()
{
    if (editor.highlightStart.textAt == -1)
    { 
        Assert(editor.highlightStart.line == -1);
        editor.highlightStart.textAt = 0;
        editor.highlightStart.line = editor.cursorPos.line;
    }

    if (editor.cursorPos.line < editor.numLines - 1)
    {
        editor.cursorPos.textAt = 0;
        editor.cursorPos.line += editor.cursorPos.line + 1 < MAX_LINES;
    }
    else 
    {
        editor.cursorPos.textAt = editor.lines[editor.cursorPos.line].len;
    }
}

void HighlightEntireFile()
{
    editor.highlightStart.textAt = 0;
    editor.highlightStart.line = 0;
    editor.cursorPos.textAt = editor.lines[editor.numLines - 1].len;
    editor.cursorPos.line = editor.numLines - 1;
}

void RemoveCurrentLine()
{
    int removedLine = editor.cursorPos.line;
    bool isLastLine = removedLine == editor.numLines - 1;

    EditorPos undoStart = {editor.lines[removedLine - 1].len, removedLine - 1}; //This is to make undo actually insert a new line. A tad annoying but hey.
    EditorPos undoEnd = {editor.lines[removedLine].len, removedLine};
    AddToUndoStack(undoStart, undoEnd, UNDOTYPE_REMOVED_TEXT_SECTION);

    for (int i = removedLine + 1; i < editor.numLines; ++i)
        editor.lines[i-1] = editor.lines[i];
    
    if (editor.numLines == 1)
    {
        editor.lines[removedLine].len = 0;
        editor.lines[removedLine].text[0] = 0;
        editor.cursorPos.textAt = 0;
    }
    else 
    {
        editor.cursorPos.line -= isLastLine;
        editor.cursorPos.textAt = 
            min(editor.lines[editor.cursorPos.line].len, editor.cursorPos.textAt);
    }

    //This needs to happen here so that the above if statement works properly
    editor.numLines -= (editor.numLines != 1);

    ClearHighlights();
    
    SetTopChangedLine(editor.cursorPos.line);
}

//TODO: Double check if this is susceptable to overflow attacks and Unix Compatability
void CopyHighlightedText()
{
    if (editor.highlightStart.textAt == -1) 
    {
        Assert(editor.highlightStart.line == -1);
        return;
    }

    TextSectionInfo highlightInfo = GetTextSectionInfo(editor.highlightStart, editor.cursorPos);

    size_t copySize = highlightInfo.topLen + 2 * (!highlightInfo.spansOneLine);
    for (int i = highlightInfo.top.line + 1; i < highlightInfo.bottom.line ; ++i)
    {
        copySize += editor.lines[i].len + 2;
    }
    if (!highlightInfo.spansOneLine) copySize += highlightInfo.bottom.textAt;

    char* copiedText = HeapAlloc(char, copySize + 1);
    memcpy(copiedText, 
           editor.lines[highlightInfo.top.line].text + highlightInfo.top.textAt, 
           highlightInfo.topLen);
	int at = highlightInfo.topLen;
	if (!highlightInfo.spansOneLine)
	{
		copiedText[highlightInfo.topLen]     = '\r';
		copiedText[highlightInfo.topLen + 1] = '\n';
		at += 2;
	}
    for (int i = highlightInfo.top.line + 1; i < highlightInfo.bottom.line; ++i)
    {
        memcpy(copiedText + at, editor.lines[i].text, editor.lines[i].len);
        copiedText[at + editor.lines[i].len]     = '\r'; 
        copiedText[at + editor.lines[i].len + 1] = '\n'; 
        at += editor.lines[i].len + 2;
    }
    if (!highlightInfo.spansOneLine) 
        memcpy(copiedText + at, 
               editor.lines[highlightInfo.bottom.line].text, 
               highlightInfo.bottom.textAt);
    copiedText[copySize] = 0;

    CopyToClipboard(copiedText, copySize);

    free(copiedText);
}

void Paste()
{
    char* textToPaste = GetClipboardText();
    if (textToPaste != nullptr)
    {
        int numLinesToPaste = 0;
        char** linesToPaste = SplitStringByLines(textToPaste, &numLinesToPaste);
        free(textToPaste);

        AddToUndoStack(editor.cursorPos, editor.cursorPos, UNDOTYPE_ADDED_TEXT);
        ResetUndoStack(true);

		if (editor.highlightStart.textAt != -1)
        {
            Assert(editor.highlightStart.line != -1);
            
            TextSectionInfo highlightInfo = 
                GetTextSectionInfo(editor.highlightStart, editor.cursorPos); 

			editor.undoStack[editor.numUndos - 1].start = highlightInfo.top;
            editor.undoStack[editor.numUndos - 1].type = UNDOTYPE_OVERWRITE;
            editor.undoStack[editor.numUndos - 1].textByLine = GetTextByLines(highlightInfo);
            editor.undoStack[editor.numUndos - 1].numLines = 
                highlightInfo.bottom.line - highlightInfo.top.line + 1;
            
            RemoveTextSection(highlightInfo);
            ClearHighlights();
        }

        TextSectionInfo sectionInfo;
        sectionInfo.spansOneLine = numLinesToPaste == 1;
        sectionInfo.top.textAt = editor.cursorPos.textAt;
        sectionInfo.topLen = StringLen(linesToPaste[0]);
        sectionInfo.top.line = editor.cursorPos.line;
        sectionInfo.bottom.textAt = (sectionInfo.spansOneLine) * editor.cursorPos.textAt + 
                                    StringLen(linesToPaste[numLinesToPaste - 1]);
        sectionInfo.bottom.line = editor.cursorPos.line + numLinesToPaste - 1;
        InsertText(linesToPaste, sectionInfo);

        editor.undoStack[editor.numUndos - 1].end = editor.cursorPos;
        SetTopChangedLine(sectionInfo.top.line);

        for (int i = 0; i < numLinesToPaste; ++i)
            free(linesToPaste[i]);
        free(linesToPaste);
    }   
}

//TODO: Investigate Performance of this
void CutHighlightedText()
{
    CopyHighlightedText();
    
    AddToUndoStack(editor.highlightStart, editor.cursorPos, UNDOTYPE_REMOVED_TEXT_SECTION);
    TextSectionInfo highlightInfo = GetTextSectionInfo(editor.highlightStart, editor.cursorPos);
    editor.undoStack[editor.numUndos - 1].textByLine = GetTextByLines(highlightInfo);
    ResetUndoStack(true);

    RemoveTextSection(GetTextSectionInfo(editor.highlightStart, editor.cursorPos));
    ClearHighlights();
}

//TODO: Resize editor.lines if file too big + maybe return success bool?
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
        FreeWin32(file);

        ResetUndoStack();
        ResetUndoStack(true);

        editor.topChangedLineIndex = -1;
        editor.cursorPos = {0, 0};
    }
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
    if (editor.numUndos == 0) return;

    UndoInfo undoInfo = editor.undoStack[--editor.numUndos];
    HandleUndoInfo(undoInfo, false);
}

void Redo()
{
    if (editor.numRedos == 0) return;

    UndoInfo redoInfo = editor.redoStack[--editor.numRedos];
    HandleUndoInfo(redoInfo, true);
}

//
//MAIN LOOP/STUFF
//

TimedEvent cursorBlink = {0.5f};
TimedEvent holdChar = {0.5f};
TimedEvent repeatChar = {0.02f, &AddChar};

TimedEvent holdAction = {0.5f};
TimedEvent repeatAction = {0.02f};

//TODO: Move multiclicks of this into input
int numMultiClicks = 0;
bool doubleClicked = false;
TimedEvent doubleClick = {0.5f};
EditorPos prevMousePos = {-1, -1};

bool capslockOn = false;
bool nonCharKeyPressed = false;

UserSettings userSettings;

void Init()
{
    for (int i = 0; i < MAX_LINES; ++i)
    {
        editor.lines[i] = InitLine();
    }

    userSettings = LoadUserSettingsFromConfigFile();
}

void Draw(float dt)
{
    //Detect key input and handle char input
    //TODO: look into simultaneous input with backpace and char keys
    bool charKeyDown = false;
    bool charKeyPressed = false;
    bool newCharKeyPressed = false;
    bool ctrlUp = false; //This seems hacky but whatever
    for (int code = (int)KEYS_START; code < (int)NUM_INPUTS; ++code)
    {
        if (code >= (int)CHAR_KEYS_START)
        {
            if (InputDown(input.flags[code]))
            {
                char charOfKeyPressed = InputCodeToChar((InputCode)code, 
                                                        InputHeld(input.leftShift), 
                                                        capslockOn);
                
                //Hacky Fix. Refactor if possible
                if (charOfKeyPressed == '\t' && InputHeld(input.leftShift)) break;

                editor.currentChar = charOfKeyPressed;
                if (charOfKeyPressed) AddChar();

                holdAction.elapsedTime = 0.0f;

                charKeyDown = true;
                if (charKeyPressed)
                    newCharKeyPressed = true;
                nonCharKeyPressed = false;
            }
            else if (InputHeld(input.flags[code]))
            {
                charKeyPressed = true;
                if (ctrlUp)
                {
                    char charOfKeyPressed = InputCodeToChar((InputCode)code, 
                                                        InputHeld(input.leftShift), 
                                                        capslockOn);
				    Assert(charOfKeyPressed != 0);
                    editor.currentChar = charOfKeyPressed;
                }
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
        else if (code == INPUTCODE_LCTRL)
        {
            if (InputUp(input.leftCtrl)) ctrlUp = true;
            else if (InputHeld(input.leftCtrl)) break;
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

        //Handle Shift Tab
        //TODO: Less repetition
        if (InputHeld(input.leftShift))
        {
            if (InputDown(input.tab))
            {
                repeatAction.OnTrigger = UnTab;
                UnTab();
                holdAction.elapsedTime = 0.0f;
            }
            else if (InputHeld(input.tab))
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

        if (InputDown(input.f5))
            userSettings = LoadUserSettingsFromConfigFile();


        if (InputDown(input.leftMouse))
        {
            ClearHighlights();
            editor.cursorPos = GetEditorPosAtMouse();
    
            if (numMultiClicks > 0 && prevMousePos == editor.cursorPos)
            {
                //Technically there's a glitch here where if someone clicks more than
                //INT_MAX - 1 times this will repeat. I don't think it's worth fixing
                //though as there's no actual reason I can think of where this bug
                //is problematic
                switch (numMultiClicks)
                {
                    case 1:
                        HighlightWordAt(editor.cursorPos);
                        break;

                    case 2:
                        HighlightCurrentLine();
                        break;

                    default:
						Assert(numMultiClicks > 0);
                        HighlightEntireFile();
                        break;
                }
                doubleClicked = true;
                doubleClick.elapsedTime = 0.0f;
            }
            prevMousePos = GetEditorPosAtMouse(); //I don't think this is 100% correct but it works
            numMultiClicks += doubleClick.elapsedTime <= doubleClick.interval;
        }

        //TODO: Maybe make mouse drag highlight extend from multi click? 
        if (InputHeld(input.leftMouse))
        {
            InitHighlight(editor.cursorPos.textAt, editor.cursorPos.line);
            if (!doubleClicked) editor.cursorPos = GetEditorPosAtMouse();
        }

        if (InputUp(input.leftMouse))
            doubleClicked = false;


        if (!inputHeld)
        {
            //TODO: Move these arrays out of loop at some point
            KeyCommand keyCommands[] =
            {
                {CTRL, INPUTCODE_A},
                {CTRL, INPUTCODE_C},
                {CTRL, INPUTCODE_L},
                {CTRL | SHIFT, INPUTCODE_L},
                {CTRL, INPUTCODE_O},
                {CTRL, INPUTCODE_S},
                {CTRL | SHIFT, INPUTCODE_S},
                {CTRL, INPUTCODE_V},
                {CTRL, INPUTCODE_X},
                {CTRL, INPUTCODE_Y},
                {CTRL, INPUTCODE_Z},
            };

            void (*keyCommandCallbacks[])(void) = 
            {
                HighlightEntireFile,
                CopyHighlightedText,
                HighlightCurrentLine,
                RemoveCurrentLine,
                OpenFile,
                Save,
                SaveAs,
                Paste,
                CutHighlightedText,
                Redo,
                Undo
            };

            for (int i = 0; i < ArrayLen(keyCommands); ++i)
            {
                if (KeyCommandDown(keyCommands[i]))
                {
                    keyCommandCallbacks[i]();
                    break;
                }
            }
        }
    }

    //Only caught this bug when using mouse but putting it here since may save my back in other situations
    if (editor.highlightStart == editor.cursorPos)
        ClearHighlights();

    if (numMultiClicks)
        doubleClick.elapsedTime += dt;
    
    if (doubleClick.elapsedTime >= doubleClick.interval)
    {
        numMultiClicks = 0;
        doubleClick.elapsedTime = 0.0f;
    }

    //Draw Background
    Rect screenDims = {0, screenBuffer.width, 0, screenBuffer.height};
    DrawRect(screenDims, userSettings.backgroundColour);
    
    //Get correct position for cursor
    const IntPair start = {TEXT_X_OFFSET, screenBuffer.height - CURSOR_HEIGHT}; 
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
        DrawText(editor.lines[i].text, x, y, userSettings.textColour, textBounds);

        //Draw Line num
        char lineNumText[8];
        IntToString(i + 1, lineNumText);
        int lineNumOffset = (fontChars[' '].advance) * 4;
        DrawText(lineNumText, 
                    start.x - lineNumOffset, 
                    y, 
                    userSettings.lineNumColour, 
                    {0, 0, yBottomLimit, 0});
    }

    //Draw highlighted text
    if (editor.highlightStart.textAt != -1) 
    {
        Assert(editor.highlightStart.line != -1);
        TextSectionInfo highlightInfo = GetTextSectionInfo(editor.highlightStart, editor.cursorPos);
        
        //Draw top line highlight
        char* topHighlightText = 
            editor.lines[highlightInfo.top.line].text + highlightInfo.top.textAt;
        const int topHighlightPixelLength = 
            TextPixelLength(topHighlightText, highlightInfo.topLen);
		int topXOffset = 
            TextPixelLength(editor.lines[highlightInfo.top.line].text, highlightInfo.top.textAt);
        int topX = start.x + topXOffset - editor.textOffset.x;
        int topY = start.y - highlightInfo.top.line * CURSOR_HEIGHT - PIXELS_UNDER_BASELINE + 
                   editor.textOffset.y;
        if (editor.lines[highlightInfo.top.line].len == 0) topXOffset = fontChars[' '].advance;
        DrawAlphaRect(
            {topX, topX + topHighlightPixelLength, topY, topY + CURSOR_HEIGHT},
            userSettings.highlightColour, 
            textBounds
        );

        //Draw inbetween highlights
        for (int i = highlightInfo.top.line + 1; i < highlightInfo.bottom.line; ++i)
        {
            int highlightedPixelLength = TextPixelLength(editor.lines[i].text, editor.lines[i].len); 
            if (editor.lines[i].len == 0) highlightedPixelLength = fontChars[' '].advance;
            int x = start.x - editor.textOffset.x;
            int y = start.y - i * CURSOR_HEIGHT - PIXELS_UNDER_BASELINE + editor.textOffset.y;
            DrawAlphaRect(
                {x, x + highlightedPixelLength, y, y + CURSOR_HEIGHT}, 
                userSettings.highlightColour, 
                textBounds
            );
        }

        //Draw bottom line highlight
        if (!highlightInfo.spansOneLine)
        {
            int bottomHighlightPixelLength = 
                TextPixelLength(editor.lines[highlightInfo.bottom.line].text, 
                                highlightInfo.bottom.textAt);
            int bottomX = start.x - editor.textOffset.x;
            int bottomY = start.y - highlightInfo.bottom.line * CURSOR_HEIGHT 
                          - PIXELS_UNDER_BASELINE + editor.textOffset.y;
            if (editor.lines[highlightInfo.bottom.line].len == 0) 
                bottomHighlightPixelLength = fontChars[' '].advance;
            DrawAlphaRect(
                {bottomX, bottomX + bottomHighlightPixelLength, bottomY, bottomY + CURSOR_HEIGHT}, 
                userSettings.highlightColour, 
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
        DrawRect(cursorDims, userSettings.cursorColour);
    }
}
