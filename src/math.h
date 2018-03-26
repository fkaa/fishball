#ifndef FB_MATH_H
#define FB_MATH_H

#include "mathtypes.h"

#include <math.h>

r32 vec3_length(struct FbVec3 vec);
r32 vec3_dot(struct FbVec3 a, struct FbVec3 b);
struct FbVec3 vec3_normalize(struct FbVec3 vec);
struct FbVec3 vec3_cross(struct FbVec3 a, struct FbVec3 b);
struct FbVec3 vec3_sub(struct FbVec3 a, struct FbVec3 b);
struct FbVec3 vec3_mul(struct FbVec3 a, struct FbVec3 b);
struct FbVec3 vec3_mul_scalar(struct FbVec3 a, r32 s);
struct FbMatrix4 mat4_identity();
struct FbMatrix4 mat4_mul(struct FbMatrix4 a, struct FbMatrix4 b);
struct FbMatrix4 mat4_perspective_RH(r32 fov, r32 aspect, r32 near, r32 far);
struct FbMatrix4 mat4_look_at_RH(struct FbVec3 pos, struct FbVec3 target, struct FbVec3 up);
struct FbMatrix4 mat4_ortho_RH(r32 top, r32 bottom, r32 left, r32 right, r32 far, r32 near);

r32 vec3_dot(struct FbVec3 a, struct FbVec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

r32 vec3_length(struct FbVec3 vec)
{
    return sqrtf(vec3_dot(vec, vec));
}

struct FbVec3 vec3_normalize(struct FbVec3 vec)
{
    return vec3_mul_scalar(vec, 1.f / vec3_length(vec));
}

struct FbVec3 vec3_cross(struct FbVec3 a, struct FbVec3 b)
{
    return (struct FbVec3) { .x = (a.y * b.z - a.z * b.y) , .y = (a.z * b.x - a.x * b.z), .z = (a.x * b.y - a.y * b.x) };
}

struct FbVec3 vec3_sub(struct FbVec3 a, struct FbVec3 b)
{
    return (struct FbVec3) { a.x - b.x, a.y - b.y, a.z - b.z };
}

struct FbVec3 vec3_mul(struct FbVec3 a, struct FbVec3 b)
{
    return (struct FbVec3) { a.x * b.x, a.y * b.y, a.z * b.z };
}


struct FbVec3 vec3_mul_scalar(struct FbVec3 a, r32 s)
{
    return (struct FbVec3) { a.x * s, a.y * s, a.z * s };
}

struct FbMatrix4 mat4_identity()
{
    return (struct FbMatrix4) {{
        { 1.f, 0.f, 0.f, 0.f },
        { 0.f, 1.f, 0.f, 0.f },
        { 0.f, 0.f, 1.f, 0.f },
        { 0.f, 0.f, 0.f, 1.f }
    }};
}

struct FbMatrix4 mat4_mul(struct FbMatrix4 a, struct FbMatrix4 b)
{
    struct FbMatrix4 m;

    r32 x = a.m[0][0],
        y = a.m[0][1],
        z = a.m[0][2],
        w = a.m[0][3];

    m.m[0][0] = (b.m[0][0] * x) + (b.m[1][0] * y) + (b.m[2][0] * z) + (b.m[3][0] * w);
    m.m[0][1] = (b.m[0][1] * x) + (b.m[1][1] * y) + (b.m[2][1] * z) + (b.m[3][1] * w);
    m.m[0][2] = (b.m[0][2] * x) + (b.m[1][2] * y) + (b.m[2][2] * z) + (b.m[3][2] * w);
    m.m[0][3] = (b.m[0][3] * x) + (b.m[1][3] * y) + (b.m[2][3] * z) + (b.m[3][3] * w);

    x = a.m[1][0];
    y = a.m[1][1];
    z = a.m[1][2];
    w = a.m[1][3];

    m.m[1][0] = (b.m[0][0] * x) + (b.m[1][0] * y) + (b.m[2][0] * z) + (b.m[3][0] * w);
    m.m[1][1] = (b.m[0][1] * x) + (b.m[1][1] * y) + (b.m[2][1] * z) + (b.m[3][1] * w);
    m.m[1][2] = (b.m[0][2] * x) + (b.m[1][2] * y) + (b.m[2][2] * z) + (b.m[3][2] * w);
    m.m[1][3] = (b.m[0][3] * x) + (b.m[1][3] * y) + (b.m[2][3] * z) + (b.m[3][3] * w);

    x = a.m[2][0];
    y = a.m[2][1];
    z = a.m[2][2];
    w = a.m[2][3];

    m.m[2][0] = (b.m[0][0] * x) + (b.m[1][0] * y) + (b.m[2][0] * z) + (b.m[3][0] * w);
    m.m[2][1] = (b.m[0][1] * x) + (b.m[1][1] * y) + (b.m[2][1] * z) + (b.m[3][1] * w);
    m.m[2][2] = (b.m[0][2] * x) + (b.m[1][2] * y) + (b.m[2][2] * z) + (b.m[3][2] * w);
    m.m[2][3] = (b.m[0][3] * x) + (b.m[1][3] * y) + (b.m[2][3] * z) + (b.m[3][3] * w);

    x = a.m[3][0];
    y = a.m[3][1];
    z = a.m[3][2];
    w = a.m[3][3];

    m.m[3][0] = (b.m[0][0] * x) + (b.m[1][0] * y) + (b.m[2][0] * z) + (b.m[3][0] * w);
    m.m[3][1] = (b.m[0][1] * x) + (b.m[1][1] * y) + (b.m[2][1] * z) + (b.m[3][1] * w);
    m.m[3][2] = (b.m[0][2] * x) + (b.m[1][2] * y) + (b.m[2][2] * z) + (b.m[3][2] * w);
    m.m[3][3] = (b.m[0][3] * x) + (b.m[1][3] * y) + (b.m[2][3] * z) + (b.m[3][3] * w);

    return m;
}

struct FbMatrix4 mat4_perspective_RH(r32 fov, r32 aspect, r32 near, r32 far)
{
    r32 sinfov = sinf(fov * 0.5f),
        cosfov = cosf(fov * 0.5f);

    r32 height = cosfov / sinfov;
    r32 width  = height * aspect;
    r32 range = far / (near - far);
    r32 rn = -(far * near) / (far - near);

    return (struct FbMatrix4) {{
        { width, 0.f,    0.f,          0.f },
        { 0.f,   height, 0.f,          0.f },
        { 0.f,   0.f,    range,       -1.f },
        { 0.f,   0.f,    rn, 0.f }
    }};
}


struct FbMatrix4 mat4_look_at_RH(struct FbVec3 pos, struct FbVec3 target, struct FbVec3 up)
{
    struct FbVec3 Z = vec3_normalize(vec3_sub(target, pos));
    struct FbVec3 X = vec3_normalize(vec3_cross(Z, up));
    struct FbVec3 Y = vec3_cross(X, Z);

    return (struct FbMatrix4) {{
        { X.x,               Y.x,               -Z.x,              0.f },
        { X.y,               Y.y,               -Z.y,              0.f },
        { X.z,               Y.z,               -Z.z,              0.f },
        { -vec3_dot(X, pos), -vec3_dot(Y, pos), vec3_dot(Z, pos),  1.f }
    }};
}

struct FbMatrix4 mat4_ortho_RH(r32 top, r32 bottom, r32 left, r32 right, r32 far, r32 near)
{
    r32 w = 1.f / (right - left);
    r32 h = 1.f / (top - bottom);
    r32 range = 1.f / (near - far);

    return (struct FbMatrix4) {{
        { w + w,                0.f,                0.f,          0.f },
        { 0.f,                  h + h,              0.f,          0.f },
        { 0.f,                  0.f,                range,        0.f },
        { -(left + right) * w, -(top + bottom) * h, range * near, 1.f }
    }};
}

#endif /* FB_MATH_H */
