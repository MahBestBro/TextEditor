#include "TextEditor_defs.h"
#include "TextEditor.h"

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
    TOKEN_FUNCTION,
    TOKEN_CUSTOM_TYPE,
    TOKEN_INBUILT_TYPE,
    TOKEN_KEYWORD,
    TOKEN_PREPROCESSOR_TAG,
    TOKEN_DEFINE,

    TOKEN_COMMENT,

    TOKEN_EOS,
    TOKEN_UNKNOWN,

    NUM_TOKENS
};

enum MultilineState
{
    MS_NON_MULTILINE,
    MS_STRING,
    MS_COMMENT
};

struct Token
{
    EditorPos at;
    int textLength;
    TypeOfToken type;
};

struct TokenInfo
{
    Token tokens[MAX_LINES];
    int numTokens;
};

void Tokenise(TokenInfo* dest);
TokenInfo TokeniseLine(Line code, int lineIndex, MultilineState* multilineState);

#endif