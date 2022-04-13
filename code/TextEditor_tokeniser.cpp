#include "TextEditor_defs.h"
#include "TextEditor.h"
#include "TextEditor_string.h"
#include "TextEditor_tokeniser.h"

#define INITIAL_TYPEDEFS_SIZE 64

struct LineView
{
    int lineIndex;
    string text;
};

extern Editor editor;

//DefinedTokenHashSet types = InitHashSet();
internal LineView typedefs[INITIAL_TYPEDEFS_SIZE];
internal int numTypedefs = 0;

//DefinedTokenHashSet defines = InitHashSet();
internal LineView poundDefines[INITIAL_TYPEDEFS_SIZE];
internal int numPoundDefines = 0;

//TODO: also include suffixes
bool IsNumber(string str)
{
    //Checking for binary and hexadecimal numbers
	bool isHexOrBinary = str.len >= 3 && CharInString(str[1], lstring("xXbB"));
    
    for (int i = 2 * (isHexOrBinary); i < str.len; ++i)
    {
        if (!IsNumeric(str[i]))
            return false;
    }
    return true;
}

bool IsBool(string str)
{
    return str == lstring("true") || str == lstring("false");
}

bool IsInStringArray(string str, string* stringArr, int arrLen)
{
    for (int i = 0; i < arrLen; ++i)
    {
        if (stringArr[i] == str)
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
        if (definitions[i].text == token.text)
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

    //While we haven't hit a semicolon, also skip over curly brackets
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
        string typeText = {currentLine.text + typeStart + 1, at.textAt - typeStart - 1};

        typedefs[numTypedefs++] = {at.line, typeText};
    }
}

string inbuiltTypes[] = 
{
    lstring("int"), 
    lstring("float"), 
    lstring("char"), 
    lstring("void"), 
    lstring("bool"), 
    lstring("struct"), 
    lstring("enum"), 
    lstring("unsigned"),
    lstring("namespace")
};

string keywords[] = 
{
    lstring("return"), 
    lstring("static"), 
    lstring("if"), 
    lstring("else"), 
    lstring("for"), 
    lstring("do"),
    lstring("while"), 
    lstring("typedef"),
    lstring("inline"),
    lstring("extern"),
    lstring("using")
};

string preprocessorTags[] = 
{
    lstring("#include"), 
    lstring("#define"), 
    lstring("#if"), 
    lstring("#elif"), 
    lstring("#else"), 
    lstring("#ifdef"), 
    lstring("#ifndef"),
    lstring("#endif")
};

