#include "TextEditor_defs.h"
#include "TextEditor.h"
#include "TextEditor_string.h"
#include "TextEditor_tokeniser.h"

//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>

StringContainer types = {};
StringContainer defines = {};

bool IsNumber(char* str, int len)
{
    for (int i = 0; i < len; ++i)
    {
        if (!IsNumeric(str[i]))
            return false;
    }
    return true;
}

bool IsBool(char* str, int len)
{
    return CompareStrings(str, len, "true", 4) || CompareStrings(str, len, "false", 5);
}

bool IsInStringArray(char* str, int len, char** stringArr, int arrLen)
{
    for (int i = 0; i < arrLen; ++i)
    {
        if (CompareStrings(str, len, stringArr[i], StringLen(stringArr[i])))
            return true;
    }
    return false;
}

/*internal void EatWhitespaceAndComments(Line code, int* at)
{
    while(*at < code.len)
    {
        if (IsWhiteSpace(code.text[*at]))
        {
            (*at)++;
        }
        else if (code.text[*at] == '/' && code.text[*at + 1] == '/')
        {
            //Parse C++ '//' Comment
            *at += 2;
            while (*at < code.len) (*at)++;
        }
        else if (code.text[*at] == '/' && code.text[*at + 1] == '*')
        {
            //Parse C style '/*' Comment
            *at += 2;
            while (*at < code.len && code.text[*at] != '*' && code.text[*at + 1] != '/')
                (*at)++;

            *at += 2 * (code.text[*at] != '*');
        }
        else break;
    }   
    
}*/

enum MultilineState
{
    MS_NON_MULTILINE,
    MS_STRING,
    MS_COMMENT
};

