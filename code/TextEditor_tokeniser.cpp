#include "TextEditor_defs.h"
#include "TextEditor.h"
#include "TextEditor_string.h"
#include "TextEditor_tokeniser.h"

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

Token GetTokenFromLine(Line code, int* at, MultilineState* ms)
{
    //EatWhitespaceAndComments(tokeniser);
	int _at = *at;
	while (IsWhiteSpace(code.text[_at]) && _at < code.len) ++_at;

    //TODO: Move these out of function
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
    token.textAt = _at;
	if (_at == code.len)
	{
		token.textLength = 0;
		token.type = TOKEN_EOS;
        *at = code.len;
        return token;
	}

    switch(*ms)
    {
        case MS_COMMENT:
        {
            token.type = TOKEN_COMMENT;
            _at++;
            while(_at < code.len)
            {
                if (code.text[_at] == '/' && code.text[_at - 1] == '*')
                {
                    *ms = MS_NON_MULTILINE;
                    break;
                }

                _at++;
            } 
            token.textLength = _at + (_at < code.len);
            *at = _at + (_at < code.len);
        } return token;

        case MS_STRING: 
        {
            token.type = TOKEN_STRING;
            while(_at < code.len)
            {
                if (code.text[_at] == '"')
                {
                    *ms = MS_NON_MULTILINE;
                    break;
                }
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

        case '/':
        {
            token.type = TOKEN_OPERATOR;
            if (code.text[_at] == '/' || code.text[_at] == '*')
            {
				if (code.text[_at] == '*')
					*ms = MS_COMMENT;

                token.type = TOKEN_COMMENT;
				token.textLength = code.len;
                _at = code.len;   
            }
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
            //starting quotation
            int start = _at;
            while(_at < code.len && code.text[_at] != '"') ++_at;
            token.textLength = _at - start + 1; //end - start + quotation
            
            if (code.text[_at] == '"')
            {
                ++token.textLength;
                ++_at;    
            }
			else
			{
				*ms = MS_STRING;
			}
        } break;

        //TODO: This doesn't work, fix it
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
                
                if (CompareStrings(code.text + token.textAt, token.textLength, "#define", 7))
                {
                    //Get what's actually defined
                    int defStart = _at;
					while (IsWhiteSpace(code.text[defStart])) ++defStart;

                    int defEnd = defStart;
                    while (!IsWhiteSpace(code.text[defEnd])) ++defEnd;
                    int defLen = defEnd - defStart;

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
                token.textLength += _at - start;
                
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
            }
            else if (IsNumeric(c))
            {
                int start = _at;
                while (IsNumeric(code.text[_at])) ++_at;  
                token.textLength += _at - start;
                char* tokenStr = code.text + token.textAt;
                
                token.type = (IsNumber(tokenStr, token.textLength)) ? TOKEN_NUMBER : TOKEN_UNKNOWN;
            }
            else
            {
                token.type = TOKEN_UNKNOWN;
            }
        } break;
        
    }

    *at = _at; 
    return token;
}

TokenInfo TokeniseLine(Line code, MultilineState* multilineState)
{
    //The number of tokens in a line should not exceed that line's length 
    TokenInfo result = {};
    if (code.len == 0) return result;
    result.tokens = HeapAlloc(Token, code.len); 

    int at = 0;
    bool parsing = true;
    while (parsing)
    {
        Token token = GetTokenFromLine(code, &at, multilineState);
        result.tokens[result.numTokens++] = token;
        parsing = (at != code.len);
    }

    return result;
}