Token GetTokenFromLine(Line code, int lineIndex, int* at, MultilineState* ms)
{
    if (code.len == 0) return {lineIndex, string{0}, TOKEN_UNKNOWN};

	int _at = *at;
	while (IsWhiteSpace(code.text[_at]) && _at < code.len) ++_at;

   

    Token token = {};
	token.text.str = code.text + _at;
    token.line = lineIndex;
	if (_at == code.len)
	{
		token.text.len = 0;
		token.type = TOKEN_EOS;
        *at = code.len;
        return token;
	}

    switch(*ms)
    {
        case MS_COMMENT:
        {
            token.type = TOKEN_COMMENT;
            
            int start = _at;
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

            token.text.len = _at - start + (_at < code.len);
            *at = _at + (_at < code.len);
        } return token;

        case MS_STRING: 
        {
            token.type = TOKEN_STRING;

            int start = _at;
            while(_at < code.len)
            {
                if (code.text[_at] == '"')
                {
                    *ms = MS_NON_MULTILINE;
                    break;
                }
                _at++;
            } 

            token.text.len = _at - start + (_at < code.len);
            *at = _at + (_at < code.len);
        } return token;

        case MS_NON_MULTILINE: break;
    }

    token.text.len = 1;
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
        case '\\':   
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
                token.text.len = 2;
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
				token.text.len = code.len - _at + 1;
                _at = code.len;   
            }
        } break;

        case '<':
        {
            token.type = TOKEN_OPERATOR;

            string startOfLineText = {code.text, 8};
            while (IsWhiteSpace(startOfLineText[0])) ++startOfLineText.str;
            
            if (startOfLineText == lstring("#include"))
            {
                token.type = TOKEN_STRING;
                
                int start = _at;
                while(code.text[_at] != '>' && _at < code.len) _at++;
                _at += (code.text[_at] == '>');
                token.text.len = _at - start + 1;
            }
        } break;

        case '"':
        {
            token.type = TOKEN_STRING;

            int start = _at;
            while(_at < code.len && code.text[_at] != '"') ++_at;
            token.text.len = _at - start + 1;
            
            if (code.text[_at] == '"')
            {
                ++token.text.len;
                ++_at;    
            }
			else
			{
				*ms = MS_STRING;
			}
        } break;

        case '#':
        {
            token.type = TOKEN_UNKNOWN;

            int start = _at;
            while (IsAlphabetical(code.text[_at]) && _at < code.len)
            {
                ++_at;
            }
            token.text.len += _at - start;

            if (IsInStringArray(token.text, preprocessorTags, StackArrayLen(preprocessorTags)))
            {
                token.type = TOKEN_PREPROCESSOR_TAG;

                
                if (token.text == lstring("#define"))
                {
                    //Get what's actually defined
                    int defStart = _at;
					while (defStart < code.len && IsWhiteSpace(code.text[defStart])) 
                        ++defStart;

                    int defEnd = defStart;
                    while (defEnd < code.len && !IsWhiteSpace(code.text[defEnd])) 
                        ++defEnd;

                    string poundDefineText = {code.text + defStart, defEnd - defStart};
					if (poundDefineText.len > 0)
                        poundDefines[numPoundDefines++] = {lineIndex, poundDefineText};
                }
            }  
        } break;

        default:
        {
            token.type = TOKEN_UNKNOWN;

            int start = _at;
            while (_at < code.len && (IsAlphaNumeric(code.text[_at]) || code.text[_at] == '_'))
            {
                ++_at;
            }
            token.text.len += _at - start;

            if (IsAlphabetical(c))
            {
                token.type = TOKEN_IDENTIFIER;
                if (IsInStringArray(token.text, keywords, StackArrayLen(keywords)))
                {
                    token.type = TOKEN_KEYWORD;

                    if (token.text == lstring("typedef"))
                        AddTypeNameForTypedef({_at, lineIndex});
                }
                else if (code.text[_at] == '(')
                {
                    token.type = TOKEN_FUNCTION;
                }
                else if (IsInStringArray(token.text, inbuiltTypes, StackArrayLen(inbuiltTypes))) 
                {
                    token.type = TOKEN_INBUILT_TYPE;
                    
                    //If a struct or enum, add the type
                    if (token.text == lstring("struct") || token.text == lstring("enum"))
                    {
                        int typeStart = _at;
                        while (typeStart < code.len && IsWhiteSpace(code.text[typeStart])) 
                            ++typeStart;

                        int typeEnd = typeStart;
                        while (typeStart < code.len && !IsWhiteSpace(code.text[typeEnd])) 
                            ++typeEnd;

                        string typeText = {code.text + typeStart, typeEnd - typeStart};
                        if (typeText.len > 0)
                            typedefs[numTypedefs++] = {lineIndex, typeText};
                    }
                }
                else if (IsBool(token.text))
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
            else if (IsNumber(token.text))
            { 
                token.type = TOKEN_NUMBER;
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

string tokenisableFileExtensions[] 
{
    lstring("cpp"),
    lstring("c"), 
    lstring("h") 
};

bool IsTokenisable(string fileName)
{
    int dotIndex = IndexOfLastCharInString(fileName, '.');
    if (dotIndex == -1) return false;

    string extension = SubString(fileName, dotIndex + 1);    
    for (int j = 0; j < StackArrayLen(tokenisableFileExtensions); ++j)
    {
        if (extension == tokenisableFileExtensions[j]) return true;
    }
    return false;
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