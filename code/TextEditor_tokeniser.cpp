#include "TextEditor_defs.h"
#include "TextEditor.h"
#include "TextEditor_string.h"
#include "TextEditor_tokeniser.h"
#include "TextEditor_dynarray.h"

#define INITIAL_TYPEDEFS_SIZE 64

union TokenColours
{
    struct 
    {
        Colour punctuationColour;
        Colour operatorColour;
        Colour stringColour;
        Colour numberColour;
        Colour boolColour;
        Colour identifierColour;
        Colour functionColour;
        Colour customTypeColour;
        Colour inbuiltTypeColour;
        Colour keywordColour;
        Colour preprocessorColour;
        Colour defineColour;
        Colour commentColour;
        Colour unknownColour;
    };
    Colour colours[NUM_TOKENS - 1];
};

struct LineView
{
    int lineIndex;
    string text;
};

//extern TokenInfo tokenInfo; //TODO: Make this internal

TokenColours tokenColours;

//DefinedTokenHashSet types = InitHashSet();
LineView typedefs[INITIAL_TYPEDEFS_SIZE];
int numTypedefs = 0;

//DefinedTokenHashSet defines = InitHashSet();
LineView poundDefines[INITIAL_TYPEDEFS_SIZE];
int numPoundDefines = 0;

void LoadTokenColours()
{
    //TODO: Log error or something
    string file = ReadEntireFileAsString(lstring("config/config_tokeniser.txt"));
    Assert(file.str);

    string line = GetNextLine(&file);
    for (int i = 0; line[0]; ++i)
    {
        string val = SubString(line, IndexOfCharInString(line, ' ') + 1);

        Colour colour;
        bool success = true;

        byte r = StringToByte(AdvanceToCharAndSplitString(&val, ','), &success);
        Assert(success);
        colour.r = r;

        byte g = StringToByte(AdvanceToCharAndSplitString(&val, ','), &success);
        Assert(success);
        colour.g = g;

        byte b = StringToByte(AdvanceToCharAndSplitString(&val, ','), &success);
        Assert(success);
        colour.b = b;

        tokenColours.colours[i] = colour;

        line = GetNextLine(&file);
    }

    FreeWin32(file.str);
}

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

void AddTypeNameForTypedef(Editor* editor, EditorPos at)
{
    string_buf currentLine = editor->lines[at.line];

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
            if (at.line == editor->numLines) break;
			currentLine = editor->lines[at.line];
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
    if (at.line < editor->numLines)
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
    lstring("short"), 
    lstring("long"), 
    lstring("float"), 
    lstring("double"), 
    lstring("char"), 
    lstring("void"), 
    lstring("bool"), 
    lstring("struct"), 
    lstring("class"), 
    lstring("union"), 
    lstring("enum"), 
    lstring("unsigned"),
    lstring("namespace"),
    lstring("auto")
};

string keywords[] = 
{
    lstring("return"), 
    lstring("static"), 
    lstring("const"), 
    lstring("if"), 
    lstring("else"), 
    lstring("switch"),
    lstring("case"),
    lstring("default"),
    lstring("for"), 
    lstring("do"),
    lstring("while"), 
    lstring("break"),  
    lstring("typedef"),
    lstring("inline"),
    lstring("extern"),
    lstring("using"),
    lstring("volatile")
};

string preprocessorTags[] = 
{
    lstring("#include"), 
    lstring("#define"), 
    lstring("#undef"), 
    lstring("#if"), 
    lstring("#elif"), 
    lstring("#else"), 
    lstring("#ifdef"), 
    lstring("#ifndef"),
    lstring("#endif"),
    lstring("#error"),
    lstring("#pragma")
};

