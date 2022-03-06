#ifndef TEXT_EDITOR_TOKENISER_H
#define TEXT_EDITOR_TOKENISER_H

//Thanks winapi for not allowing me to name this TokenType
enum TypeOfToken
{    
    TOKEN_PUNCTUATION,
    TOKEN_OPERATOR,
    
    TOKEN_STRING,
    TOKEN_NUMBER,
    TOKEN_BOOL,
    TOKEN_IDENTIFIER,
    TOKEN_CUSTOM_TYPE,
    TOKEN_INBUILT_TYPE,
    TOKEN_KEYWORD,
    TOKEN_PREPROCESSOR_TAG,
    TOKEN_DEFINE,

    TOKEN_COMMENT,

    TOKEN_EOS,
    TOKEN_UNKNOWN
};

struct Token
{
    int textAt;
    int textLength;
    TypeOfToken type;
};

struct Tokeniser
{
    char* code;
    int at;
    int codeLen;
};

struct TokenInfo
{
    Token* tokens;
    int numTokens;
};

struct StringContainer
{
    char* strings[32];
    int count;
};

TokenInfo TokeniseLine(Line code);

#endif