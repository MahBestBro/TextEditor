#ifndef TEXT_EDITOR_STRING_H
#define TEXT_EDITOR_STRING_H

inline int StringLen(const char* string)
{
    int result = 0;
    for (; string[0]; string++)
        result++;
    return result;
}

inline bool IsAlphabetical(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

inline bool IsNumeric(char c)
{
    return c >= '0' && c <= '9';
}

inline bool IsAlphaNumeric(char c)
{
    return IsAlphabetical(c) || IsNumeric(c);
}

void IntToString(int val, char* buffer);
char** SplitStringByLines(char* str, int *len);

#endif