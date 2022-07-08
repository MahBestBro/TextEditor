#include <windows.h>

#include "TextEditor.h"
#include "TextEditor_input.h"
#include "TextEditor_string.h"
#include "TextEditor_font.h"
#include "TextEditor_config.h"
#include "TextEditor_tokeniser.h"

#define MAX_LINE_NUM_DIGITS 6
#define PIXELS_UNDER_BASELINE 5
#define LINE_NUM_OFFSET ((fontData.chars[' '].advance) * 4)

const Allocator lineMemoryAllocator = {LineMemory_Alloc, LineMemory_Realloc, LineMemory_Free};

//extern StringArena temporaryStringArena;
//extern Input input;
//extern Font font;
//extern ScreenBuffer screenBuffer;

struct TimedEvent
{
    float interval;
    KeyCallback onTrigger = {0};
    float elapsedTime = 0.0f;
};

void HandleTimedEvent(TimedEvent* timedEvent, float dt, TimedEvent* nestedEvent, Editor* editor = nullptr)
{
    timedEvent->elapsedTime += dt;
    if (timedEvent->elapsedTime >= timedEvent->interval)
    {
        if (timedEvent->onTrigger.voidFunc) //May not be needed???
        {
            if (timedEvent->onTrigger.isEditorFunc) timedEvent->onTrigger.editorFunc(editor); 
            else timedEvent->onTrigger.voidFunc();
        }
        
        if (nestedEvent)
        {
            nestedEvent->elapsedTime += dt;
            if (nestedEvent->elapsedTime >= nestedEvent->interval)
            {
				if (nestedEvent->onTrigger.voidFunc)
				{
					if (nestedEvent->onTrigger.isEditorFunc) nestedEvent->onTrigger.editorFunc(editor);
					else nestedEvent->onTrigger.voidFunc();
				}

                nestedEvent->elapsedTime = 0.0f;
            }
        }
    }
}

//Can only think that these are the only 3 needed. Chnge this in the future?
enum CommandKeys
{
    NONE  = 0b000,
    CTRL  = 0b001,
    ALT   = 0b010,
    SHIFT = 0b100
};

CommandKeys operator|(CommandKeys a, CommandKeys b)
{
    return (CommandKeys)((byte)a | (byte)b);
}

struct KeyBinding
{
    CommandKeys commandKeys; //Keys like ctrl, shift and alt which are usually held before actual key
    InputCode mainKey;
    KeyCallback callback;
};

bool KeyCommandDown(KeyBinding keyBinding)
{
	if (InputDown(input.letterKeys['L' - 'A']))
	{
		Assert(true);
	}

    Assert(keyBinding.commandKeys);

    bool mainKeyDown = InputDown(input.flags[keyBinding.mainKey]);
    
    byte heldCommandKeys = 0b000;
    heldCommandKeys |= (byte)InputHeld(input.leftCtrl);
    heldCommandKeys |= (byte)InputHeld(input.leftAlt) << 1;
    heldCommandKeys |= (byte)InputHeld(input.leftShift) << 2;
    
    return mainKeyDown && (heldCommandKeys == keyBinding.commandKeys);
}

int TextPixelLength(char* text, int len)
{
    int result = 0;
    for (int i = 0; i < len; i++)
        result += fontData.chars[text[i]].advance;
    return result;
}

