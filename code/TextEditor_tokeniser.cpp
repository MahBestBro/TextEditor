#include "TextEditor_defs.h"
#include "TextEditor.h"
#include "TextEditor_string.h"
#include "TextEditor_tokeniser.h"
#include "TextEditor_dynarray.h"

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

const string integerSuffixes[] = 
{
    lstring("u"), 
    lstring("l"),  

    lstring("ul"), lstring("lu"),
    lstring("Ul"), lstring("lU"), 
    lstring("uL"), lstring("Lu"), 
    lstring("UL"), lstring("LU"),

    lstring("ull"), lstring("llu"),
    lstring("uLL"), lstring("LLu"),
    lstring("Ull"), lstring("llU"),
    lstring("ULL"), lstring("LLU")
};

//TODO: also include suffixes
bool IsNumber(string str)
{
    string suffix = str;
    //Checking for binary and hexadecimal numbers
	if (str.len >= 3 && CharInString(str[1], lstring("xXbB")))
    {
        if (str[0] != '0') return false;

        suffix.str += 2;
        suffix.len -= 2;
    }
    
    //Get suffix of number
    bool encounteredDecimalPoint = false;
    while(suffix.len > 0 && (IsNumeric(suffix[0]) || suffix[0] == '.'))
    {
        if (suffix[0] == '.') 
        {
            if (encounteredDecimalPoint) return false;
            encounteredDecimalPoint = true;
        }
        suffix.str++;
        suffix.len--;
    }

    //Check all the suffix cases
    if (suffix.len == 0) return true;
    if (encounteredDecimalPoint) return suffix == "f" || suffix == "l";
        
    for (int i = 0; i < StackArrayLen(integerSuffixes); ++i)
    {
        if (suffix == integerSuffixes[i]) return true;
    }
    return false;

    
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

bool DefinitionExists(bool isTypedef, Token token)
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

inline bool PoundDefineExists(Token token)
{
    return DefinitionExists(false, token);
}

inline bool TypedefExists(Token token)
{
    return DefinitionExists(true, token);
}

void AddTypeNameForTypedef(EditorPos at)
{
    string_buf currentLine = editor.lines[at.line];

    //Skip over first word
    while (at.textAt < currentLine.len && !IsWhiteSpace(currentLine[at.textAt]))
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
			char charAt = currentLine[at.textAt];
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
        while (typeStart < currentLine.len && !IsWhiteSpace(currentLine[typeStart])) 
            typeStart--;
        string typeText = {currentLine.str + typeStart + 1, at.textAt - typeStart - 1};

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

Token GetTokenFromLine(string_buf code, int lineIndex, int* lineAt, MultilineState* ms)
{
    if (code.len == 0) return {EditorPos{0, lineIndex}, string{0}, TOKEN_UNKNOWN};

	int at = *lineAt;
	while (IsWhiteSpace(code[at]) && at < code.len) ++at;
   

    Token token = {};
	token.text.str = code.str + at;
    token.at = {at, lineIndex};
	if (at == code.len)
	{
		token.text.len = 0;
		token.type = TOKEN_EOS;
        *lineAt = code.len;
        return token;
	}

    switch(*ms)
    {
        case MS_COMMENT:
        {
            token.type = TOKEN_COMMENT;
            
            int start = at;
            at++;
            while(at < code.len)
            {
                if (code[at] == '/' && code[at - 1] == '*')
                {
                    *ms = MS_NON_MULTILINE;
                    break;
                }

                at++;
            } 

            token.text.len = at - start + (at < code.len);
            *lineAt = at + (at < code.len);
        } return token;

        case MS_STRING: 
        {
            token.type = TOKEN_STRING;

            int start = at;
            while(at < code.len)
            {
                if (code[at] == '"')
                {
                    *ms = MS_NON_MULTILINE;
                    break;
                }
                at++;
            } 

            token.text.len = at - start + (at < code.len);
            *lineAt = at + (at < code.len);
        } return token;

        case MS_NON_MULTILINE: break;
    }

    token.text.len = 1;
    char c = code[at];
    ++at;

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
            if (code[at] == '>')
            {
                token.type = TOKEN_PUNCTUATION;
                token.text.len = 2;
                at++;
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
            if (code[at] == '/' || code[at] == '*')
            {
				if (code[at] == '*')
					*ms = MS_COMMENT;

                token.type = TOKEN_COMMENT;
				token.text.len = code.len - at + 1;
                at = code.len;   
            }
        } break;

        case '<':
        {
            token.type = TOKEN_OPERATOR;

            string startOfLineText = {code.str, 8};
            while (IsWhiteSpace(startOfLineText[0])) ++startOfLineText.str;
            
            if (startOfLineText == lstring("#include"))
            {
                token.type = TOKEN_STRING;
                
                int start = at;
                while(at < code.len && code[at] != '>') at++;
                at += (at < code.len && code[at] == '>');
                token.text.len = at - start + 1;
            }
        } break;

        case '"':
        {
            //TODO: Handle \"
            token.type = TOKEN_STRING;

            int start = at;
            while(at < code.len && code[at] != '"') ++at;
            token.text.len = at - start + 1;
            
            if (at < code.len && code[at] == '"')
            {
                ++token.text.len;
                ++at;    
            }
			else
			{
				*ms = MS_STRING;
			}
        } break;

        case '#':
        {
            token.type = TOKEN_UNKNOWN;

            int start = at;
            while (IsAlphabetical(code[at]) && at < code.len)
            {
                ++at;
            }
            token.text.len += at - start;

            if (IsInStringArray(token.text, preprocessorTags, StackArrayLen(preprocessorTags)))
            {
                token.type = TOKEN_PREPROCESSOR_TAG;

                
                if (token.text == lstring("#define"))
                {
                    //Get what's actually defined
                    int defStart = at;
					while (defStart < code.len && IsWhiteSpace(code[defStart])) 
                        ++defStart;

                    int defEnd = defStart;
                    while (defEnd < code.len && !IsWhiteSpace(code[defEnd])) 
                        ++defEnd;

                    string poundDefineText = {code.str + defStart, defEnd - defStart};
					if (poundDefineText.len > 0)
                        poundDefines[numPoundDefines++] = {lineIndex, poundDefineText};
                }
            }  
        } break;

        default:
        {
            token.type = TOKEN_UNKNOWN;

            if (IsAlphabetical(c))
            {
                int start = at;
                while (at < code.len && (IsAlphaNumeric(code[at]) || code[at] == '_'))
                {
                    ++at;
                }
                token.text.len += at - start;

                token.type = TOKEN_IDENTIFIER;
                if (IsInStringArray(token.text, keywords, StackArrayLen(keywords)))
                {
                    token.type = TOKEN_KEYWORD;

                    if (token.text == lstring("typedef"))
                        AddTypeNameForTypedef({at, lineIndex});
                }
                else if (code[at] == '(')
                {
                    token.type = TOKEN_FUNCTION;
                }
                else if (IsInStringArray(token.text, inbuiltTypes, StackArrayLen(inbuiltTypes))) 
                {
                    token.type = TOKEN_INBUILT_TYPE;
                    
                    //If a struct or enum, add the type
                    if (token.text == lstring("struct") || token.text == lstring("enum"))
                    {
                        int typeStart = at;
                        while (typeStart < code.len && IsWhiteSpace(code[typeStart])) 
                            ++typeStart;

                        int typeEnd = typeStart;
                        while (typeStart < code.len && !IsWhiteSpace(code[typeEnd])) 
                            ++typeEnd;

                        string typeText = {code.str + typeStart, typeEnd - typeStart};
                        if (typeText.len > 0)
                            typedefs[numTypedefs++] = {lineIndex, typeText};
                    }
                }
                else if (IsBool(token.text))
                {
                    token.type = TOKEN_BOOL;
                }
                else if (PoundDefineExists(token))
                {
                    token.type = TOKEN_DEFINE;
                }
                else if (TypedefExists(token))
                {
                    token.type = TOKEN_CUSTOM_TYPE;
                } 
                
            }
            else if (IsNumeric(c))
            { 
                int start = at;
                while (at < code.len && (IsAlphaNumeric(code[at]) || code[at] == '.'))
                {
                    ++at;
                }
                token.text.len += at - start;

                if (IsNumber(token.text)) token.type = TOKEN_NUMBER;
            }
        } break;
        
    }

    *lineAt = at; 
    return token;
}


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
    if (!IsTokenisable(editor.fileName.toStr())) return; 

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
            parsing = (lineAt < editor.lines[i].len);
        }
    }
}