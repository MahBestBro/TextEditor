#include "TextEditor_defs.h"
#include "TextEditor.h"
#include "TextEditor_string.h"
#include "TextEditor_tokeniser.h"

#define INITIAL_TYPEDEFS_SIZE 64

struct LineView
{
    int lineIndex;
    int textAt;
    int textLen;
};

extern Editor editor;

//DefinedTokenHashSet types = InitHashSet();
internal LineView typedefs[INITIAL_TYPEDEFS_SIZE];
internal int numTypedefs = 0;

//DefinedTokenHashSet defines = InitHashSet();
internal LineView poundDefines[INITIAL_TYPEDEFS_SIZE];
internal int numPoundDefines = 0;

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

bool DefinitionExists(bool isTypedef, Token token, Line code)
{
    LineView* definitions = (isTypedef) ? typedefs : poundDefines;
    int numDefinitions = (isTypedef) ? numTypedefs : numPoundDefines;
    for (int i = 0; i < numDefinitions; ++i)
    {
        char* defText = editor.lines[definitions[i].lineIndex].text + definitions[i].textAt;
        char* tokenText = code.text + token.at.textAt;
        if (CompareStrings(defText, definitions[i].textLen, tokenText, token.textLength))
            return true;
    } 
    return false;
}

inline bool PoundDefineExists(Token token, Line code)
{
    return DefinitionExists(false, token, code);
}

inline bool TypedefExists(Token token, Line code)
{
    return DefinitionExists(true, token, code);
}

void AddTypeNameForTypedef(EditorPos at)
{
    Line currentLine = editor.lines[at.line];

    //Skip over first word
    while (at.textAt < currentLine.len && !IsWhiteSpace(currentLine.text[at.textAt]))
        at.textAt++;

    //While we haven't hit a semicolon, also skip over curly crackets
    int scopeLevel = 0;
    while (true)
    {
        if (at.textAt == currentLine.len)
        {
			at.line++;
            if (at.line == editor.numLines) break;
			currentLine = editor.lines[at.line];
            at.textAt = 0;
        }

		if (currentLine.len > 0)
		{
			char charAt = currentLine.text[at.textAt];
			scopeLevel += charAt == '{';
			scopeLevel -= charAt == '}';
			if (scopeLevel == 0 && charAt == ';') break;
			at.textAt++;
		}
    }

    //Get all text behind semicolon on line and add it as a type
    if (at.line < editor.numLines)
    {
        int typeStart = at.textAt;
        while (typeStart < currentLine.len && !IsWhiteSpace(currentLine.text[typeStart])) 
            typeStart--;
        int typeLen = at.textAt - typeStart - 1;

        typedefs[numTypedefs++] = {at.line, typeStart + 1, typeLen};
    }
}