Token GetTokenFromLine(Line code, int* at, MultilineState ms)
{
    //EatWhitespaceAndComments(tokeniser);

    char* inbuiltTypes[] = 
    {
        "int", 
        "float", 
        "char", 
        "void", 
        "bool", 
        "struct", 
        "enum", 
        "unsigned"
    };

    char* keywords[] = 
    {
        "return", 
        "static", 
        "if", 
        "else", 
        "for", 
        "while", 
        "typedef"
    };

    char* preprocessorTags[] = 
    {
        "#include", 
        "#define", 
        "#if", 
        "#elif", 
        "#else", 
        "#ifdef", 
        "#ifndef"
    };

    int _at = *at;
    Token token = {};
    token.textAt = _at;

    switch(ms)
    {
        case MS_COMMENT:
        {
            Assert(_at == 0);

            token.type = TOKEN_COMMENT;
            _at++;
            while(_at < code.len)
            {
                if (code.text[_at] == '/' && code.text[_at - 1] == '*')
                    break;

                _at++;
            } 
            token.textLength = _at + (_at < code.len);
            *at = _at + (_at < code.len);
        } return token;

        case MS_STRING: 
        {
            Assert(_at == 0);

            token.type = TOKEN_STRING;
            while(_at < code.len)
            {
                if (code.text[_at] == '"')
                    break;
                _at++;
            } 

            token.textLength = _at + (_at < code.len);
            *at = _at + (_at < code.len);
        } return token;

        case MS_NON_MULTILINE: break;
    }

    token.textLength = 1;
    char c = code.text[_at];
    ++_at;

    switch(c)
    {
        case '\0': 
            token.type = TOKEN_EOS; 
            break;

        case '(': 
        case ')':     
        case ';': 
        case '[':     
        case ']':      
        case '{':    
        case '}':  
        case ',':   
        case '.':   
            token.type = TOKEN_PUNCTUATION; 
            break;
 
        case '+': 
        case '*': 
        case '/': 
        case '%': 
        case '=':  
        case '&': 
        case '|': 
        case '^': 
        case '!': 
        case '?': 
        case ':': 
            token.type = TOKEN_OPERATOR;      
            break;

        case '-': 
        {
            if (code.text[_at] == '>')
            {
                token.type = TOKEN_PUNCTUATION;
                token.textLength = 2;
                _at++;
            } 
            else
            {
                token.type = TOKEN_OPERATOR;
            }
        } break;

        case '>': 
        {
            //NOTE: Don't need to check for arrow operator because the above skips over the > if it is an arrow operator
            token.type = TOKEN_OPERATOR;
        } break;

        case '<':
        {
            //TODO: Handle this weird angle bracket file name case in the include part
            if (!IsAlphabetical(code.text[_at]))
            {
                token.type = TOKEN_OPERATOR;
                break;
            }

            int start = _at;
            while((IsAlphabetical(code.text[_at]) || code.text[_at] == '.') && 
                  _at < code.len)
            {
                _at++;
            }
            token.textLength = _at - start;

            token.type = (code.text[_at] == '>') ? TOKEN_STRING : TOKEN_UNKNOWN;
            _at++;
        } break;

        case '"':
        {
            token.type = TOKEN_STRING;
            while(code.text[_at] != '"' && _at < code.len)
            {
                _at++;
            }

            //starting quotation
            int start = _at;
            while(_at < code.len && code.text[_at] != '"') ++_at;
            token.textLength = _at - start; //end - start + quotation
            
            if (code.text[_at] == '"')
            {
                ++token.textLength;
                ++_at;    
            }
        } break;

        case '#':
        {
            int start = _at;
            while (IsAlphabetical(code.text[_at]) && _at < code.len)
            {
                ++_at;
            }
            token.textLength = _at - start;

            if (IsInStringArray(code.text + token.textAt, token.textLength, 
                preprocessorTags, StackArrayLen(preprocessorTags)))
            {
                token.type = TOKEN_PREPROCESSOR_TAG;
                
                //char tokenText[token.textLength + 1];
                //snprintf(tokenText, sizeof(tokenText), "%.*s", token.textLength, token.text);
                
                if (CompareStrings(code.text + token.textAt, token.textLength, "#define", 7))
                {
                    //Get what's actually defined
                    int defStart = _at;
                    while (IsWhiteSpace(code.text[defStart])) ++defStart;

                    int defEnd = defStart;
                    while (!IsWhiteSpace(code.text[defEnd])) ++defEnd;
                    int defLen = defEnd - defStart;

                    //char buffer[defineLen + 1];
                    //snprintf(buffer, sizeof(buffer), "%.*s", defineLen, at);
                    //buffer[defineLen] = 0;
                    //For some reason here I have to use strcpy, I don't know why
                    defines.strings[defines.count] = HeapAlloc(char, defLen + 1);
                    memcpy(defines.strings[defines.count], code.text + defStart, defLen + 1);
                    defines.strings[defines.count][defLen] = 0;
                    defines.count++;
                }
            }
            else
            {
                token.type = TOKEN_UNKNOWN;
            }    
        } break;

        default:
        {
            token.type = TOKEN_IDENTIFIER;
            if (IsAlphabetical(c))
            {
                int start = _at;
                while (IsAlphaNumeric(code.text[_at]) || code.text[_at] == '_')
                {
                    ++_at;
                }
                token.textLength = _at - start + 1;
                
                char* tokenStr = code.text + token.textAt;
                if (IsInStringArray(tokenStr, token.textLength, keywords, StackArrayLen(keywords)))
                {
                    token.type = TOKEN_KEYWORD;

                    //If typedef, add type
                    //char tokenText[token.textLength + 1];
                    //snprintf(tokenText, sizeof(tokenText), "%.*s", token.textLength, token.text);
                    /*if (CompareStrings(token.text, token.textLength, "typedef", 7))
                    {
                        //Search for semicolon
                        char* at = _at;
                        while (at[0] != ';')
                        {
                            if (at[0] == '{')
                            {
                                while (at[0] != '}')
                                {
                                    ++at;
                                }
                            }
                            else ++at;
                            
                        }
                        char* end = at;

                        //Get type length
                        while (!IsWhiteSpace(at[0]))
                        {
                            --at;
                        }
                        ++at;
                        int typeLen = (int)(end - at);

                        //Add type to types
                        types.strings[types.count] = HeapAlloc(char, typeLen + 1);
                        memcpy(types.strings[types.count], at, typeLen + 1);
                        types.strings[types.count][typeLen] = 0;
                        types.count++;
                    }*/
                }
                else if (IsInStringArray(tokenStr, token.textLength, inbuiltTypes, 
                         StackArrayLen(inbuiltTypes))) 
                {
                    token.type = TOKEN_INBUILT_TYPE;
                    
                    //If a struct or enum, add the type
                    if (CompareStrings(tokenStr, token.textLength, "struct", 6) ||
                        CompareStrings(tokenStr, token.textLength, "enum", 4))
                    {
                        int typeStart = _at;
                        while (IsWhiteSpace(code.text[typeStart])) ++typeStart;

                        int typeEnd = start;
                        while (!IsWhiteSpace(code.text[typeEnd])) ++typeEnd;
                        int typeLen = typeEnd - typeStart;

                        //Add type to types
                        types.strings[types.count] = HeapAlloc(char, typeLen + 1);
                        memcpy(types.strings[types.count], code.text + typeStart, typeLen + 1);
                        types.strings[types.count][typeLen] = 0;
                        types.count++;
                    }
                }
                else if (IsInStringArray(tokenStr, token.textLength, defines.strings, defines.count))
                {
                    token.type = TOKEN_DEFINE;
                }
                else if (IsInStringArray(tokenStr, token.textLength, types.strings, types.count))
                {
                    token.type = TOKEN_CUSTOM_TYPE;
                }
                else if (IsBool(tokenStr, token.textLength))
                {
                    token.type = TOKEN_BOOL;
                }
                else
                {
                    token.type = TOKEN_IDENTIFIER;
                }
            }
            else if (IsNumeric(c))
            {
                int start = _at;
                while (IsNumeric(code.text[_at])) ++_at;  
                token.textLength = _at - start + 1;
                char* tokenStr = code.text + token.textAt;
                
                token.type = (IsNumber(tokenStr, token.textLength)) ? TOKEN_NUMBER : TOKEN_UNKNOWN;
            }
            else
            {
                token.type = TOKEN_UNKNOWN;
            }
        } break;
        
    }

    *at = _at + (_at < code.len); 
    return token;
}

