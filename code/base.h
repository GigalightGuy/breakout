#ifndef BREAKOUT_BASE_H_
#define BREAKOUT_BASE_H_

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float  f32;
typedef double f64;

typedef size_t    usize;
typedef ptrdiff_t isize;

constexpr double PI  = 3.14159265358979323846;
constexpr double TAU = 2*PI;

constexpr float F_PI  = (float)PI;
constexpr float F_TAU = (float)TAU;

#define LOG(...) fprintf(stderr, __VA_ARGS__)

#ifdef _WIN32
# define DEBUGBREAK() __debugbreak()
#else
# error Unknown platform
#endif // _WIN32

#define ASSERT(c) do { if (!(c)) { LOG("%s %d: assertion '%s' failed\n", __FILE__, __LINE__, #c); DEBUGBREAK(); } } while (0)

struct Arena {
    u8*   memory;
    usize capacity;
    usize offset;
};

#define push(arena, T)             (T*)allocate(arena, sizeof(T), alignof(T))
#define pushCount(arena, T, count) (T*)allocate(arena, (count)*sizeof(T), alignof(T))
#define pop(arena, memory)         (T*)deallocate(arena, memory)

static void* allocate(Arena* arena, usize size, usize alignment = 1) {
    ASSERT(alignment != 0);
    ASSERT(alignment == 1 || (alignment & 1) == 0);
    u8* memory = NULL;

    usize alignmentOffset = alignment - ((usize)arena->memory+arena->offset) & (alignment-1);
    alignmentOffset = alignmentOffset == alignment ? 0 : alignmentOffset;
    if ((arena->offset + alignmentOffset + size) > arena->capacity) {
        return NULL;
    }

    arena->offset += alignmentOffset;
    memory = arena->memory + arena->offset;
    arena->offset += size;

    return memory;
}

static void deallocate(Arena* arena, void* memory) {
    if (memory >= arena->memory && memory <= arena->memory + arena->offset) {
        arena->offset = (u8*)memory - arena->memory;
    }
}

struct Vec2 {
    float x;
    float y;
};

static Vec2 vec2(float x, float y) {
    return {x, y};
}

static Vec2 vec2(float a) {
    return vec2(a, a);
}

static Vec2 operator+(Vec2 a, Vec2 b) {
    return vec2(a.x + b.x, a.y + b.y);
}

static Vec2 operator+=(Vec2& a, Vec2 b) {
    a = a + b;
    return a;
}

static Vec2 operator-(Vec2 a) {
    return vec2(-a.x, -a.y);
}

static Vec2 operator-(Vec2 a, Vec2 b) {
    return a + (-b);
}

static Vec2 operator-=(Vec2& a, Vec2 b) {
    a = a - b;
    return a;
}

static Vec2 operator*(Vec2 a, Vec2 b) {
    return vec2(a.x * b.x, a.y * b.y);
}

static Vec2 operator*(Vec2 v, float a) {
    return v * vec2(a);
}

static Vec2 operator*(float a, Vec2 v) {
    return vec2(a) * v;
}

static Vec2 operator/(Vec2 v, float a) {
    return vec2(v.x / a, v.y / a);
}

static float dot(Vec2 a, Vec2 b) {
    return a.x * b.x + a.y * b.y;
}

static float length(Vec2 a) {
    return sqrtf(dot(a,a));
}

static Vec2 normalize(Vec2 a) {
    return a / length(a);
}

static Vec2 abs(Vec2 a) {
    return vec2(fabs(a.x), fabs(a.y));
}

static Vec2 reflect(Vec2 v, Vec2 n) {
    float d = dot(v, n);
    Vec2 r = v -2*d * n;
    return r;
}

#endif // BREAKOUT_BASE_H_
