#ifndef FB_MATHTYPES_H
#define FB_MATHTYPES_H

#include "shared/types.h"

struct FbVec3 {
    r32 x, y, z;
};

struct FbVec4 {
    r32 x, y, z, w;
};

struct FbMatrix4 {
    r32 m[4][4];
};

struct FbQuaternion {
    r32 x, y, z, w;
};

#endif /* FB_MATHTYPES_H */

