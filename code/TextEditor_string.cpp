#include <windows.h>
#include "TextEditor_defs.h"

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

//TODO: Make compatible with UNIX style line splitting
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
        for (; at[0] && at[0] != '\r'; at++)
            lineLen++;

        result[i] = (char*)malloc(lineLen + 1);
        memcpy(result[i], start, lineLen);
        result[i][lineLen] = 0;

        at += 2;
    }
        
    *len = numStrings;
    return result;
}

