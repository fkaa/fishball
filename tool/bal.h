#include "shared/types.h"

#define BAL_PTR(ref) \
    ((void*)((char *)&(ref) + (ref).offset))
#define BAL_REF_TYPE(type) \
    union type##Ref { \
        s32 offset; \
        s32 offset_##type; \
     };
#define BAL_PTR_FROM_REF(type, ref) \
    ((type *)((char *)&(ref) + (ref).offset_##type))
#define BAL_SET_REF_PTR(ref, ptr) \
     (ref).offset = (s32)((char *)(ptr) - (char *)&(ref))
#define BAL_GET_SIZE(type, array_field, count) \
    (sizeof(type) - sizeof(((type *)0)->array_field[0]) + sizeof(((type *)0)->array_field[0]) * (count))
#define BAL_FOURCC(code) (code)

enum {
    BAL_MAGIC = BAL_FOURCC('BALL')
};

enum {
    BAL_TYPE_FONT = BAL_FOURCC('FONT')
};

struct BalRef {
    s32 offset;
};

BAL_REF_TYPE(BalDescriptorTable)
BAL_REF_TYPE(BalBuffer)
BAL_REF_TYPE(BalFont)

struct BalDescriptor {
    u32 type;
    struct BalRef ref;
};

struct BalDescriptorTable {
    u32 descriptor_count;
    struct BalDescriptor descriptors[1];
};

struct BalBuffer {
    u32 size;
    u8 data[1];
};

struct BalBufferView {
    u32 offset;
    u32 stride;
    union BalBufferRef buffer;
};

struct BalGlyph {
    u16 codepoint;
    u8 width, height;
    s8 xoff, yoff;
    s8 xadvance;
    u32 data_offset;
};

struct BalFont {
    u32 glyph_count;
    union BalBufferRef buffer;
    struct BalGlyph glyphs[1];
};

struct BalHeader {
    u32 magic;
    union BalDescriptorTableRef descriptor_tables;
};
