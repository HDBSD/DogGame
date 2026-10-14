#ifndef TITLE_GEOMETRY_TYPES_H
#define TITLE_GEOMETRY_TYPES_H
#include <math.h>

typedef struct Vector3
{
    float x, y, z;
} Vector3;

typedef struct CollisionTriangle
{
    Vector3 p[3];
    unsigned color;
} CollisionTriangle;

typedef struct Trigger
{
    Vector3 p[8];
    unsigned color;
    char label[64];
} Trigger;

enum
{
    MAX_TRIGGERS = 1024,
    MAX_TRIANGLES = 131072
};

static Vector3 vector_subtract(Vector3 a, Vector3 b)
{
    Vector3 v = {a.x - b.x, a.y - b.y, a.z - b.z};
    return v;
}

static Vector3 vector_lerp(Vector3 a, Vector3 b, float t)
{
    Vector3 v = {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t};
    return v;
}

static float vector_dot(Vector3 a, Vector3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static int vector_is_valid(Vector3 v)
{
    return isfinite(v.x) && isfinite(v.y) && isfinite(v.z) && fabsf(v.x) < 100000 && fabsf(v.y) < 100000 &&
           fabsf(v.z) < 100000;
}

#endif
