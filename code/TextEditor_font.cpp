#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "TextEditor_defs.h"
#include "TextEditor_font.h"
#include "TextEditor.h"

void ResizeFont(int fontSizeIndex)
{
    stbtt_fontinfo fontInfo;
    uchar* ttfFile = ReadEntireFileUChar("fonts/consolab.ttf", 0);
    stbtt_InitFont(&fontInfo, ttfFile, stbtt_GetFontOffsetForIndex(ttfFile, 0));

    int offsetAboveBaseline, offsetBelowBaseline, lineGap;
    stbtt_GetFontVMetrics(&fontInfo, &offsetAboveBaseline, &offsetBelowBaseline, &lineGap);
    float scale = stbtt_ScaleForPixelHeight(&fontInfo, 
                                            (float)PointsToPix(fontSizes[fontData.sizeIndex]));

    fontData.maxHeight = (uint32)RoundToInt((offsetAboveBaseline - offsetBelowBaseline) * scale);
    fontData.lineGap = (uint32)RoundToInt(lineGap * scale);
    fontData.offsetBelowBaseline = (uint32)RoundToInt(-offsetBelowBaseline * scale);

    for (uchar c = 0; c < 128; ++c)
    {
        FontChar fc;

        stbtt_FreeBitmap(fontData.chars[c].pixels, 0);
        
        int width, height, xOffset, yOffset, advance, lsb;
        fc.pixels = stbtt_GetCodepointBitmap(&fontInfo, 0, scale, c, &width, &height, &xOffset, &yOffset);
        stbtt_GetCodepointHMetrics(&fontInfo, c, &advance, &lsb);

        fc.width = width;
        fc.height = height;
        fc.left = xOffset;
        fc.top = yOffset;
        fc.advance = (uint32)RoundToInt(advance * scale);
        fontData.chars[c] = fc;
    } 

    FreeWin32(ttfFile);
}

void ChangeFont(string ttfFileName)
{
    //All the FTT shit in here
    //FT_Done_Face frees a ft_face I think
}