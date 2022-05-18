#ifndef TEXT_EDITOR_META_H
#define TEXT_EDITOR_META_H

#define StructOffset(structType, member) (uint64)&((structType*)0)->member
#define MemberPtr(structInstance, offset) (((byte*)&structInstance) + offset)

enum MetaType
{
    TYPE_Colour,
    TYPE_ColourRGBA,
    TYPE_int,
    TYPE_bool,
    TYPE_string
};

struct MemberMetaData
{
    MetaType type;
    uint64 offset;
};

#endif