//TODO: Make this just get next token or something cause now I realise I need to pass in editor and doing it by line is meaningless now
Token GetTokenFromLine(Editor* editor, int lineIndex, int* lineAt, MultilineState* ms)
{
    string_buf code = editor->lines[lineIndex];

    if (code.len == 0) return {EditorPos{0, lineIndex}, string{0}, TOKEN_UNKNOWN};

	int at = *lineAt;
	while (IsWhiteSpace(code[at]) && at < code.len) ++at;
   

    Token token = {};
	token.text.str = code.str + at;
    token.at = {at, lineIndex};
	if (at == code.len)
	{
		token.text.len = 0;
		token.type = TOKEN_UNKNOWN;
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
            token.type = TOKEN_UNKNOWN; 
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
        case '>':   //NOTE: Don't need to check for arrow operator because the above skips over the > if it is an arrow operator
            token.type = TOKEN_OPERATOR;      
            break;

        case '-': 
        {
            token.type = TOKEN_OPERATOR;

            if (code[at] == '>')
            {
                token.type = TOKEN_PUNCTUATION;
                token.text.len = 2;
                at++;
            } 
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

        case '\'':
        case '"':
        {
            token.type = TOKEN_STRING;

            int start = at;
            while(at < code.len && (code[at] != c || code[at - 1] == '\\')) ++at;
            token.text.len = at - start + 1;
            
            if (at < code.len && code[at] == c)
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
                        AddTypeNameForTypedef(editor, {at, lineIndex});
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
    result.lineSkipIndicies = HeapAlloc(int, result.size);
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

void Tokenise(int editorIndex)
{
    if (numEditors > 3) return;

    Editor* editor = &editors[editorIndex];
    if (!IsTokenisable(editor->fileName.toStr())) return; 

    TokenInfo* tokenInfo = &tokenInfos[editorIndex];

    Assert(editor->numLines < MAX_LINES);

    numTypedefs = 0;
    numPoundDefines = 0;
    tokenInfo->numTokens = 0;

    int tokenIndex = 0;
    MultilineState multilineState = MS_NON_MULTILINE;
    for (int i = 0; i < editor->numLines; ++i)
    {
		int lineAt = 0;
        bool parsingLine = true;

        tokenInfo->lineSkipIndicies[i] = tokenIndex;

        while (parsingLine)
        {
            Token token = GetTokenFromLine(editor, i, &lineAt, &multilineState);

            tokenInfo->tokens[tokenInfo->numTokens++] = token;
            if (tokenInfo->numTokens >= tokenInfo->size)
            {
                tokenInfo->size *= 2;
                tokenInfo->tokens = HeapRealloc(Token, tokenInfo->tokens, tokenInfo->size);
                tokenInfo->lineSkipIndicies = HeapRealloc(int, tokenInfo->lineSkipIndicies, tokenInfo->size);
            }
            
            parsingLine = (lineAt < editor->lines[i].len);
            tokenIndex++;
        }
    }
}

void OnFileOpen()
{
    Tokenise(numEditors - 1);
}

void OnTextChanged()
{
    Tokenise(openEditorIndexes[currentEditorSide]);
}

void OnEditorSwitch()
{
    Tokenise(openEditorIndexes[currentEditorSide]);
}

void OnFileSave()
{
	if (IsTokenisable(editors[openEditorIndexes[currentEditorSide]].fileName.toStr()))
        Tokenise(openEditorIndexes[currentEditorSide]);
}

//TODO: Put this in api function
void HighlightSyntax()
{
    for (int e = 0; e < 2; ++e)
    {
        Editor* editor = &editors[openEditorIndexes[e]];
        if (!IsTokenisable(editor->fileName.toStr())) continue; 

        int numLinesOnScreen = screenBuffer.height / (int)(fontData.maxHeight + fontData.lineGap);
        int firstLine = abs(editor->textOffset.y) / (int)(fontData.maxHeight + fontData.lineGap);

        TokenInfo tokenInfo = tokenInfos[openEditorIndexes[e]];

        const IntPair textStart = (e == 0) ? GetLeftTextStart() : GetRightTextStart();
        const Rect textLimits = (e == 0) ? GetLeftTextLimits() : GetRightTextLimits();

        int x = 0;
        int numLinesTokenised = 0; 
        int t = tokenInfo.lineSkipIndicies[firstLine];
        while (numLinesTokenised < numLinesOnScreen && t < tokenInfo.numTokens)
        {
            Token token = tokenInfo.tokens[t];

            bool prevTokenOnSameLine = t > 0 && tokenInfo.tokens[t - 1].at.line == token.at.line;
            bool nextTokenOnSameLine = t < tokenInfo.numTokens - 1 && 
                                        tokenInfo.tokens[t + 1].at.line == token.at.line;

            if (t == 0 || !prevTokenOnSameLine)
                x = textStart.x - editor->textOffset.x;
            int y = textStart.y - token.at.line * (int)(fontData.maxHeight + fontData.lineGap) 
                    + editor->textOffset.y;

            //Draw whitespace at front of line
            if (!prevTokenOnSameLine)
            {
                if (token.text.str) //TODO: Investigate whether this check is really necessary
                {
                    int whitespaceLen = (int)(token.text.str - editor->lines[token.at.line].str);
                    x += TextPixelLength(editor->lines[token.at.line].str, whitespaceLen);
                }
            }

            string text = token.text;
            Colour textColour = tokenColours.colours[token.type];

            //Draw token
            DrawText(text, x, y, textColour, textLimits);
            x += TextPixelLength(text);

            //Draw whitespace
            if (t < tokenInfo.numTokens - 1 && token.text.len > 0 && nextTokenOnSameLine)
            {
                text.str += token.text.len;
                int remainderLength = (int)(tokenInfo.tokens[t + 1].text.str - token.text.str) 
                                        - token.text.len;
                text.len = remainderLength;
                DrawText(text, x, y, textColour, textLimits);
                x += TextPixelLength(text);
            }

            t++;
            numLinesTokenised += !nextTokenOnSameLine;
        }
    }

}