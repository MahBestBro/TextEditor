#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h> 
#include <windowsx.h>
#include <commdlg.h>

#include "stdio.h"

#include "stb_truetype.h"

#include "TextEditor_defs.h"
#include "TextEditor_alloc.h"
#include "TextEditor_string.h"
#include "TextEditor_input.h"
#include "TextEditor_font.h"
#include "TextEditor.h"
#include "TextEditor_meta.h"
#include "TextEditor_config.h"
#include "TextEditor_tokeniser.h"

#include "TextEditor_alloc.cpp"

DEF_STRING_ARENA_FUNCS(temporaryStringArena);
//DEF_STRING_ARENA_FUNCS(undoStringArena);

global Input input = {};
global ScreenBuffer screenBuffer;
global UserSettings userSettings;

#include "TextEditor_input.cpp"
#include "TextEditor.cpp"
#include "TextEditor_string.cpp"
#include "TextEditor_font.cpp"
#include "TextEditor_meta.cpp"
#include "TextEditor_config.cpp"
#include "TextEditor_tokeniser.cpp"


BITMAPINFO bitmapInfo;
bool running;

inline uint32 SafeTruncateSize32(uint64 val)
{
    //TODO: Defines for max values
    Assert(val <= 0xFFFFFFFF);
    uint32 result = (uint32)val;
    return result;
}

void Print(const char* message)
{
    wchar* msg = CStrToWStr(message);
    OutputDebugString(msg);
}

internal void win32_LogError()
{
    DWORD errCode = GetLastError();
    char errMsg[128];
    snprintf(errMsg, sizeof(errMsg), "Error Code: %8x\n", errCode);
    Print(errMsg);
}

void FreeWin32(void* mem)
{
    VirtualFree(mem, 0, MEM_RELEASE);
}

void* ReadEntireFile(string fileName, int* fileLen)
{
    void* result = nullptr;

    char* fileNameCStr = fileName.cstr();
    HANDLE fileHandle = CreateFileA(
        fileNameCStr, 
        GENERIC_READ, 
        FILE_SHARE_READ, 
        0, 
        OPEN_EXISTING, 
        0, 0
    );
    free(fileNameCStr);

    if (fileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER fileSize;
        if (GetFileSizeEx(fileHandle, &fileSize))
        {
            uint32 fileSize32 = SafeTruncateSize32(fileSize.QuadPart);
            result = VirtualAlloc(0, fileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            if (result)
            {
                DWORD bytesRead;
                if(ReadFile(fileHandle, result, fileSize32, &bytesRead, 0) && fileSize32 == bytesRead)
                {
                    //File read Successfully!
                    if (fileLen) *fileLen = fileSize32;
                }
                else
                {
                    VirtualFree(result, 0, MEM_RELEASE);
                    result = nullptr;
                }
            }
            else
            {
                //Log
            }
        }
        else
        {
            //Log
        }
        CloseHandle(fileHandle);
    }
    else
    {
        //Log
        win32_LogError();
    }

    return result;
}

string ReadEntireFileAsString(string fileName)
{
    string result;
    result.str = (char*)ReadEntireFile(fileName, &result.len);
    return result;
}

bool WriteToFile(string fileName, string text, bool overwrite, int32 writeStart)
{
    bool result = false;

    char* fileNameCStr = fileName.cstr();
    DWORD creationDisposition = (overwrite) ? OPEN_EXISTING : CREATE_ALWAYS;
    HANDLE fileHandle = CreateFileA(fileNameCStr, GENERIC_WRITE, 0, 0, creationDisposition, 0, 0);
    free(fileNameCStr);
    if (fileHandle != INVALID_HANDLE_VALUE)
    {
        Assert(writeStart >= 0);
        //TODO: make possible for large changes
        SetFilePointer(fileHandle, writeStart, NULL, FILE_BEGIN);
        DWORD bytesWritten;
        if(WriteFile(fileHandle, text.str, (DWORD)text.len, &bytesWritten, 0))
        {
            //File written Successfully!
            result = (bytesWritten == (DWORD)text.len);
            if(!SetEndOfFile(fileHandle))
            {
                //Log
            } 
        }
        else
        {
            //Log
        }
        CloseHandle(fileHandle);
    }
    else
    {
        //Log
    }

    return result;
}

void CopyToClipboard(string text)
{
    LPTSTR lptstrCopy; 
    HGLOBAL hglbCopy; 

    //Should this maybe return error??? When would not opening the clipboard happen???
    if (!OpenClipboard(NULL)) 
        return; 

    EmptyClipboard();

    hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (text.len + 1) * sizeof(wchar)); 
    if (hglbCopy == NULL) 
    { 
        //Should this maybe return error??? 
        CloseClipboard(); 
        return; 
    }

    lptstrCopy = (LPTSTR)GlobalLock(hglbCopy);
    memcpy(lptstrCopy, CStrToWStr(text.str, text.len), text.len * sizeof(wchar)); 
    lptstrCopy[text.len] = (wchar)0;
    GlobalUnlock(hglbCopy); 

    SetClipboardData(CF_UNICODETEXT, hglbCopy); 

    CloseClipboard(); 
}

