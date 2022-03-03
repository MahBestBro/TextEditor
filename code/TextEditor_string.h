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

inline bool IsInvisChar(char c)
{
    return c <= ' ';
}

//Double check this is correct?
inline bool IsPunctuation(char c)
{
    return !IsAlphaNumeric(c) && !IsInvisChar(c);
}

inline bool IsEmptyString(char* str)
{
    return str[0] == 0;
}

void IntToString(int val, char* buffer);
int StringToInt(char* str, int len, bool* success = nullptr);
byte StringToByte(char* str, int len, bool* success = nullptr);
char** SplitStringByLines(char* str, int *len = nullptr);
char* AdvanceToNextLine(char* file);
char* ReverseString(char* str, int len);

#endif