Token GetTokenFromLine(Line code, int lineIndex, int* at, MultilineState* ms)
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
    token.at = {_at, lineIndex};
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

    //if (*wantType)

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
            char* startOfLineText = code.text;
            while (IsWhiteSpace(startOfLineText[0])) ++startOfLineText;
            if (CompareStrings(startOfLineText, 8, "#include", 8))
            {
                token.type = TOKEN_STRING;
                
                int start = _at;
                while(code.text[_at] != '>' && _at < code.len) _at++;
                _at += (code.text[_at] == '>');
                token.textLength = _at - start + 1;
            }
            else
            {
                token.type = TOKEN_OPERATOR;
            }
        } break;

        case '"':
        {
            token.type = TOKEN_STRING;

            int start = _at;
            while(_at < code.len && code.text[_at] != '"') ++_at;
            token.textLength = _at - start + 1; //+1 to include quotation
            
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

        case '#':
        {
            int start = _at;
            while (IsAlphabetical(code.text[_at]) && _at < code.len)
            {
                ++_at;
            }
            token.textLength += _at - start;

            if (IsInStringArray(code.text + token.at.textAt, token.textLength, 
                preprocessorTags, StackArrayLen(preprocessorTags)))
            {
                token.type = TOKEN_PREPROCESSOR_TAG;

                
                if (CompareStrings(code.text + token.at.textAt, token.textLength, "#define", 7))
                {
                    //Get what's actually defined
                    int defStart = _at;
					while (defStart < code.len && IsWhiteSpace(code.text[defStart])) 
                        ++defStart;

                    int defEnd = defStart;
                    while (defStart < code.len && !IsWhiteSpace(code.text[defEnd])) 
                        ++defEnd;
                    int defLen = defEnd - defStart;

					if (defLen > 0)
                    {
                        poundDefines[numPoundDefines++] = {lineIndex, defStart, defLen}; 
                    }
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
                while (_at < code.len && (IsAlphaNumeric(code.text[_at]) || code.text[_at] == '_'))
                {
                    ++_at;
                }
                token.textLength += _at - start;
                
                char* tokenStr = code.text + token.at.textAt;
                if (IsInStringArray(tokenStr, token.textLength, keywords, StackArrayLen(keywords)))
                {
                    token.type = TOKEN_KEYWORD;

                    //If typedef, add type
                    if (CompareStrings(tokenStr, token.textLength, "typedef", 7))
                    {
                        AddTypeNameForTypedef({_at, lineIndex});
                    }
                }
                else if (code.text[_at] == '(')
                {
                    token.type = TOKEN_FUNCTION;
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
                        while (typeStart < code.len && IsWhiteSpace(code.text[typeStart])) 
                            ++typeStart;

                        int typeEnd = typeStart;
                        while (typeStart < code.len && !IsWhiteSpace(code.text[typeEnd])) 
                            ++typeEnd;
                        int typeLen = typeEnd - typeStart;

                        if (typeLen > 0)
                        {
                            typedefs[numTypedefs++] = {lineIndex, typeStart, typeLen};
                        }
                    }
                }
                else if (IsBool(tokenStr, token.textLength))
                {
                    token.type = TOKEN_BOOL;
                }
                else if (PoundDefineExists(token, code))
                {
                    token.type = TOKEN_DEFINE;
                }
                else if (TypedefExists(token, code))
                {
                    token.type = TOKEN_CUSTOM_TYPE;
                } 
                
            }
            else if (IsNumeric(c))
            {
                //TODO: also include letter at end of number 
                int start = _at;
                while (IsNumeric(code.text[_at])) ++_at;  
                token.textLength += _at - start;
                char* tokenStr = code.text + token.at.textAt;
                
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

//Maybe make editor variable global across files so that we only need to pass index?
/*TokenInfo TokeniseLine(Line code, int lineIndex, MultilineState* multilineState)
{
    //The number of tokens in a line should not exceed that line's length 
    TokenInfo result = {};
    if (code.len == 0) return result;
    result.tokens = HeapAlloc(Token, code.len); 

    int at = 0;
    bool parsing = true;
    while (parsing)
    {
        Token token = GetTokenFromLine(code, lineIndex, &at, multilineState);
        result.tokens[result.numTokens++] = token;
        parsing = (at != code.len);
    }

    return result;
}*/

TokenInfo InitTokenInfo()
{
    TokenInfo result;
    result.tokens = HeapAlloc(Token, result.size);
    return result;
}

void Tokenise(TokenInfo* dest)
{
    Assert(editor.numLines < MAX_LINES);

    numTypedefs = 0;
    numPoundDefines = 0;
    dest->numTokens = 0;

    MultilineState multilineState = MS_NON_MULTILINE;
    for (int i = 0; i < editor.numLines; ++i)
    {
        int lineAt = 0;
        bool parsing = true;
        while (parsing)
        {
            Token token = GetTokenFromLine(editor.lines[i], i, &lineAt, &multilineState);
            AppendToDynamicArray(dest->tokens, dest->numTokens, token, dest->size);
            parsing = (lineAt != editor.lines[i].len);
        }
    }
}