//TODO: If we decide to fully support unicode, have this return wchar*
string GetClipboardText()
{
    HGLOBAL hglb; 
    LPTSTR lptstr; 

    if (!OpenClipboard(NULL)) 
        return {0};

    string result = {0};
    hglb = GetClipboardData(CF_UNICODETEXT);
    if (hglb != NULL) 
    {   
        lptstr = (wchar*)GlobalLock(hglb); 
        if (lptstr != NULL) 
        { 
            result.len = (int)wcslen(lptstr);
            result.str = (char*)Alloc_temporaryStringArena(result.len + 1);
            wcstombs_s(0, result.str, result.len + 1, lptstr, result.len);
            //result[len] = 0;
            GlobalUnlock(hglb); 
        }
    }

    CloseClipboard();
    return result;
}

//TODO: This thing doesn't always work, it's probably going to suck but you may have to do it the reccomended way
string ShowFileDialogAndGetFileName(bool save)
{
    string result = {0};

    OPENFILENAME ofn = {0};     // common dialog box structure
    wchar chosenFileName[256];  // buffer for file name //TODO: CHECK FOR BUFFER OVERFLOW
    chosenFileName[0] = 0;      // this ensures GetOpenFileName does not use the contents of szFile to initialize itself.

    // Initialize OPENFILENAME
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = chosenFileName;
    ofn.nMaxFile = sizeof(chosenFileName) / sizeof(wchar);
    ofn.lpstrFilter = L"All\0*.*\0Text\0*.TXT\0\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;

    // Display the Open dialog box. 
    BOOL succeeded = (save) ? GetSaveFileName(&ofn) : GetOpenFileName(&ofn);
    if (succeeded)
    {
        int len = (int)wcslen(chosenFileName);
        result.str = (char*)Alloc_temporaryStringArena(len + 1);
        wcstombs_s(0, result.str, len + 1, chosenFileName, len);
        result.len = len;
    } 

    //NOTE: This is to prevent input not updating properly. Maybe find better approach idk
    input = {0}; 
 
    return result;
}

inline void win32_HandleInputDown(byte* inputFlags)
{
    if (!InputHeld(*inputFlags)) 
    {
        *inputFlags |= INPUT_DOWN;
        *inputFlags |= INPUT_HELD;
    }
}

inline void win32_HandleInputUp(byte* inputFlags)
{
    *inputFlags &= ~INPUT_HELD;
    *inputFlags |= INPUT_UP;
}

internal void win32_ProcessInput()
{
	input.scrollWheelDelta = 0.0f;

    for (int i = 0; i < NUM_INPUTS; ++i)
    {
        if (InputDown(input.flags[i]))
        {
            input.flags[i] &= ~INPUT_DOWN;
        }
        else if (InputUp(input.flags[i]))
        {
            input.flags[i] &= ~INPUT_UP;
        }
    }
}

void win32_LogInput(InputCode code)
{
	if (InputDown(input.flags[code]))
	{
		Assert(true);
	}

    wchar inputLog[32];
    wchar* codeStr = CStrToWStr(InputCodeToStr(code));
    if (code >= MOUSE_START && code < ARROWS_START)
        swprintf_s(inputLog, L"%s BUTTON", codeStr);
    else if (code >= ARROWS_START && code < NON_SHIFT_START)
        swprintf_s(inputLog, L"%s ARROW KEY", codeStr);
    else
        swprintf_s(inputLog, L"%s KEY", codeStr);

    wchar log[64];
    if (InputDown(input.flags[code]))
        swprintf_s(log, L"%s DOWN\n", inputLog);
    else if (InputUp(input.flags[code]))
        swprintf_s(log, L"%s UP\n", inputLog);
    else if (InputHeld(input.flags[code]))
        swprintf_s(log, L"HOLDING %s\n", inputLog);

    if (input.flags[code])
        OutputDebugString(log);
}

