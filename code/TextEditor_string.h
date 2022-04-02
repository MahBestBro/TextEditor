#ifndef TEXT_EDITOR_STRING_H
#define TEXT_EDITOR_STRING_H

struct string
{
    char* str;
    int len;

    char operator[](int index) { return str[index]; }
};

string cstring(char* cstr);

bool operator==(string lhs, string rhs);
inline bool operator==(char* lhs, string rhs) { return cstring(lhs) == rhs; }
inline bool operator==(string lhs, char* rhs) { return lhs == cstring(rhs); }

inline bool operator!=(string lhs, string rhs) { return !(lhs == rhs); }
inline bool operator!=(char* lhs, string rhs) { return !(lhs == rhs); }
inline bool operator!=(string lhs, char* rhs) { return !(lhs == rhs); }

inline bool cstrEquals(char* lhs, char* rhs) { return cstring(lhs) == cstring(rhs); }

string SubString(string s, int start, int end = -1);
inline string SubString(char* cstr, int start, int end = -1)
{
    return SubString(cstring(cstr), start, end);
}

bool StringContains(string s, string target);
inline bool StringContains(char* cstr, string target) 
{ 
    return StringContains(cstring(cstr), target); 
}
inline bool StringContains(string s, char* target_cstr) 
{ 
    return StringContains(s, cstring(target_cstr)); 
}
inline bool StringContains(char* cstr, char* target_cstr) 
{ 
    return StringContains(cstring(cstr), cstring(target_cstr)); 
}

int IndexOfCharInString(string s, char target);
inline int IndexOfCharInString(char* s, char target) 
{ 
    return IndexOfCharInString(cstring(s), target);
}
int IndexOfLastCharInString(string s, char target);
inline int IndexOfLastCharInString(char* s, char target) 
{ 
    return IndexOfLastCharInString(cstring(s), target);
}

struct string_buf
{
    char* str;
    size_t len;
    size_t cap;

    char operator[](int index) { return str[index]; }

    void operator+=(string s)
    {
        len += s.len;
        ResizeDynamicArray(&str, len, 1, &cap);
        memcpy(str + len - s.len, s.str, s.len);
    }
    void operator+=(char* s) { *this += cstring(s); }

    void operator=(string s)
    {
        len = s.len;
        cap = max(cap, s.len);
        memcpy(str, s.str, s.len);
    }
    void operator=(char* s) { *this = cstring(s); }

    void dealloc();

    inline string toStr()
    {
        return string{str, (int)len};
    }

    char* cstr();
};

string_buf init_string_buf(string s, int capacity);
inline string_buf init_string_buf(char* cstr, int capacity = -1)
{
    return init_string_buf(cstring(cstr), capacity);
}


inline int StringLen(const char* string)
{
    int result = 0;
    for (; string[0]; string++)
        result++;
    return result;
}

inline bool IsForwardsBracket(char c)
{
    return c == '(' || c == '[' || c == '{';
}

inline bool IsBackwardsBracket(char c)
{
    return c == ')' || c == ']' || c == '}';
}

inline bool IsBracket(char c)
{
    return IsForwardsBracket(c) ||IsBackwardsBracket(c);
}

inline char GetOtherBracket(char bracket)
{
    Assert(IsBracket(bracket));

    //idk if there's a clever way to do this but I can't figure it out
    switch(bracket) 
    {
        case '(': return ')';
        case ')': return '(';
        case '[': return ']';
        case ']': return '[';
        case '{': return '}';
        case '}': return '{';
        default: 
        {
            //NOTE: This is unreachable
            Assert(false);
            return 0;
            break;
        }
    }
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

inline bool IsWhiteSpace(char c)
{
    return c <= ' ';
}

//Double check this is correct?
inline bool IsPunctuation(char c)
{
    return !IsAlphaNumeric(c) && !IsWhiteSpace(c);
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