/*static Token GetToken(Tokeniser* tokeniser)
{
    EatWhitespaceAndComments(tokeniser);

    char* inbuiltTypes[] = 
    {
        "int", 
        "float", 
        "char", 
        "void", 
        "bool", 
        "struct", 
        "enum", 
        "unsigned"
    };

    char* keywords[] = 
    {
        "return", 
        "static", 
        "if", 
        "else", 
        "for", 
        "while", 
        "typedef"
    };

    char* preprocessorTags[] = 
    {
        "#include", 
        "#define", 
        "#if", 
        "#elif", 
        "#else", 
        "#ifdef", 
        "#ifndef"
    }; 

    Token token = {};
    token.textLength = 1;
    token.text = tokeniser->code + tokeniser->at;
    char c = tokeniser->code[tokeniser->at];
    ++tokeniser->at;
    switch(c)
    {
        case '\0': 
            token.type = TOKEN_EOS; 
            break;

        case '(': 
        case ')':     
        case ';': 
        case '[':     
        case ']':      
        case '{':    
        case '}':  
        case ',':   
        case '.':   
            token.type = TOKEN_PUNCTUATION; 
            break;
 
        case '+': 
        case '*': 
        case '/': 
        case '%': 
        case '=':  
        case '&': 
        case '|': 
        case '^': 
        case '!': 
        case '?': 
        case ':': 
            token.type = TOKEN_OPERATOR;      
            break;

        case '-': 
        {
            if (tokeniser->code[tokeniser->at] == '>')
            {
                token.type = TOKEN_PUNCTUATION;
                token.textLength = 2;
                tokeniser->at++;
            } 
            else
            {
                token.type = TOKEN_OPERATOR;
            }
        } break;

        case '>': 
        {
            //NOTE: Don't need to check for arrow operator because the above skips over the > if it is an arrow operator
            token.type = TOKEN_OPERATOR;
        } break;

        case '<':
        {
            //TODO: Handle this weird angle bracket file name case in the include part
            if (!IsAlphabetical(tokeniser->code[tokeniser->at]))
            {
                token.type = TOKEN_OPERATOR;
                break;
            }

            int start = tokeniser->at;

            while(IsAlphabetical(tokeniser->code[tokeniser->at]) || 
                  tokeniser->code[tokeniser->at] == '.' && 
                  tokeniser->at < tokeniser->codeLen)
            {
                tokeniser->at++;
            }
            token.textLength = tokeniser->at - start;

            token.type = (tokeniser->code[tokeniser->at] == '>') ? TOKEN_STRING : TOKEN_UNKNOWN;
            tokeniser->at++;
        } break;

        case '"':
        {
            token.type = TOKEN_STRING;
            token.text = tokeniser->code + tokeniser->at - 1; //starting quotation
            while(tokeniser->code[tokeniser->at] && tokeniser->code[tokeniser->at] != '"')
            {
                if (tokeniser->code[tokeniser->at] == '\\' && tokeniser->at[1])
                {
                    ++tokeniser->at;
                }
                
                ++tokeniser->at;
            }
            token.textLength = (int)(tokeniser->at - token.text + 1); //end - start + quotation
            if (tokeniser->at[0] == '"')
            {
                ++tokeniser->at;    
            }
        } break;

        case '#':
        {
            while (IsAlphabetical(tokeniser->at[0]) || IsNumeric(tokeniser->at[0]) || 
                    tokeniser->at[0] == '_')
            {
                ++tokeniser->at;
            }
            token.textLength = (int)(tokeniser->at - token.text);

            if (IsInStringArray(token, preprocessorTags, StackArrayLen(preprocessorTags)))
            {
                token.type = TOKEN_PREPROCESSOR_TAG;
                
                //char tokenText[token.textLength + 1];
                //snprintf(tokenText, sizeof(tokenText), "%.*s", token.textLength, token.text);
                
                if (CompareStrings(token.text, token.textLength, "#define", 7))
                {
                    //Get what's actually defined
                    char* at = tokeniser->at + 1;
                    char* end = at;
                    while (!IsWhiteSpace(end[0]))
                    {
                        ++end;
                    }
                    int defineLen = (int)(end - at);

                    //char buffer[defineLen + 1];
                    //snprintf(buffer, sizeof(buffer), "%.*s", defineLen, at);
                    //buffer[defineLen] = 0;
                    //For some reason here I have to use strcpy, I don't know why
                    defines.strings[defines.count] = HeapAlloc(char, defineLen + 1);
                    memcpy(defines.strings[defines.count], at, defineLen + 1);
                    defines.strings[defines.count][defineLen] = 0;
                    defines.count++;
                }
            }
            else
            {
                token.type = TOKEN_UNKNOWN;
            }    
        } break;

        default:
        {
            token.type = TOKEN_IDENTIFIER;
            if (IsAlphabetical(c))
            {
                while (IsAlphaNumeric(tokeniser->at[0]) || tokeniser->at[0] == '_')
                {
                    ++tokeniser->at;
                }
                token.textLength = (int)(tokeniser->at - token.text);
                
                if (IsInStringArray(token, keywords, StackArrayLen(keywords)))
                {
                    token.type = TOKEN_KEYWORD;

                    //If typedef, add type
                    //char tokenText[token.textLength + 1];
                    //snprintf(tokenText, sizeof(tokenText), "%.*s", token.textLength, token.text);
                    if (CompareStrings(token.text, token.textLength, "typedef", 7))
                    {
                        //Search for semicolon
                        char* at = tokeniser->at;
                        while (at[0] != ';')
                        {
                            if (at[0] == '{')
                            {
                                while (at[0] != '}')
                                {
                                    ++at;
                                }
                            }
                            else ++at;
                            
                        }
                        char* end = at;

                        //Get type length
                        while (!IsWhiteSpace(at[0]))
                        {
                            --at;
                        }
                        ++at;
                        int typeLen = (int)(end - at);

                        //Add type to types
                        types.strings[types.count] = HeapAlloc(char, typeLen + 1);
                        memcpy(types.strings[types.count], at, typeLen + 1);
                        types.strings[types.count][typeLen] = 0;
                        types.count++;
                    }
                }
                else if (IsInStringArray(token, inbuiltTypes, StackArrayLen(inbuiltTypes))) 
                {
                    token.type = TOKEN_INBUILT_TYPE;
                }
                else if (IsInStringArray(token, defines.strings, defines.count))
                {
                    token.type = TOKEN_DEFINE;
                }
                else if (IsInStringArray(token, types.strings, types.count))
                {
                    token.type = TOKEN_CUSTOM_TYPE;
                }
                else if (IsBool(token))
                {
                    token.type = TOKEN_BOOL;
                }
                else
                {
                    token.type = TOKEN_IDENTIFIER;
                }
            }
            else if (IsNumeric(c))
            {
                while (IsNumeric(tokeniser->at[0]))
                {
                    ++tokeniser->at;  
                }
                token.textLength = (int)(tokeniser->at - token.text);
                token.type = (IsNumber(token)) ? TOKEN_NUMBER : TOKEN_UNKNOWN;
            }
            else
            {
                token.type = TOKEN_UNKNOWN;
            }
        } break;
        
    }

    return token;
}*/

TokenInfo TokeniseLine(Line code)
{
    //The number of tokens in a line should not exceed that line's length 
    TokenInfo result = {};
    if (code.len == 0) return result;
    result.tokens = HeapAlloc(Token, code.len); 

    //Tokeniser tokeniser = {};
    //tokeniser.at = code.text;
    //tokeniser.codeLen = code.len;

    int at = 0;
    MultilineState multilineState = MS_NON_MULTILINE;
    bool parsing = true;
    while (parsing)
    {
        Token token = GetTokenFromLine(code, &at, multilineState);
        if (at == code.len)
        {
            switch (token.type)
            {
                case TOKEN_COMMENT: multilineState = MS_COMMENT; break;
                case TOKEN_STRING: multilineState = MS_STRING; break;
                default: break; 
            }
            parsing = false;
        }
        else
        {
            result.tokens[result.numTokens++] = token;
        }
            
    }

    return result;
}