int TextPixelLength(string text)
{
    int result = 0;
    for (int i = 0; i < text.len; i++)
        result += fontData.chars[text[i]].advance;
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

void Draw8bppPixels(Rect rect, uint8* pixels, int stride, Colour colour, Rect limits)
{
    Assert(rect.left <= rect.right);
    Assert(rect.bottom <= rect.top);

    //TODO: There shouldn't be -1s at the end of these things, fix
    if (!limits.right) limits.right = screenBuffer.width - 1;
    if (!limits.top) limits.top = screenBuffer.height - 1;

    int pixelColStart = -min(0, rect.left - limits.left);
    int pixelRowStart = -min(0, limits.top - rect.top);

    rect.left   = Clamp(rect.left,   limits.left,   limits.right);
    rect.right  = Clamp(rect.right,  limits.left,   limits.right);
    rect.bottom = Clamp(rect.bottom, limits.bottom, limits.top);
    rect.top    = Clamp(rect.top,    limits.bottom, limits.top);

    int drawWidth = rect.right - rect.left;
    int drawHeight = rect.top - rect.bottom;

    int start = (rect.left + rect.top * screenBuffer.width) * PIXEL_IN_BYTES;
	uint8* row = (uint8*)screenBuffer.memory + start;
	for (int y = 0; y < drawHeight; ++y)
    {
        uint8* pixel = row;
        for (int x = 0; x < drawWidth; ++x)
        {
            uint8* greyScalePixel = pixels + stride * (y + pixelRowStart) + x + pixelColStart;
            
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
		row -= screenBuffer.width * PIXEL_IN_BYTES;
    }
}

void DrawText(string text, int xCoord, int yCoord, Colour colour, Rect limits)
{ 
	int xAdvance = 0;
    for (int i = 0; i < text.len; i++)
    {
        FontChar fc = fontData.chars[text[i]];
        int xOffset = fc.left + xAdvance + xCoord;
        int yOffset = yCoord - (fc.height + fc.top);
        Rect charDims = {xOffset, (int)(xOffset + fc.width), yOffset, (int)(yOffset + fc.height)};

        Draw8bppPixels(charDims, (byte*)fc.pixels, fc.width, colour, limits);

		xAdvance += fc.advance;
    }
}

//
//EDITOR HELPER FUNCTIONS
//

global Editor editors[3];
global int numEditors = 1;
global int currentEditorSide = 0;
global int openEditorIndexes[] = {0, 1};
//Editor editor;

global TokenInfo tokenInfos[3];

IntPair GetLeftTextStart()
{
    return IntPair 
    {
        36 + MAX_LINE_NUM_DIGITS * (int)fontData.chars['0'].advance,
        screenBuffer.height - (int)fontData.maxHeight - 5
    };
}

IntPair GetRightTextStart()
{
    IntPair result = GetLeftTextStart();
    result.x += screenBuffer.width / 2;
    return result;
}

inline IntPair GetCurrentEditorTextStart()
{
    return (currentEditorSide) ? GetRightTextStart() : GetLeftTextStart();
}

Rect GetLeftTextLimits()
{
    return Rect
    {
        36 + MAX_LINE_NUM_DIGITS * (int)fontData.chars['0'].advance,
        screenBuffer.width / 2 - 10,
        (int)(fontData.maxHeight + fontData.lineGap),
        screenBuffer.height - 1
    };
}

Rect GetRightTextLimits()
{
    return Rect
    {
        36 + MAX_LINE_NUM_DIGITS * (int)fontData.chars['0'].advance + screenBuffer.width / 2,
        screenBuffer.width - 10,
        (int)(fontData.maxHeight + fontData.lineGap),
        screenBuffer.height - 1
    };
}

inline Rect GetCurrentEditorTextLimits()
{
    return (currentEditorSide) ? GetRightTextLimits() : GetLeftTextLimits();
}

void InitHighlight(Editor* editor, int initialTextIndex, int initialLineIndex)
{
    if (editor->highlightStart.textAt == -1)
    {
        Assert(editor->highlightStart.line == -1);
        editor->highlightStart.textAt = initialTextIndex;
        editor->highlightStart.line = initialLineIndex;
    }
}

void ClearHighlights(Editor* editor)
{
    editor->highlightStart.textAt = -1;
    editor->highlightStart.line = -1;
}

void AdvanceCursorToEndOfWord(Editor* editor, bool forward)
{
	if (editor->lines[editor->cursorPos.line].len == 0) return; //This happens when you go form the end of one line down to an empty line

    string_buf line = editor->lines[editor->cursorPos.line];
    
    bool (*ShouldAdvance)(char) = nullptr;
    bool skipOverSpace = true;
    do 
    {
        editor->cursorPos.textAt += (forward) ? 1 : -1;
        if (!IsWhiteSpace(line[editor->cursorPos.textAt - !forward]))
            skipOverSpace = false;
        
        if (!skipOverSpace && ShouldAdvance == nullptr)
        {
            ShouldAdvance = (IsPunctuation(line[editor->cursorPos.textAt - !forward])) ?
                             IsPunctuation : IsAlphaNumeric;
        }
    }
    while (InRange(editor->cursorPos.textAt, 1, line.len - 1) && 
          (skipOverSpace || ShouldAdvance(line[editor->cursorPos.textAt - !forward])));
}

void MoveCursorForward(Editor* editor)
{
    if (editor->cursorPos.textAt < editor->lines[editor->cursorPos.line].len)
    {
        int prevTextIndex = editor->cursorPos.textAt;
        if (InputHeld(input.leftCtrl))
            AdvanceCursorToEndOfWord(editor, true);
        else
            editor->cursorPos.textAt++;

        if (InputHeld(input.leftShift))
            InitHighlight(editor, prevTextIndex, editor->cursorPos.line);
            
    }
    else if (editor->cursorPos.line < editor->numLines - 1)
    {
        //Go down a line
        editor->cursorPos.line++;
		editor->cursorPos.textAt = 0;
        if (InputHeld(input.leftCtrl))
            AdvanceCursorToEndOfWord(editor, true);

        if (InputHeld(input.leftShift))
            InitHighlight(editor, 
                          editor->lines[editor->cursorPos.line - 1].len, 
                          editor->cursorPos.line - 1);
    }
}

void MoveCursorBackward(Editor* editor)
{
    if (editor->cursorPos.textAt > 0)
    {
        int prevTextIndex = editor->cursorPos.textAt;
        if (InputHeld(input.leftCtrl))
            AdvanceCursorToEndOfWord(editor, false);
        else
            editor->cursorPos.textAt--;

        if (InputHeld(input.leftShift))
            InitHighlight(editor, prevTextIndex, editor->cursorPos.line);
    }
    else if (editor->cursorPos.line > 0)
    {
        //Go up a line
        editor->cursorPos.line--;
        editor->cursorPos.textAt = editor->lines[editor->cursorPos.line].len;
        if (InputHeld(input.leftCtrl) && editor->lines[editor->cursorPos.line].len > 0)
            AdvanceCursorToEndOfWord(editor, false);

        if (InputHeld(input.leftShift))
            InitHighlight(editor, 0, editor->cursorPos.line + 1);
    }
}

//TODO: When moving back to single line whilst highlighting, move back to previous editor.cursorPos.textAt
void MoveCursorUp(Editor* editor)
{
    if (editor->cursorPos.line > 0)
    {
        int prevTextIndex = editor->cursorPos.textAt; 
        editor->cursorPos.line--; 
        editor->cursorPos.textAt = min(editor->cursorPos.textAt, editor->lines[editor->cursorPos.line].len);
        if (InputHeld(input.leftShift))
            InitHighlight(editor, prevTextIndex, editor->cursorPos.line + 1);
    }
}

//TODO: When moving back to single line, move back to previous editor.cursorPos.textAt
void MoveCursorDown(Editor* editor)
{
    if (editor->cursorPos.line < editor->numLines - 1)
    {
        int prevTextIndex = editor->cursorPos.textAt; 
        editor->cursorPos.line++;
        editor->cursorPos.textAt = min(editor->cursorPos.textAt, editor->lines[editor->cursorPos.line].len);
        if (InputHeld(input.leftShift))
            InitHighlight(editor, prevTextIndex, editor->cursorPos.line - 1);
    }
}

Editor InitEditor(string fileName = lstring(""))
{
    Editor result;
    for (int i = 0; i < MAX_LINES; ++i)
        result.lines[i] = init_string_buf(LINE_CHUNK_SIZE, lineMemoryAllocator);
    result.fileName = init_string_buf(fileName);
    return result;
}

void SetTopChangedLine(Editor* editor, int newLineIndex)
{
    if (editor->topChangedLineIndex != -1)
        editor->topChangedLineIndex = min(editor->topChangedLineIndex, newLineIndex);
    else 
        editor->topChangedLineIndex = editor->cursorPos.line;
}

string_buf GetMultilineText(Editor* editor, TextSectionInfo sectionInfo, Allocator allocator = {}, bool CRCL = false)
{
    const int numLines = sectionInfo.bottom.line - sectionInfo.top.line + 1;
    
    string_buf result = init_string_buf(128, allocator);

    for (int i = 0; i < numLines; ++i)
    {
        bool isLastLine = i == numLines - 1;
        int l = i + sectionInfo.top.line;
        int lineStart = sectionInfo.top.textAt * (i == 0);
        int lineEnd = (isLastLine || numLines == 1) ? sectionInfo.bottom.textAt : editor->lines[l].len;
        result += SubString(editor->lines[l].toStr(), lineStart, lineEnd);
        if (!isLastLine) 
        {
            if (CRCL) result += '\r';
            result += '\n'; 
        }
    }

    return result;
}

void RemoveTextSection(Editor* editor, TextSectionInfo sectionInfo)
{   
    //Get highlighted text on bottom line
    string remainingBottomText = {0};
    if (!sectionInfo.spansOneLine)
    {
        remainingBottomText = SubString(editor->lines[sectionInfo.bottom.line].toStr(), 
                                        sectionInfo.bottom.textAt);
    }

    //Shift lines below highlited section up
    editor->numLines -= sectionInfo.bottom.line - sectionInfo.top.line;
    for (int i = sectionInfo.top.line + 1; i < editor->numLines; ++i)
    {
        editor->lines[i] = editor->lines[i + sectionInfo.bottom.line - sectionInfo.top.line];
    }

    //Remove Text from top line and connect bottom line text
    StringBuf_RemoveStringAt(&editor->lines[sectionInfo.top.line], 
                             sectionInfo.top.textAt, 
                             sectionInfo.topLen);
    if (!sectionInfo.spansOneLine)
        editor->lines[sectionInfo.top.line] += remainingBottomText;

    editor->cursorPos = {sectionInfo.top.textAt, sectionInfo.top.line};
    SetTopChangedLine(editor, editor->cursorPos.line);
}

TextSectionInfo GetTextSectionInfo(string_buf* lines, EditorPos start, EditorPos end)
{
    TextSectionInfo result;
    if (start.line > end.line)
    {
        result.top.textAt = end.textAt;
        result.top.line = end.line;
        result.bottom.textAt = start.textAt;
        result.bottom.line = start.line;
        result.topLen = lines[result.top.line].len - result.top.textAt;
    }
    else if (start.line < end.line)
    {
        result.top.textAt = start.textAt;
        result.top.line = start.line;
        result.bottom.textAt = end.textAt;
        result.bottom.line = end.line;
        result.topLen = lines[result.top.line].len - result.top.textAt;
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

inline void* UndoArenas_Alloc(size_t size)
{  
    return StringArena_Alloc(&editors[openEditorIndexes[currentEditorSide]].undoStringArena, size);
} 
inline void* UndoArenas_Realloc(void* block, size_t size)
{ 
    return StringArena_Realloc(&editors[openEditorIndexes[currentEditorSide]].undoStringArena, block, size);
} 
inline void UndoArenas_Free(void* block)
{
    return StringArena_Free(&editors[openEditorIndexes[currentEditorSide]].undoStringArena, block);
}
const Allocator undoArenasAllocator = {UndoArenas_Alloc, UndoArenas_Realloc, UndoArenas_Free};

void AddToUndoStack(Editor* editor, EditorPos undoStart, EditorPos undoEnd, UndoType type, bool redo = false, bool fillBuffer = true)
{
    UndoInfo undo;
    undo.start = undoStart;
    undo.end = undoEnd;
    undo.prevCursorPos = editor->cursorPos;
    undo.type = type;
    if (type == UNDOTYPE_REMOVED_TEXT_SECTION || type == UNDOTYPE_OVERWRITE)
    {
        TextSectionInfo section = GetTextSectionInfo(editor->lines, undoStart, undoEnd);
		if (fillBuffer)
		{
			undo.text = GetMultilineText(editor, section, undoArenasAllocator);
		}
        else
        {
            undo.text = init_string_buf(128, undoArenasAllocator);
        }
    }
    else if (type == UNDOTYPE_REMOVED_TEXT_REVERSE_BUFFER)
    {
        undo.text = init_string_buf(); //I don't like this at all
    }

    if (redo)
        editor->redoStack[editor->numRedos++] = undo;
    else
        editor->undoStack[editor->numUndos++] = undo;
}

void ResetUndoStack(Editor* editor, bool redo = false)
{
    UndoInfo* stack = (redo) ? editor->redoStack : editor->undoStack;
    int* numInStack = (redo) ? &editor->numRedos : &editor->numUndos;

    for (int i = 0; i < *numInStack; ++i)
    {
        if (stack[i].text.len) stack[i].text.dealloc();
    }

    *numInStack = 0;
}

//Inserts line without messing around with the cursor indicies
void InsertLineAt(Editor* editor, int lineIndex)
{
    //Shift lines down
	editor->numLines++;
    for (int i = editor->numLines - 1; i > lineIndex; --i)
    {
        editor->lines[i] = editor->lines[i-1];
    }
	editor->lines[lineIndex] = init_string_buf(LINE_CHUNK_SIZE, lineMemoryAllocator);
}

void InsertText(Editor* editor, string multilineText, EditorPos insertAt)
{
    int startOfLine = 0;
    int lineIndex = insertAt.line;
    int firstInsertLineLen = 0;
    for (int endOfLine = 0; endOfLine <= multilineText.len; ++endOfLine)
    {       
        if (endOfLine == multilineText.len || multilineText[endOfLine] == '\n')
        {
			bool IsCRCL = multilineText[endOfLine - 1] == '\r';
            int insertLen = endOfLine - startOfLine - IsCRCL;
            int insertStart = insertAt.textAt * (lineIndex == insertAt.line);
            string textToInsert = SubString(multilineText, startOfLine, endOfLine - IsCRCL);
            StringBuf_InsertString(&editor->lines[lineIndex], textToInsert, insertStart);
            
            if (endOfLine == multilineText.len) 
            {
				if (lineIndex == insertAt.line)
					editor->cursorPos.textAt += insertLen;
				else
					editor->cursorPos.textAt = insertLen;
				editor->cursorPos.line = lineIndex;

                break;
            }
            
            startOfLine = endOfLine + 1;
            if (lineIndex == insertAt.line) firstInsertLineLen = insertLen; 

            lineIndex++;
            InsertLineAt(editor, lineIndex);
        }
    }

    //If we inserted more than one line, move remainder text on firts line to last line
    if (lineIndex != insertAt.line)
    {
        string remainderText = SubString(editor->lines[insertAt.line].toStr(),
                                         insertAt.textAt + firstInsertLineLen);
        editor->lines[lineIndex] += remainderText;
        editor->lines[insertAt.line].len -= remainderText.len;
    }

    SetTopChangedLine(editor, insertAt.line);
}

//TODO: There is still a bug where 2 null characters are being written (my guess we overshooting /r/n): fix
void SaveFile(Editor* editor, string fileName)
{
    if (editor->topChangedLineIndex == -1) return; 

	bool overwrite = fileName == editor->fileName.toStr();

    EditorPos writeSectionStart = {0, editor->topChangedLineIndex * (overwrite)};
    int writeStart = 0;
    for (int i = 0; i < writeSectionStart.line * (overwrite); ++i)
        writeStart += editor->lines[i].len + 2; //TODO: Make UNIX compatible
    
    EditorPos writeSectionEnd = {editor->lines[editor->numLines - 1].len, editor->numLines - 1};
    TextSectionInfo writeTextSection = GetTextSectionInfo(editor->lines, writeSectionStart, writeSectionEnd);
    string_buf textToWrite = GetMultilineText(editor, writeTextSection, allocator_temporaryStringArena, true);
    if(WriteToFile(fileName, textToWrite.toStr(), overwrite, writeStart))
    {
        if (!overwrite) editor->fileName = fileName;
    }
    else
    {
        //Log
    }

    editor->topChangedLineIndex = -1;

    OnFileSave();
}

//TODO: Double check memory leaks
void HandleUndoInfo(Editor* editor, UndoInfo undoInfo, bool isRedo)
{
    UndoInfo* stack = (!isRedo) ? editor->redoStack : editor->undoStack;
    int* numInStack = (!isRedo) ? &editor->numRedos : &editor->numUndos; 
    TextSectionInfo sectionInfo = GetTextSectionInfo(editor->lines, undoInfo.start, undoInfo.end);

    ClearHighlights(editor);

    AddToUndoStack(editor, undoInfo.start, undoInfo.end, UNDOTYPE_ADDED_TEXT, !isRedo);
    stack[*numInStack - 1].wasHighlight = undoInfo.wasHighlight; //hnnngghhhh

    switch(undoInfo.type)
    {
        case UNDOTYPE_ADDED_TEXT:
        {
            stack[*numInStack - 1].type = UNDOTYPE_REMOVED_TEXT_SECTION;
            stack[*numInStack - 1].text = GetMultilineText(editor, sectionInfo, undoArenasAllocator);
            RemoveTextSection(editor, sectionInfo);
        } break;

        case UNDOTYPE_REMOVED_TEXT_SECTION:
        {
            stack[*numInStack - 1].type = UNDOTYPE_ADDED_TEXT;

            InsertText(editor, undoInfo.text.toStr(), sectionInfo.top);
            undoInfo.text.dealloc();
            
            if (!isRedo && undoInfo.wasHighlight)
            {
                editor->highlightStart = (undoInfo.prevCursorPos == sectionInfo.bottom) ? 
                                        sectionInfo.top : sectionInfo.bottom;
            }
        } break;

        case UNDOTYPE_REMOVED_TEXT_REVERSE_BUFFER:
        {    
            Assert(!isRedo);
            stack[*numInStack - 1].type = UNDOTYPE_ADDED_TEXT;

            int firstInsertLineLen = 0;
            EditorPos at = sectionInfo.top;
            for (int i = (int)undoInfo.text.len - 1; i >= 0; --i)
            {
                //NOTE: This should not need to check for CRCL as I wanna just use '\n' so hopefully that lasts
                if (undoInfo.text[i] == '\n')
                {
                    //editor->lines[at.line].text[at.textAt] = 0;
                    if (at.line == sectionInfo.top.line)
                        firstInsertLineLen = at.textAt - sectionInfo.top.textAt;

					at.textAt = 0;

                    at.line++;
                    InsertLineAt(editor, at.line);
                }
                else
                {
                    editor->lines[at.line] += undoInfo.text[i]; 
                    at.textAt++;
                }
            }

            //If we inserted more than one line, move remainder text on firts line to last line
            if (at.line != sectionInfo.top.line)
            {
                string remainderText = SubString(editor->lines[sectionInfo.top.line].toStr(),
                                                 sectionInfo.top.textAt + firstInsertLineLen);
                editor->lines[at.line] += remainderText;
                editor->lines[sectionInfo.top.line].len -= remainderText.len;
            }
        } break;

        case UNDOTYPE_OVERWRITE:
        {
            stack[*numInStack - 1].type = UNDOTYPE_OVERWRITE;

            RemoveTextSection(editor, sectionInfo);

            EditorPos insertStart = {sectionInfo.top.textAt, sectionInfo.top.line};
			EditorPos insertEnd = insertStart;
			int lastLineStart = IndexOfLastCharInString(undoInfo.text.toStr(), '\n') + 1;
			insertEnd.textAt += (int)undoInfo.text.len - lastLineStart;
            insertEnd.line += CharCount('\n', undoInfo.text.toStr());
            insertEnd.textAt -= insertStart.textAt * (insertEnd.line != insertStart.line); 
            
            InsertText(editor, undoInfo.text.toStr(), insertStart);

            stack[*numInStack - 1].start = insertStart;
            stack[*numInStack - 1].end = insertEnd;

            if (!isRedo)
            {
                editor->highlightStart = (undoInfo.prevCursorPos == insertEnd) ? 
                                        insertStart : insertEnd;
            }
        } break;

        //TODO: This assumes that multine cursors are all at the same text index on each line, make this handle different test indicies
        case UNDOTYPE_MULTILINE_ADD:
        {
            stack[*numInStack - 1].type = UNDOTYPE_MULTILINE_REMOVE;
			stack[*numInStack - 1].text = undoInfo.text;

            int lineIndex = sectionInfo.top.line;
            int removeStart = 0;
            for (int i = 0; i <= undoInfo.text.len; ++i)
            {
                //NOTE: This should not need to check for CRCL as I wanna just use '\n' so hopefully that lasts
                if (undoInfo.text[i] == '\n' || i == undoInfo.text.len)
                {
                    StringBuf_RangeRemove(&editor->lines[lineIndex], 
                                          sectionInfo.top.textAt, 
                                          i - removeStart);
                    removeStart = i + 1;
                    lineIndex++;
                }
            }
        } break;

        //TODO: This assumes that multine cursors are all at the same text index on each line, make this handle different test indicies
        case UNDOTYPE_MULTILINE_REMOVE:
        {
            stack[*numInStack - 1].type = UNDOTYPE_MULTILINE_ADD;
			stack[*numInStack - 1].text = undoInfo.text;

            int lineIndex = sectionInfo.top.line;
            int insertStart = 0;
            for (int i = 0; i <= undoInfo.text.len; ++i)
            {
                //NOTE: This should not need to check for CRCL as I wanna just use '\n' so hopefully that lasts
                if (undoInfo.text[i] == '\n' || i == undoInfo.text.len)
                {
                    string insertedText = SubString(undoInfo.text.toStr(), insertStart, i);
                    StringBuf_InsertString(&editor->lines[lineIndex], 
                                           insertedText, 
                                           sectionInfo.top.textAt);
                    insertStart = i + 1;
                    lineIndex++;
                }
            }
        } break;
    }
     
    editor->cursorPos = undoInfo.prevCursorPos;
}

inline bool MouseOnLeftSide()
{
    return input.mousePixelPos.x < screenBuffer.width / 2; 
}

EditorPos GetEditorPosAtMouse()
{
    Editor* editor = &editors[openEditorIndexes[currentEditorSide]];

    EditorPos result;
    int lineY = screenBuffer.height - input.mousePixelPos.y + editor->textOffset.y;
    int mouseLine = lineY / (fontData.maxHeight + fontData.lineGap);
    result.line = min(mouseLine, editor->numLines - 1);
    
    int linePixLen = GetCurrentEditorTextStart().x;
    result.textAt = 0;
    string_buf line = editor->lines[result.line];
    while (linePixLen < input.mousePixelPos.x && result.textAt < line.len)
    {
        linePixLen += fontData.chars[line[result.textAt]].advance;
        result.textAt++;
    }

    return result;
}

void HighlightWordAt(Editor* editor, EditorPos pos)
{
    char startingChar = editor->lines[pos.line][pos.textAt];
    if (startingChar == 0) return;

    int highlightTextAtStart = pos.textAt;
    int newCursorTextAt = pos.textAt;
    string_buf line = editor->lines[pos.line]; 

    bool (*correctChar)(char) = IsPunctuation; 
    if (IsAlphaNumeric(startingChar)) correctChar = IsAlphaNumeric;
    else if (IsWhiteSpace(startingChar)) correctChar = IsWhiteSpace;

    while (correctChar(line[highlightTextAtStart]) && highlightTextAtStart > 0)
        highlightTextAtStart--;

    while(correctChar(line[newCursorTextAt]) && newCursorTextAt < line.len)
        newCursorTextAt++;

	editor->highlightStart = {highlightTextAtStart + (highlightTextAtStart > 0), pos.line};
	editor->cursorPos = {newCursorTextAt, pos.line};
}

Token GetTokenAtCursor(EditorPos cursorPos)
{
    for (int i = 0; i < tokenInfos[openEditorIndexes[currentEditorSide]].numTokens; ++i)
    {
        Token token = tokenInfos[openEditorIndexes[currentEditorSide]].tokens[i]; 

        if (token.at.line > cursorPos.line) break;

        if (token.at.line == cursorPos.line)
        {
            int tokenEnd = token.at.textAt + token.text.len;
            if (InRange(cursorPos.textAt, token.at.textAt, tokenEnd))
                return tokenInfos[openEditorIndexes[currentEditorSide]].tokens[i];
        }
    }

    return Token{EditorPos{-1, -1}, string{0, 0}, TOKEN_UNKNOWN};
}

//
//KEY-MAPPED EDITOR FUNCTIONS
//

void AddChar(Editor* editor)
{
    string_buf* line = &editor->lines[editor->cursorPos.line];
    char charAtCursor = (*line)[editor->cursorPos.textAt];
    char prevChar = (*line)[editor->cursorPos.textAt - 1];

    //TODO: Track nested brackets??
    if (IsBackwardsBracket(editor->currentChar) && IsBackwardsBracket(charAtCursor) && 
        IsForwardsBracket(prevChar))
    {
        editor->cursorPos.textAt++;
        return;
    }

    UndoInfo* currentUndo = &editor->undoStack[editor->numUndos - 1];

    char prevPrevChar = (*line)[editor->cursorPos.textAt - 2];
    bool startOfNewWord = (editor->currentChar == ' ' && prevChar != ' ') || 
        (editor->currentChar != ' ' && prevChar == ' ' && prevPrevChar == ' ');

    if (startOfNewWord || editor->currentChar == '\t' || currentUndo->type != UNDOTYPE_ADDED_TEXT || 
        line->len == 0 || editor->highlightStart.textAt != -1 || 
        editor->cursorPos != currentUndo->end)
    {
        AddToUndoStack(editor, editor->cursorPos, editor->cursorPos, UNDOTYPE_ADDED_TEXT);
        ResetUndoStack(editor, true);
    }

    if (editor->highlightStart.textAt != -1)
    {
        TextSectionInfo highlightInfo = 
            GetTextSectionInfo(editor->lines, editor->highlightStart, editor->cursorPos);
        editor->undoStack[editor->numUndos - 1].type = UNDOTYPE_OVERWRITE;
        editor->undoStack[editor->numUndos - 1].text = GetMultilineText(editor, highlightInfo, undoArenasAllocator);
        
		RemoveTextSection(editor, highlightInfo);
        editor->undoStack[editor->numUndos - 1].start = editor->cursorPos;
		line = &editor->lines[editor->cursorPos.line]; //cursor has moved so get it again
    }

    int numCharsAdded = 0; 
    char textToInsert[5]; //this'll likely never need to be greater than 5
    bool addedTwoCharacters = false;
    switch(editor->currentChar)
    {
        case '\t':
        {
            numCharsAdded = 4 - editor->cursorPos.textAt % 4;
            for (int i = 0; i < numCharsAdded; ++i)
                textToInsert[i] = ' ';
        } break;

        case '{':
        case '[':
        case '(':
        {
            numCharsAdded = 2;
            textToInsert[0] = editor->currentChar;
            textToInsert[1] = GetOtherBracket(editor->currentChar);
            addedTwoCharacters = true;
        } break;

        case '\'':
        case '"':
        {
            numCharsAdded = 1;
            textToInsert[0] = editor->currentChar;
            
            Token tokenAtCursor = GetTokenAtCursor(editor->cursorPos);
            if (tokenAtCursor.type != TOKEN_STRING && 
                (editor->currentChar == '"' || tokenAtCursor.type != TOKEN_COMMENT))
            {
                numCharsAdded++;
                textToInsert[1] = editor->currentChar;
                addedTwoCharacters = true;
            }
        } break;

        default:
        {
            numCharsAdded = 1;
            textToInsert[0] = editor->currentChar;
        } break;
    } 
    Assert(numCharsAdded > 0);
    textToInsert[numCharsAdded] = 0;
    StringBuf_InsertString(&editor->lines[editor->cursorPos.line], 
                           textToInsert, 
                           editor->cursorPos.textAt);

    editor->cursorPos.textAt += (editor->currentChar == '\t') ? numCharsAdded : 1;

    editor->undoStack[editor->numUndos - 1].end = editor->cursorPos;
    if (addedTwoCharacters) editor->undoStack[editor->numUndos - 1].end.textAt++;

    SetTopChangedLine(editor, editor->cursorPos.line);
}

void RemoveChar(Editor* editor)
{
    string_buf* line = &editor->lines[editor->cursorPos.line];
    UndoInfo* currentUndo = &editor->undoStack[editor->numUndos - 1];
	string_buf* undoReverseBuffer = &editor->undoStack[editor->numUndos - 1].text;

    //TODO: Fix bug where when a deleted quote is undone, it appears in the wrong spot
    if (currentUndo->type != UNDOTYPE_REMOVED_TEXT_REVERSE_BUFFER || 
        editor->cursorPos != currentUndo->end)
    {
        AddToUndoStack(editor, editor->cursorPos, editor->cursorPos, UNDOTYPE_REMOVED_TEXT_REVERSE_BUFFER);
		undoReverseBuffer = &editor->undoStack[editor->numUndos - 1].text;
        ResetUndoStack(editor, true);
    }

    if (editor->cursorPos.textAt > 0)
    {
        editor->cursorPos.textAt--; 
        *undoReverseBuffer += (*line)[editor->cursorPos.textAt];
		StringBuf_RemoveAt(line, editor->cursorPos.textAt);
    }
    else if (editor->numLines > 1 && editor->cursorPos.line > 0)
    {
        *undoReverseBuffer += '\n';

        editor->cursorPos.textAt = editor->lines[editor->cursorPos.line - 1].len;
        
        editor->lines[editor->cursorPos.line - 1] += editor->lines[editor->cursorPos.line].toStr();

		//Shift lines up
		for (int i = editor->cursorPos.line; i < editor->numLines; ++i)
		{
			editor->lines[i] = editor->lines[i + 1];
		}
		editor->numLines--;

        editor->cursorPos.line--;
    }

    editor->undoStack[editor->numUndos - 1].end = editor->cursorPos;

    //undoReverseBuffer->str[undoReverseBuffer->len] = 0;

    SetTopChangedLine(editor, editor->cursorPos.line);
}

void Backspace(Editor* editor)
{
    if (editor->highlightStart.textAt != -1)
    {
        AddToUndoStack(editor, editor->highlightStart, editor->cursorPos, UNDOTYPE_REMOVED_TEXT_SECTION);
        editor->undoStack[editor->numUndos - 1].wasHighlight = true;
        TextSectionInfo highlightInfo = GetTextSectionInfo(editor->lines, 
                                                           editor->highlightStart, 
                                                           editor->cursorPos);
        editor->undoStack[editor->numUndos - 1].text = GetMultilineText(editor, highlightInfo, undoArenasAllocator);
        ResetUndoStack(editor, true);
        
        RemoveTextSection(editor, highlightInfo);
        ClearHighlights(editor);
    }
    else
    {
        RemoveChar(editor);
    }
}

void UnTab(Editor* editor)
{
    TextSectionInfo highlight;
    highlight.bottom.line = -1;
    int lineAt = editor->cursorPos.line;
    bool isMultiline = editor->highlightStart.line != -1;
    if (isMultiline)
    {
        highlight = GetTextSectionInfo(editor->lines, editor->highlightStart, editor->cursorPos);
        lineAt = highlight.top.line;
    }

    AddToUndoStack(editor, {-1, editor->cursorPos.line}, {-1, editor->cursorPos.line}, UNDOTYPE_REMOVED_TEXT_SECTION, false, false);
    editor->undoStack[editor->numUndos - 1].text.len = 0;

    int undoTextAt = 0;
    do
    {
        int numSpacesAtFront = 0;
        while (editor->lines[lineAt][numSpacesAtFront] == ' ')
            numSpacesAtFront++;
        int numRemoved = (numSpacesAtFront - 1) % 4 + 1;

		if (lineAt == editor->cursorPos.line)
		{
			editor->undoStack[editor->numUndos - 1].start.textAt = 0;
			editor->undoStack[editor->numUndos - 1].end.textAt = 0;
		}

        if (numSpacesAtFront > 0)
        {
            int destIndex = numSpacesAtFront - numRemoved;
            StringBuf_RemoveStringAt(&editor->lines[lineAt], destIndex, numRemoved);

            editor->undoStack[editor->numUndos - 1].start.textAt = 
                min(destIndex, editor->undoStack[editor->numUndos - 1].start.textAt);
            //If first line, set undo positions accordingly
            if (lineAt == editor->cursorPos.line) 
            {
                editor->undoStack[editor->numUndos - 1].start.textAt = destIndex;
                editor->undoStack[editor->numUndos - 1].end.textAt = numSpacesAtFront;
            }
            editor->undoStack[editor->numUndos - 1].start.textAt = 
                min(destIndex, editor->undoStack[editor->numUndos - 1].start.textAt);

            if (editor->cursorPos.line == lineAt && editor->cursorPos.textAt > destIndex)
                editor->cursorPos.textAt -= min(numRemoved, editor->cursorPos.textAt - destIndex);
            if (editor->highlightStart.line == lineAt)
                editor->highlightStart.textAt -= min(numRemoved, editor->highlightStart.textAt - destIndex);

            SetTopChangedLine(editor, lineAt);
        }

        for (int i = undoTextAt; i < undoTextAt + numRemoved; ++i)
            editor->undoStack[editor->numUndos - 1].text += ' ';
		editor->undoStack[editor->numUndos - 1].text += '\n';
		undoTextAt += numRemoved;

        lineAt++;
    } while (lineAt <= highlight.bottom.line);

    //If more than one line, set proper undo info
    if (isMultiline)
    {
        editor->undoStack[editor->numUndos - 1].type = UNDOTYPE_MULTILINE_REMOVE;
        editor->undoStack[editor->numUndos - 1].end.line = highlight.bottom.line;
    }
}

void Enter(Editor* editor)
{
    if (editor->numLines >= MAX_LINES) return;


    AddToUndoStack(editor, editor->cursorPos, editor->cursorPos, UNDOTYPE_ADDED_TEXT);
    ResetUndoStack(editor, true);
    if (editor->highlightStart.textAt != -1)
    {
        TextSectionInfo highlightInfo = GetTextSectionInfo(editor->lines, 
                                                           editor->highlightStart, 
                                                           editor->cursorPos);
        editor->undoStack[editor->numUndos - 1].type = UNDOTYPE_OVERWRITE;
        editor->undoStack[editor->numUndos - 1].text = GetMultilineText(editor, highlightInfo, undoArenasAllocator);
        //editor->undoStack[editor->numUndos - 1].numLines = 
        //    highlightInfo.bottom.line - highlightInfo.top.line;

        RemoveTextSection(editor, highlightInfo);
    }

    int prevLineIndex = editor->cursorPos.line;
    editor->cursorPos.line++;

    InsertLineAt(editor, editor->cursorPos.line);
    
    int copiedLen = editor->lines[prevLineIndex].len - editor->cursorPos.textAt;
    editor->lines[editor->cursorPos.line] = SubStringAt(editor->lines[prevLineIndex].toStr(),
                                                        editor->cursorPos.textAt,
                                                        copiedLen);
    StringBuf_RemoveStringAt(&editor->lines[prevLineIndex], 
                                editor->cursorPos.textAt, 
                                copiedLen);

    editor->cursorPos.textAt = 0;

    //TODO: Refactor this is hacky
    if (editor->topChangedLineIndex == -1)
        editor->topChangedLineIndex = prevLineIndex;
    else 
        SetTopChangedLine(editor, prevLineIndex);
    

    editor->undoStack[editor->numUndos - 1].end = editor->cursorPos;
}

void HighlightCurrentLine(Editor* editor)
{
    if (editor->highlightStart.textAt == -1)
    { 
        Assert(editor->highlightStart.line == -1);
        editor->highlightStart.textAt = 0;
        editor->highlightStart.line = editor->cursorPos.line;
    }

    if (editor->cursorPos.line < editor->numLines - 1)
    {
        editor->cursorPos.textAt = 0;
        editor->cursorPos.line += editor->cursorPos.line + 1 < MAX_LINES;
    }
    else 
    {
        editor->cursorPos.textAt = editor->lines[editor->cursorPos.line].len;
    }
}

void HighlightEntireFile(Editor* editor)
{
    editor->highlightStart.textAt = 0;
    editor->highlightStart.line = 0;
    editor->cursorPos.textAt = editor->lines[editor->numLines - 1].len;
    editor->cursorPos.line = editor->numLines - 1;
}

void RemoveCurrentLine(Editor* editor)
{
    int removedLine = editor->cursorPos.line;
    bool isLastLine = removedLine == editor->numLines - 1;

    EditorPos undoStart = {editor->lines[removedLine - 1].len, removedLine - 1}; //This is to make undo actually insert a new line. A tad annoying but hey.
    EditorPos undoEnd = {editor->lines[removedLine].len, removedLine};
    AddToUndoStack(editor, undoStart, undoEnd, UNDOTYPE_REMOVED_TEXT_SECTION);

    for (int i = removedLine + 1; i < editor->numLines; ++i)
        editor->lines[i-1] = editor->lines[i];
    
    if (editor->numLines == 1)
    {
        editor->lines[removedLine].len = 0;
        //editor->lines[removedLine][0] = 0;
        editor->cursorPos.textAt = 0;
    }
    else 
    {
        editor->cursorPos.line -= isLastLine;
        editor->cursorPos.textAt = 
            min(editor->lines[editor->cursorPos.line].len, editor->cursorPos.textAt);
    }

    //This needs to happen here so that the above if statement works properly
    editor->numLines -= (editor->numLines != 1);

    ClearHighlights(editor);
    
    SetTopChangedLine(editor, editor->cursorPos.line);
}


void CopyHighlightedText(Editor* editor)
{
    if (editor->highlightStart.textAt == -1) 
    {
        Assert(editor->highlightStart.line == -1);
        return;
    }

    TextSectionInfo highlightInfo = GetTextSectionInfo(editor->lines, 
                                                       editor->highlightStart, 
                                                       editor->cursorPos);
    string_buf copiedText = GetMultilineText(editor, highlightInfo, allocator_temporaryStringArena, true);
    CopyToClipboard(copiedText.toStr());
}

void Paste(Editor* editor)
{
    string textToPaste = GetClipboardText();
    if (textToPaste.str != nullptr)
    {
        AddToUndoStack(editor, editor->cursorPos, editor->cursorPos, UNDOTYPE_ADDED_TEXT);
        ResetUndoStack(editor, true);

		if (editor->highlightStart.textAt != -1)
        {
            Assert(editor->highlightStart.line != -1);
            
            TextSectionInfo highlightInfo = 
                GetTextSectionInfo(editor->lines, editor->highlightStart, editor->cursorPos); 

			editor->undoStack[editor->numUndos - 1].start = highlightInfo.top;
            editor->undoStack[editor->numUndos - 1].type = UNDOTYPE_OVERWRITE;
            editor->undoStack[editor->numUndos - 1].text = GetMultilineText(editor, highlightInfo, undoArenasAllocator);
            
            RemoveTextSection(editor, highlightInfo);
            ClearHighlights(editor);
        }

        InsertText(editor, textToPaste, editor->cursorPos);

        editor->undoStack[editor->numUndos - 1].end = editor->cursorPos;
    } 
}

//TODO: Investigate Performance of this
void CutHighlightedText(Editor* editor)
{
    CopyHighlightedText(editor);
    
    AddToUndoStack(editor, editor->highlightStart, editor->cursorPos, UNDOTYPE_REMOVED_TEXT_SECTION);
    TextSectionInfo highlightInfo = GetTextSectionInfo(editor->lines, 
                                                       editor->highlightStart, 
                                                       editor->cursorPos);
    editor->undoStack[editor->numUndos - 1].text = GetMultilineText(editor, highlightInfo, undoArenasAllocator);
    ResetUndoStack(editor, true);

    RemoveTextSection(editor, GetTextSectionInfo(editor->lines, 
                                                 editor->highlightStart, 
                                                 editor->cursorPos));
    ClearHighlights(editor);
}

void SaveAs(Editor* editor)
{
	string fileName = ShowFileDialogAndGetFileName(true);
    if (fileName.str) SaveFile(editor, fileName);
}

void Save(Editor* editor)
{
    if (editor->fileName.len > 0)
		SaveFile(editor, editor->fileName.toStr());
	else
    {
        Assert(editor->topChangedLineIndex != -1);
		SaveAs(editor);
    }
}

void Undo(Editor* editor)
{
    if (editor->numUndos == 0) return;

    UndoInfo undoInfo = editor->undoStack[--editor->numUndos];
    HandleUndoInfo(editor, undoInfo, false);
}

void Redo(Editor* editor)
{
    if (editor->numRedos == 0) return;

    UndoInfo redoInfo = editor->redoStack[--editor->numRedos];
    HandleUndoInfo(editor, redoInfo, true);
}

//
//KEY-MAPPED VOID FUNCTIONS
//

void ZoomIn()
{
    fontData.sizeIndex = min(fontData.sizeIndex + 1, StackArrayLen(fontSizes) - 1); 
    ResizeFont(fontData.sizeIndex);
}

void ZoomOut()
{
    fontData.sizeIndex = max(fontData.sizeIndex - 1, 0); 
    ResizeFont(fontData.sizeIndex - 1);
}

//TODO: Resize editor.lines if file too big + maybe return success bool?
void TE_OpenFile()
{
    if (numEditors >= 3) return;

    //Allocating twice on heap here, don't think it should be massive performance hit but kinda sketchy
    string fileName = ShowFileDialogAndGetFileName(false);
    string file = ReadEntireFileAsString(fileName);
    if (file.str)
    {
        currentEditorSide = numEditors == 1 || currentEditorSide;
        openEditorIndexes[currentEditorSide] = numEditors;
        numEditors++;

        const int currentEditorIndex = openEditorIndexes[currentEditorSide];
        editors[currentEditorIndex] = InitEditor(fileName);

        char* startOfFile = file.str;

        editors[currentEditorIndex].numLines = 0;
        string line = GetNextLine(&file);
        while(line[0])
        {
            editors[currentEditorIndex].lines[editors[currentEditorIndex].numLines] = line;
            editors[currentEditorIndex].numLines++;

            line = GetNextLine(&file);
        }

        FreeWin32(startOfFile);

        ResetUndoStack(&editors[currentEditorIndex]);
        ResetUndoStack(&editors[currentEditorIndex], true);

        editors[currentEditorIndex].topChangedLineIndex = -1;
        editors[currentEditorIndex].cursorPos = {0, 0};

        OnFileOpen();
    }
}

void NewEditor()
{
    if (numEditors >= 3) return;

    editors[numEditors++] = InitEditor();
    editors[numEditors - 1].lines[0] = "Type your text here.";
    editors[numEditors - 1].cursorPos = EditorPos{editors[numEditors - 1].lines[0].len, 0};
    
	if (numEditors == 2) currentEditorSide = 1;
    openEditorIndexes[currentEditorSide] = numEditors - 1; 
}

void SelectNextEditor()
{  
    openEditorIndexes[currentEditorSide] = (openEditorIndexes[currentEditorSide] + 1) % numEditors; 

    OnEditorSwitch();
}

void SelectPrevEditor()
{ 
    openEditorIndexes[currentEditorSide] = (openEditorIndexes[currentEditorSide] > 0) ? 
                                             openEditorIndexes[currentEditorSide] - 1 : numEditors - 1;

    OnEditorSwitch();
}

//
//MAIN LOOP/STUFF
//

TimedEvent cursorBlink = {0.5f};
TimedEvent holdChar = {0.5f};
TimedEvent repeatChar = {0.02f, {true, &AddChar}};
TimedEvent holdAction = {0.5f};
TimedEvent repeatAction = {0.02f};

//TODO: Move multiclicks of this into input
int numMultiClicks = 0;
bool doubleClicked = false;
TimedEvent doubleClick = {0.5f};
EditorPos prevMousePos = {-1, -1};

bool capslockOn = false;
bool nonCharKeyPressed = false;

//TODO: Generalise all this stuff into a single struct,would be simpler imo
KeyBinding nonCharKeyBindings[] =
{
    //These modify
    {NONE, INPUTCODE_ENTER, {true, Enter}},
    {NONE, INPUTCODE_BACKSPACE, {true, Backspace}},

    //These do not modify
    {NONE, INPUTCODE_RIGHT, {true, MoveCursorForward}},
    {NONE, INPUTCODE_LEFT, {true, MoveCursorBackward}},
    {NONE, INPUTCODE_UP, {true, MoveCursorUp}},
    {NONE, INPUTCODE_DOWN, {true, MoveCursorDown}}
};

KeyBinding commandBindings[] = 
{
    //These modify
    {CTRL | SHIFT, INPUTCODE_L, {true, RemoveCurrentLine}},
    {CTRL, INPUTCODE_V, {true, Paste}},
    {CTRL, INPUTCODE_X, {true, CutHighlightedText}},
    {CTRL, INPUTCODE_Y, {true, Redo}},
    {CTRL, INPUTCODE_Z, {true, Undo}},

    //These do not modify
    {CTRL, INPUTCODE_A, {true, HighlightEntireFile}},
    {CTRL, INPUTCODE_C, {true, CopyHighlightedText}},
    {CTRL, INPUTCODE_L, {true, HighlightCurrentLine}},
    {CTRL, INPUTCODE_S, {true, Save}},
    {CTRL | SHIFT, INPUTCODE_S, {true, SaveAs}},

    //Non-editor bindings
    //idk why I have to cast the function pointers, frikkin c++
    {CTRL, INPUTCODE_N, {false, (EditorFunc)NewEditor}},
    {CTRL, INPUTCODE_O, {false, (EditorFunc)TE_OpenFile}},
    {ALT,  INPUTCODE_RIGHT, {false, (EditorFunc)SelectNextEditor}},
    {ALT,  INPUTCODE_LEFT,  {false, (EditorFunc)SelectPrevEditor}},
    {CTRL, INPUTCODE_MINUS,  {false, (EditorFunc)ZoomOut}},
    {CTRL, INPUTCODE_EQUALS, {false, (EditorFunc)ZoomIn}}
};

void Init()
{   
    LoadTokenColours();

    editors[0] = InitEditor();

    for (int i = 0; i < 3; ++i)
        tokenInfos[i] = InitTokenInfo();
}

void Draw(float dt)
{
    Editor* currentEditor = &editors[openEditorIndexes[currentEditorSide]];

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

                currentEditor->currentChar = charOfKeyPressed;
                if (charOfKeyPressed) AddChar(currentEditor);

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
                    currentEditor->currentChar = charOfKeyPressed;
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
        InputHeld(input.flags[CharToInputCode(currentEditor->currentChar)]))
    {
        if (newCharKeyPressed)
            holdChar.elapsedTime = 0.0f;
        HandleTimedEvent(&holdChar, dt, &repeatChar, currentEditor);

        ClearHighlights(currentEditor);

        OnTextChanged();
    }
    else
    {
        holdChar.elapsedTime = 0.0f;

        bool inputHeld = false;
        for (int i = 0; i < StackArrayLen(nonCharKeyBindings); ++i)
        {
            if (InputDown(input.flags[nonCharKeyBindings[i].mainKey]))
            {
                repeatAction.onTrigger = nonCharKeyBindings[i].callback;
                nonCharKeyBindings[i].callback.editorFunc(currentEditor); //TODO: check for both funcs if needed
                holdAction.elapsedTime = 0.0f;

                //NOTE: This i < 2 check may get bad in the future
                if (i < 2 || !InputHeld(input.leftShift))
                {
                    OnTextChanged();
                    ClearHighlights(currentEditor);
                }
            }
            else if (InputHeld(input.flags[nonCharKeyBindings[i].mainKey]))
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
                repeatAction.onTrigger.editorFunc = UnTab;
                //repeatAction.onTrigger.isEditorFunc = true;
                UnTab(currentEditor);
                holdAction.elapsedTime = 0.0f;
            }
            else if (InputHeld(input.tab))
            {
                inputHeld = true;
            }   
        }

        if (inputHeld)
            HandleTimedEvent(&holdAction, dt, &repeatAction, currentEditor);
        else
            holdAction.elapsedTime = 0.0f;


        if (InputDown(input.capsLock))
            capslockOn = !capslockOn;

        if (InputDown(input.f5))
        {
            free(userSettings.fontFile.str); //TODO: Get this free outta here once we have more than one string
            userSettings = LoadUserSettingsFromConfigFile();
            ResizeFont(fontData.sizeIndex); //Rename function perhaps?
        }


        if (InputDown(input.leftMouse))
        {
            ClearHighlights(currentEditor);
            currentEditorSide = !MouseOnLeftSide();
            currentEditor = &editors[openEditorIndexes[currentEditorSide]];
            currentEditor->cursorPos = GetEditorPosAtMouse();
    
            if (numMultiClicks > 0 && prevMousePos == currentEditor->cursorPos)
            {
                //Technically there's a glitch here where if someone clicks more than
                //INT_MAX - 1 times this will repeat. I don't think it's worth fixing
                //though as there's no actual reason I can think of where this bug
                //is problematic
                switch (numMultiClicks)
                {
                    case 1:
                        HighlightWordAt(currentEditor, currentEditor->cursorPos);
                        break;

                    case 2:
                        HighlightCurrentLine(currentEditor);
                        break;

                    default:
						Assert(numMultiClicks > 0);
                        HighlightEntireFile(currentEditor);
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
            InitHighlight(currentEditor, currentEditor->cursorPos.textAt, currentEditor->cursorPos.line);
            if (!doubleClicked) currentEditor->cursorPos = GetEditorPosAtMouse();
        }

        if (InputUp(input.leftMouse))
            doubleClicked = false;


        if (!inputHeld)
        {
            for (int i = 0; i < StackArrayLen(commandBindings); ++i)
            {
                if (KeyCommandDown(commandBindings[i]))
                {
                    if (commandBindings[i].callback.isEditorFunc)
                        commandBindings[i].callback.editorFunc(currentEditor);
                    else
                        commandBindings[i].callback.voidFunc();

                    //TODO: Find a better way than this stupid if check
                    if (i < 5) OnTextChanged();

                    break;
                }
            }
        }
    }    

    //Only caught this bug when using mouse but putting it here since may save my back in other situations
    if (currentEditor->highlightStart == currentEditor->cursorPos)
        ClearHighlights(currentEditor);

    doubleClick.elapsedTime += dt * (numMultiClicks);
    if (doubleClick.elapsedTime >= doubleClick.interval)
    {
        numMultiClicks = 0;
        doubleClick.elapsedTime = 0.0f;
    }

    //Draw Background
    Rect screenDims = {0, screenBuffer.width, 0, screenBuffer.height};
    DrawRect(screenDims, userSettings.backgroundColour);

    IntPair currentCursorDrawPos = {};
    
    for (int e = 0; e < min(2, numEditors); ++e)
    {
        Editor* editor = &editors[openEditorIndexes[e]]; 
        
        const IntPair textStart = (e == 0) ? GetLeftTextStart() : GetRightTextStart();
        const Rect textLimits = (e == 0) ? GetLeftTextLimits() : GetRightTextLimits();

        //Get correct position for cursor
        IntPair cursorDrawPos = textStart;
        for (int i = 0; i < editor->cursorPos.textAt; ++i)
            cursorDrawPos.x += fontData.chars[editor->lines[editor->cursorPos.line][i]].advance;
        cursorDrawPos.y -= editor->cursorPos.line * (int)(fontData.maxHeight + fontData.lineGap);
        cursorDrawPos.y -= fontData.offsetBelowBaseline;

        //All this shit here seems very dodgy, maybe refactor or just find a better method
        if (cursorDrawPos.x >= textLimits.right)
            editor->textOffset.x = max(editor->textOffset.x, cursorDrawPos.x - textLimits.right);
        else
            editor->textOffset.x = 0;

        char cursorChar = editor->lines[editor->cursorPos.line][editor->cursorPos.textAt];
        int xLeftLimit = textStart.x + editor->textOffset.x;
        if (cursorDrawPos.x < xLeftLimit)
            editor->textOffset.x -= fontData.chars[cursorChar].advance;
	    cursorDrawPos.x -= editor->textOffset.x;

        if (cursorDrawPos.y < textLimits.bottom)
            editor->textOffset.y = max(editor->textOffset.y, textLimits.bottom - cursorDrawPos.y);
        else
            editor->textOffset.y = 0;

        int yTopLimit = textStart.y - editor->textOffset.y;
        if (cursorDrawPos.y > yTopLimit)
            editor->textOffset.y -= (int)(fontData.maxHeight + fontData.lineGap);
        cursorDrawPos.y += editor->textOffset.y;

        if (e == currentEditorSide)
        {
            currentCursorDrawPos = cursorDrawPos;

            //Draw Line Background
            Rect lineBackgroundDims = 
            {
                textLimits.left, 
                textLimits.right, 
                cursorDrawPos.y, 
                cursorDrawPos.y + (int)(fontData.maxHeight + fontData.lineGap)
            };
            DrawRect(lineBackgroundDims, userSettings.lineBackgroundColour);
        }

        //Draw all of the text on screen
        int numLinesOnScreen = screenBuffer.height / (int)(fontData.maxHeight + fontData.lineGap);
        int firstLine = abs(editor->textOffset.y) / (int)(fontData.maxHeight + fontData.lineGap);
        for (int i = firstLine; i < editor->numLines && i < firstLine + numLinesOnScreen; ++i)
        {
            //Draw text
            int x = textStart.x - editor->textOffset.x;
            int y = textStart.y - i * (int)(fontData.maxHeight + fontData.lineGap) + editor->textOffset.y;
            DrawText(editor->lines[i].toStr(), x, y, userSettings.defaultTextColour, textLimits);

            //Draw Line num
            char lineNumText[8];
            IntToString(i + 1, lineNumText);
            DrawText(cstring(lineNumText), 
                     textStart.x - TextPixelLength(cstring(lineNumText)) - LINE_NUM_OFFSET, 
                     y, 
                     userSettings.lineNumColour, 
                     {0, 0, textLimits.bottom, 0});
        }
    }
    
    const IntPair textStart = GetCurrentEditorTextStart();
    const Rect textLimits = GetCurrentEditorTextLimits();

    //Draw highlights
    if (currentEditor->highlightStart.textAt != -1) 
    {
        Assert(currentEditor->highlightStart.line != -1);
        TextSectionInfo highlightInfo = GetTextSectionInfo(currentEditor->lines, currentEditor->highlightStart, currentEditor->cursorPos);
        
        //Draw top line highlight
        char* topHighlightText = 
            currentEditor->lines[highlightInfo.top.line].str + highlightInfo.top.textAt;
        const int topHighlightPixelLength = 
            TextPixelLength(topHighlightText, highlightInfo.topLen);
		int topXOffset = 
            TextPixelLength(currentEditor->lines[highlightInfo.top.line].str, highlightInfo.top.textAt);
        int topX = textStart.x + topXOffset - currentEditor->textOffset.x;
        int topY = textStart.y - highlightInfo.top.line * (int)(fontData.maxHeight + fontData.lineGap) 
                   - PIXELS_UNDER_BASELINE + currentEditor->textOffset.y;
        if (currentEditor->lines[highlightInfo.top.line].len == 0) topXOffset = fontData.chars[' '].advance;
        DrawAlphaRect(
            {topX, topX + topHighlightPixelLength, topY, topY + (int)(fontData.maxHeight + fontData.lineGap)},
            userSettings.highlightColour, 
            textLimits
        );

        //Draw inbetween highlights
        for (int i = highlightInfo.top.line + 1; i < highlightInfo.bottom.line; ++i)
        {
            int highlightedPixelLength = TextPixelLength(currentEditor->lines[i].toStr()); 
            if (currentEditor->lines[i].len == 0) highlightedPixelLength = fontData.chars[' '].advance;
            int x = textStart.x - currentEditor->textOffset.x;
            int y = textStart.y - i * (int)(fontData.maxHeight + fontData.lineGap) 
                    - PIXELS_UNDER_BASELINE + currentEditor->textOffset.y;
            DrawAlphaRect(
                {x, x + highlightedPixelLength, y, y + (int)(fontData.maxHeight + fontData.lineGap)}, 
                userSettings.highlightColour, 
                textLimits
            );
        }

        //Draw bottom line highlight
        if (!highlightInfo.spansOneLine)
        {
            int bottomHighlightPixelLength = 
                TextPixelLength(currentEditor->lines[highlightInfo.bottom.line].str, 
                                highlightInfo.bottom.textAt);
            int bottomX = textStart.x - currentEditor->textOffset.x;
            int bottomY = textStart.y - highlightInfo.bottom.line * (int)(fontData.maxHeight + fontData.lineGap) 
                          - PIXELS_UNDER_BASELINE + currentEditor->textOffset.y;
            if (currentEditor->lines[highlightInfo.bottom.line].len == 0) 
                bottomHighlightPixelLength = fontData.chars[' '].advance;
            DrawAlphaRect(
                {
                    bottomX, bottomX + bottomHighlightPixelLength, 
                    bottomY, bottomY + (int)(fontData.maxHeight + fontData.lineGap)
                }, 
                userSettings.highlightColour, 
                textLimits
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

        Rect cursorDims = {currentCursorDrawPos.x, currentCursorDrawPos.x + 2, //TODO: Make width scale with font size
                           currentCursorDrawPos.y, currentCursorDrawPos.y + (int)(fontData.maxHeight + fontData.lineGap)};
        DrawRect(cursorDims, userSettings.cursorColour);
    }

    HighlightSyntax();
}