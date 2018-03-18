#include "shared/types.h"


#define BAL_PTR(ref) \
    ((void*)((char *)&(ref) + (ref.offset))

#define BAL_REF_TYPE(type) \
    union type##Ref { \
        s32 offset; \
        s32 offset_##type; \
     };

#define BAL_PTR_FROM_REF(type, ref) \
    ((type *)((char *)&(ref) + (ref).offset_##type))

#define BAL_SET_REF_PTR(ref, ptr) \
     (ref).offset = (s32)((char *)(ptr) - (char *)&(ref))

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

struct BalDescriptor {
    u32 type;
    struct BalRef ref;
};

struct BalDescriptorTable {
    u32 descriptor_count;
    struct BalDescriptor descriptors[1];
};

BAL_REF_TYPE(BalDescriptorTable)

struct BalHeader {
    u32 magic;
    union BalDescriptorTableRef descriptor_tables;
};
