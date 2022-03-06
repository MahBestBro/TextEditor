#include <windows.h>
#include "TextEditor_defs.h"
#include "TextEditor.h"

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

//TODO: Support multiple bases?
int StringToInt(char* str, int len, bool* success)
{
    if (len == 1 && str[0] == '-')
    {
        if (success) *success = false;
        return -1;
    }

    int result = 0;
    int powOf10 = 1;
    int sign = (str[0] == '-') ? 1 : -1;
    for (int i = len - 1; i >= (int)(str[0] == '-'); --i)
    {
        if (i == 0 && str[i] == '-') continue;

        int digit = str[i] - '0';
        if (digit < 0 || digit > 9)
        {
            if (success) *success = false;
            return -1;
        }
            
        result += digit * powOf10;
        powOf10 *= 10;
    }

    return result * sign;
}

//TODO: Overflow check
byte StringToByte(char* str, int len, bool* success)
{
    byte result = 0;
    byte powOf10 = 1;
    for (int i = len - 1; i >= 0; --i)
    {
        byte digit = str[i] - '0';
        if (digit < 0 || digit > 9)
        {
            if (success) *success = false;
            return 0;
        }
            
        result += digit * powOf10;
        powOf10 *= 10;
    }

    return result;
}

char** SplitStringByLines(char* str, int* len)
{
    int numStrings = 1;
    for (char* p = str; p[1]; p++)
    {
        if (p[0] == '\r')
        {
            Assert(p[1] == '\n');
            numStrings++;
            p++;
        }
    }

    char** result = (char**)malloc(numStrings * sizeof(char*));
    char* at = str;
    for (int i = 0; i < numStrings; ++i)
    {
        int lineLen = 0;
        char* start = at;
        for (; at[0] && at[0] != '\r' && at[0] != '\n'; at++)
            lineLen++;

        result[i] = (char*)malloc(lineLen + 1);
        memcpy(result[i], start, lineLen);
        result[i][lineLen] = 0;

        at += (at[0] == '\r') ? 2 : 1;
    }
        
    if (len) *len = numStrings;
    return result;
}

char* AdvanceToNextLine(char* file)
{
    while (file[0] != '\n') 
    {
        if (file[0] == 0)
            return nullptr;
        file++;
    }
    file++;
    return file;
}

char* ReverseString(char* str, int len)
{
    char* result = HeapAlloc(char, len);
    for (int i = 0; i < len; ++i)
        result[len - i - 1] = str[i];
    result[len] = 0;
    return result;
}

bool CompareStrings(char* a, int aLen, char* b, int bLen)
{
    if (aLen != bLen) return false;

    for (int i = 0; i < aLen; ++i)
    {
        if (a[i] != b[i]) return false;
    }
    return true;
}