//DIB: Device Independent Bitmap
internal void win32_ResizeDIB(int width, int height)
{
    if (screenBuffer.memory)
    {
        VirtualFree(screenBuffer.memory, 0, MEM_RELEASE);
    }

    screenBuffer.width = width;
    screenBuffer.height = height;

    bitmapInfo.bmiHeader.biSize        = sizeof(bitmapInfo.bmiHeader);
    bitmapInfo.bmiHeader.biWidth       = screenBuffer.width;
    bitmapInfo.bmiHeader.biHeight      = screenBuffer.height;
    bitmapInfo.bmiHeader.biPlanes      = 1;
    bitmapInfo.bmiHeader.biBitCount    = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    int bitmapMemorySize = PIXEL_IN_BYTES * screenBuffer.width * screenBuffer.height;
    screenBuffer.memory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);   

}

internal void win32_UpdateWindow(HDC deviceContext, int x, int y, int width, int height)
{
    StretchDIBits(
        deviceContext, 
        0, 0, screenBuffer.width, screenBuffer.height,
        x, y, width, height,
        screenBuffer.memory,
        &bitmapInfo,
        DIB_RGB_COLORS,
        SRCCOPY);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                    PWSTR pCmdLine, int nCmdShow)
{
    LARGE_INTEGER perfCountFreqResult;
    QueryPerformanceFrequency(&perfCountFreqResult);
    int64 perfCountFreq = perfCountFreqResult.QuadPart;


    // Register the window class.
    const wchar windowName[]  = L"TextEditorWindowClass";
    
    WNDCLASS wc = {};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = windowName;

    RegisterClass(&wc);

    // Create the window.

    HWND hwnd = CreateWindowEx(
        0,                              // Optional window styles.
        windowName,                     // Window class
        L"Text editor",    // Window text
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
        );

    if (hwnd == NULL)
    {
        return 0;
    }

    userSettings = LoadUserSettingsFromConfigFile();

    stbtt_fontinfo fontInfo;
    uchar* ttfFile = (uchar*)ReadEntireFile(userSettings.fontFile);
    stbtt_InitFont(&fontInfo, ttfFile, stbtt_GetFontOffsetForIndex(ttfFile, 0));

    int offsetAboveBaseline, offsetBelowBaseline, lineGap;
    stbtt_GetFontVMetrics(&fontInfo, &offsetAboveBaseline, &offsetBelowBaseline, &lineGap);
    float scale = stbtt_ScaleForPixelHeight(&fontInfo, 
                                            (float)PointsToPix(fontSizes[fontData.sizeIndex]));

    fontData.maxHeight = (uint32)RoundToInt((offsetAboveBaseline - offsetBelowBaseline) * scale);
    fontData.lineGap = (uint32)RoundToInt(lineGap * scale);
    fontData.offsetBelowBaseline = (uint32)RoundToInt(-offsetBelowBaseline * scale);

    for (uchar c = 0; c < 128; ++c)
    {
        FontChar fc;
        
        int width, height, xOffset, yOffset, advance, lsb;
        fc.pixels = stbtt_GetCodepointBitmap(&fontInfo, 0, scale, c, &width, &height, &xOffset, &yOffset);
        stbtt_GetCodepointHMetrics(&fontInfo, c, &advance, &lsb);

        fc.width = width;
        fc.height = height;
        fc.left = xOffset;
        fc.top = yOffset;
        fc.advance = (uint32)RoundToInt(advance * scale);
        fontData.chars[c] = fc;
    }    

    FreeWin32(ttfFile);

    ShowWindow(hwnd, nCmdShow);

    running = true;
    int64 prevCount = 0;


    Init();

    while (running)
    { 
        LARGE_INTEGER currentCountResult;
        QueryPerformanceCounter(&currentCountResult);
        int64 currentCount = currentCountResult.QuadPart;
        float deltaTime = (float)(currentCount - prevCount) / (float)perfCountFreq;
        //int64 fps = perfCountFreq / (currentCount - prevCount);
        prevCount = currentCount;

        //wchar log[100];
        //swprintf_s(log, L"dt: %fs, FPS: %lli\n", deltaTime, fps);
        //OutputDebugString(log);

        //for (int i = 0; i < NUM_INPUTS; ++i) win32_LogInput((InputCode)i);

        //wchar log[100];
        //swprintf_s(log, L"MousePos: {%i, %i}.\n", input.mousePixelPos.x, input.mousePixelPos.y);
        //OutputDebugString(log);
        win32_ProcessInput();

        //If the text editor is not the current app, reset the input
        HWND currentWindow = GetForegroundWindow();
        if (currentWindow != hwnd) input = {0};

        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                running = false;

            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        Draw(deltaTime);
        
        HDC hdc = GetDC(hwnd);
        RECT windowRect;
        GetClientRect(hwnd, &windowRect);
        int windowWidth = windowRect.right - windowRect.left;
        int windowHeight = windowRect.bottom - windowRect.top;
        win32_UpdateWindow(hdc, 0, 0, windowWidth, windowHeight);
        ReleaseDC(hwnd, hdc);

        FlushStringArena(&temporaryStringArena);
    }
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_SIZE:
        {
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            int width = clientRect.right - clientRect.left;
            int height = clientRect.bottom - clientRect.top;
            win32_ResizeDIB(width, height);
        } return 0;

        case WM_CLOSE:
        {
            running = false;
            DestroyWindow(hwnd);
        } return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC hdc = BeginPaint(hwnd, &paint);

            
            int x = paint.rcPaint.left;
            int y = paint.rcPaint.top;
            int width = paint.rcPaint.right - paint.rcPaint.left;
            int height = paint.rcPaint.bottom - paint.rcPaint.top;

            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            win32_UpdateWindow(hdc, x, y, width, height);


            EndPaint(hwnd, &paint);
        } return 0;

        case WM_LBUTTONDOWN:
            win32_HandleInputDown(&input.leftMouse);
            return 0;

        case WM_LBUTTONUP:
            win32_HandleInputUp(&input.leftMouse);
            return 0;

        case WM_RBUTTONDOWN:
            win32_HandleInputDown(&input.rightMouse);
            return 0;

        case WM_RBUTTONUP:
            win32_HandleInputUp(&input.rightMouse);
            return 0;

        case WM_MBUTTONDOWN:
            win32_HandleInputDown(&input.middleMouse);
            Print("test\n");
            return 0;

        case WM_MBUTTONUP:
            win32_HandleInputUp(&input.middleMouse);
            return 0;

        case WM_MOUSEWHEEL:
			input.scrollWheelDelta = (float)GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
            return 0;

        case WM_MOUSEMOVE:
            input.mousePixelPos.x = GET_X_LPARAM(lParam); 
            input.mousePixelPos.y = screenBuffer.height - GET_Y_LPARAM(lParam);
            return 0;

        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        {
            uint32 vkCode = (uint32)wParam;
            if (vkCode >= (uint32)'A' && vkCode <= (uint32)'Z')
                win32_HandleInputDown(&input.letterKeys[vkCode - 'A']);
            else if (vkCode >= (uint32)'0' && vkCode <= (uint32)'9')
                win32_HandleInputDown(&input.numberKeys[vkCode - '0']);
            else if (vkCode >= VK_LEFT && vkCode <= VK_DOWN)
                win32_HandleInputDown(&input.arrowKeys[vkCode - VK_LEFT]);
            //else if (vkCode >= VK_F1 && <= VK_F12)
            //    win32_HandleInputDown(&input.fKeys[vkCode - VK_F1]);
            else if (vkCode == VK_F5)
                win32_HandleInputDown(&input.f5);
            else if (vkCode == VK_BACK)
                win32_HandleInputDown(&input.backspace);
            else if (vkCode == VK_SHIFT || vkCode == VK_LSHIFT)
                win32_HandleInputDown(&input.leftShift);
            else if (vkCode == VK_MENU || vkCode == VK_LMENU)
                win32_HandleInputDown(&input.leftAlt);
            else if (vkCode == VK_CAPITAL)
                win32_HandleInputDown(&input.capsLock);
            else if (vkCode == VK_TAB)
                win32_HandleInputDown(&input.tab);
            else if (vkCode == VK_RETURN)
                win32_HandleInputDown(&input.enter);
            else if (vkCode == VK_CONTROL || vkCode == VK_LCONTROL)
                win32_HandleInputDown(&input.leftCtrl);
            else if (vkCode == VK_SPACE)
                win32_HandleInputDown(&input.space);
            else if (vkCode == VK_OEM_4)
                win32_HandleInputDown(&input.flags[INPUTCODE_OPEN_SQ_BRACKET]);
            else if (vkCode == VK_OEM_6)
                win32_HandleInputDown(&input.flags[INPUTCODE_CLOSED_SQ_BRACKET]);
            else if (vkCode == VK_OEM_1)
                win32_HandleInputDown(&input.flags[INPUTCODE_SEMICOLON]);
            else if (vkCode == VK_OEM_COMMA) 
                win32_HandleInputDown(&input.flags[INPUTCODE_COMMA]);
            else if (vkCode == VK_OEM_PERIOD) 
                win32_HandleInputDown(&input.flags[INPUTCODE_PERIOD]);
            else if (vkCode == VK_OEM_7) 
                win32_HandleInputDown(&input.flags[INPUTCODE_QUOTE]);
            else if (vkCode == VK_OEM_2) 
                win32_HandleInputDown(&input.flags[INPUTCODE_FORWARD_SLASH]);
            else if (vkCode == VK_OEM_3) 
                win32_HandleInputDown(&input.grave);
            else if (vkCode == VK_OEM_PLUS) 
                win32_HandleInputDown(&input.equals);
            else if (vkCode == VK_OEM_MINUS) 
                win32_HandleInputDown(&input.minus);
            else if (vkCode == VK_OEM_5) 
                win32_HandleInputDown(&input.backslash);
        } return 0;

        case WM_SYSKEYUP:
        case WM_KEYUP:
        {
            uint32 vkCode = (uint32)wParam;
            if (vkCode >= (uint32)'A' && vkCode <= (uint32)'Z')
                win32_HandleInputUp(&input.letterKeys[vkCode - 'A']);
            else if (vkCode >= (uint32)'0' && vkCode <= (uint32)'9')
                win32_HandleInputUp(&input.numberKeys[vkCode - '0']);
            else if (vkCode >= VK_LEFT && vkCode <= VK_DOWN)
                win32_HandleInputUp(&input.arrowKeys[vkCode - VK_LEFT]);
            //else if (vkCode >= VK_F1 && <= VK_F12)
            //    win32_HandleInputUp(&input.fKeys[vkCode - VK_F1]);
            else if (vkCode == VK_F5)
                win32_HandleInputUp(&input.f5);
            else if (vkCode == VK_BACK)
                win32_HandleInputUp(&input.backspace);
            else if (vkCode == VK_SHIFT || vkCode == VK_LSHIFT)
                win32_HandleInputUp(&input.leftShift);
            else if (vkCode == VK_MENU || vkCode == VK_LMENU)
                win32_HandleInputUp(&input.leftAlt);
            else if (vkCode == VK_CAPITAL)
                win32_HandleInputUp(&input.capsLock);
            else if (vkCode == VK_TAB)
                win32_HandleInputUp(&input.tab);
            else if (vkCode == VK_RETURN)
                win32_HandleInputUp(&input.enter);
            else if (vkCode == VK_CONTROL || vkCode == VK_LCONTROL)
                win32_HandleInputUp(&input.leftCtrl);
            else if (vkCode == VK_SPACE)
                win32_HandleInputUp(&input.space);
            else if (vkCode == VK_OEM_4)
                win32_HandleInputUp(&input.flags[INPUTCODE_OPEN_SQ_BRACKET]);
            else if (vkCode == VK_OEM_6)
                win32_HandleInputUp(&input.flags[INPUTCODE_CLOSED_SQ_BRACKET]);
            else if (vkCode == VK_OEM_1)
                win32_HandleInputUp(&input.flags[INPUTCODE_SEMICOLON]);
            else if (vkCode == VK_OEM_COMMA) 
                win32_HandleInputUp(&input.flags[INPUTCODE_COMMA]);
            else if (vkCode == VK_OEM_PERIOD) 
                win32_HandleInputUp(&input.flags[INPUTCODE_PERIOD]);
            else if (vkCode == VK_OEM_7) 
                win32_HandleInputUp(&input.flags[INPUTCODE_QUOTE]);
            else if (vkCode == VK_OEM_2) 
                win32_HandleInputUp(&input.flags[INPUTCODE_FORWARD_SLASH]);
            else if (vkCode == VK_OEM_3) 
                win32_HandleInputUp(&input.grave);
            else if (vkCode == VK_OEM_PLUS) 
                win32_HandleInputUp(&input.equals);
            else if (vkCode == VK_OEM_MINUS) 
                win32_HandleInputUp(&input.minus);
            else if (vkCode == VK_OEM_5) 
                win32_HandleInputUp(&input.backslash);
        } return 0;
        
        

    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}