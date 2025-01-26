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


#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <Windows.h>
#include <wingdi.h>
#include <winuser.h>

#define LOG(...) fprintf(stderr, __VA_ARGS__)

#ifdef _WIN32
# define DEBUGBREAK() __debugbreak()
#else
# error Unknown platform
#endif // _WIN32

#define ASSERT(c) do { if (!(c)) { LOG("%s %d: assertion '%s' failed\n", __FILE__, __LINE__, #c); DEBUGBREAK(); } } while (0)

#define WIDTH  1080
#define HEIGHT 720

static bool g_running = true;

struct Bitmap {
    u32* data;
    u32  width;
    u32  height;
};

struct RenderBackBuffer {
    Bitmap bitmap;
    BITMAPINFO info;
};
static RenderBackBuffer g_backBuffer;

struct Window {
    HWND handle;
    HDC deviceContext;
    u32 width;
    u32 height;
};
static Window g_window;

static void win32_resizeBackBuffer(u32 width, u32 height) {
    if ((width == 0 || height == 0) ||
        (width == g_backBuffer.bitmap.width && height == g_backBuffer.bitmap.height)) {
        return;
    }

    if (g_backBuffer.bitmap.data) {
        free(g_backBuffer.bitmap.data);
    }
    g_backBuffer.bitmap.width  = width;
    g_backBuffer.bitmap.height = height;
    g_backBuffer.info.bmiHeader.biWidth  = width;
    g_backBuffer.info.bmiHeader.biHeight = -height;
    g_backBuffer.bitmap.data = (u32*)malloc(sizeof(u32) * width*height);
}

static void win32_resizeWindow(u32 width, u32 height) {
    g_window.width  = width;
    g_window.height = height;
    win32_resizeBackBuffer(width, height);
}

static void win32_createBackBuffer(u32 width, u32 height) {
    g_backBuffer = {};
    g_backBuffer.info.bmiHeader.biSize = sizeof(g_backBuffer.info.bmiHeader);
    g_backBuffer.info.bmiHeader.biPlanes = 1;
    g_backBuffer.info.bmiHeader.biBitCount = 32;
    g_backBuffer.info.bmiHeader.biCompression = BI_RGB;

    win32_resizeBackBuffer(width, height);
}

struct PlayerInput {
    bool left;
    bool right;
    bool a;
    bool d;
};
static PlayerInput playerInput;

LRESULT WINAPI
win32_windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP: {
        WORD vkCode = LOWORD(wParam);
        WORD keyFlags = HIWORD(lParam);
        bool isPressed = !((keyFlags & KF_UP) == KF_UP);
        if (vkCode == VK_LEFT) {
            playerInput.left = isPressed;
        } else if (vkCode == VK_RIGHT) {
            playerInput.right = isPressed;
        } else if (vkCode == 'A') {
            playerInput.a = isPressed;
        } else if (vkCode == 'D') {
            playerInput.d = isPressed;
        }
    } break;
    case WM_SIZE: {
        u32 width  = LOWORD(lParam);
        u32 height = HIWORD(lParam);
        win32_resizeWindow(width, height);
    } break;
    case WM_DESTROY: {
        g_running = false;
    } break;
    default:
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

static void win32_blitToWindow() {
    HDC deviceContext = GetDC(g_window.handle);
    StretchDIBits(deviceContext, 
                  0, 0, g_window.width, g_window.height, 
                  0, 0, g_backBuffer.bitmap.width, g_backBuffer.bitmap.height,
                  g_backBuffer.bitmap.data, &g_backBuffer.info, 
                  DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(g_window.handle, deviceContext);
}


static void drawSquare(u32 color, Vec2 center, Vec2 halfSize, Bitmap* bitmap) {
    int minX = (int)(center.x - halfSize.x);
    int minY = (int)(center.y - halfSize.y);
    int maxX = (int)(center.x + halfSize.x) + 1;
    int maxY = (int)(center.y + halfSize.y) + 1;

    if (minX >= bitmap->width || minY >= bitmap->height ||
        maxX <= 0 || maxY <= 0) {
        return;
    }

    minX = minX >= 0 ? minX : 0;
    minY = minY >= 0 ? minY : 0;
    maxX = maxX <= bitmap->width  ? maxX : bitmap->width;
    maxY = maxY <= bitmap->height ? maxY : bitmap->height;

    for (int y = minY; y < maxY; y++) {
        u32* p = &bitmap->data[y * bitmap->width + minX];
        for (int x = minX; x < maxX; x++) {
            *p++ = color;
        }
    }
}

static void drawCircle(u32 color, Vec2 center, float radius, Bitmap* bitmap) {
    int minX = (int)(center.x - radius);
    int minY = (int)(center.y - radius);
    int maxX = (int)(center.x + radius) + 1;
    int maxY = (int)(center.y + radius) + 1;

    if (minX >= bitmap->width || minY >= bitmap->height ||
        maxX < 0 || maxY < 0) {
        return;
    }

    minX = minX >= 0 ? minX : 0;
    minY = minY >= 0 ? minY : 0;
    maxX = maxX <= bitmap->width  ? maxX : bitmap->width;
    maxY = maxY <= bitmap->height ? maxY : bitmap->height;

    for (int y = minY; y < maxY; y++) {
        u32* p = &bitmap->data[y * bitmap->width + minX];
        for (int x = minX; x < maxX; x++) {
            Vec2 d = vec2(x,y) - center;
            float distanceSquared = dot(d,d);
            if (distanceSquared <= radius*radius) {
                *p = color;
            }
            p++;
        }
    }
}

struct Box {
    Vec2 center;
    Vec2 halfExtents;
};

struct Circle {
    Vec2  center;
    float radius;
};

struct Ball {
    Circle circle;
    Vec2   velocity;
    bool   alive;
};

#define MAX_TILES 128
static Box tiles[MAX_TILES];
static int aliveTiles = 0;

static Box  player;
#define BALL_SPEED 300.0f
static Ball ball;

static bool startedRound = false;

static void render() {

    { // clear backbuffer to black
        u32* p = g_backBuffer.bitmap.data;
        for (int y = 0; y < g_backBuffer.bitmap.height; y++) {
            for (int x = 0; x < g_backBuffer.bitmap.width; x++) {
                *p++ = 0xff000000;
            }
        }
    }

    for (int i = 0; i < aliveTiles; i++) {
        drawSquare(0xffff0000, tiles[i].center, tiles[i].halfExtents, &g_backBuffer.bitmap);
    }

    drawCircle(0xff00ff00, ball.circle.center, ball.circle.radius, &g_backBuffer.bitmap);
    drawSquare(0xff00ffff, player.center, player.halfExtents, &g_backBuffer.bitmap);
}

static void resetPlayer() {
    player = {
        .center      = vec2(540, 600),
        .halfExtents = vec2(65, 10),
    };
}

static void resetBall() {
    ball = {
        .circle = {
            .center = vec2(540, 300),
            .radius = 10,
        },
        .velocity = vec2(0,0),
    };
}

static void makeTileGrid() {
    int gridWidth  = 10;
    int gridHeight = 4;
    aliveTiles = gridWidth * gridHeight;
    ASSERT(aliveTiles <= MAX_TILES);

    float padding           = 20;
    float horizontalSpacing = 5;
    float verticalSpacing   = 5;
    Vec2 halfExtents = vec2(
        (WIDTH - padding*2 - horizontalSpacing * (gridWidth-1)) / gridWidth * 0.5f,
        10.0f
    );
    
    for (int y = 0; y < gridHeight; y++) {
        Vec2 offset = vec2(
            padding + halfExtents.x,
            padding + halfExtents.y + y * (halfExtents.y * 2 + verticalSpacing)
        );
        for (int x = 0; x < gridWidth; x++) {
            tiles[y * gridWidth + x] = (Box){
                .center      = offset,
                .halfExtents = halfExtents,
            };
            offset.x += halfExtents.x * 2 + horizontalSpacing;
        }
    }
}

static bool areColliding(Box* box, Circle* circle) {
    Vec2 d = circle->center - box->center;
    d = abs(d) - vec2(circle->radius);
    return (d.x <= box->halfExtents.x) &&
           (d.y <= box->halfExtents.y);
}

static void update(float deltaSeconds) {
    if (!startedRound) {
        if (playerInput.left || playerInput.right || playerInput.a || playerInput.d) {
            startedRound = true;

            ball.velocity = vec2(0, BALL_SPEED);
        } else {
            return;
        }
    }

    float playerSpeedX = 0;
    if (playerInput.left || playerInput.a) {
        playerSpeedX -= 300;
    }
    if (playerInput.right || playerInput.d) {
        playerSpeedX += 300;
    }

    player.center.x += playerSpeedX * deltaSeconds;
    if (player.center.x < 0) {
        player.center.x = 0;
    } else if (player.center.x > g_window.width) {
        player.center.x = g_window.width;
    }

    ball.circle.center += deltaSeconds * ball.velocity;

    // TODO(pedro s.): Correct position on collision
    if (areColliding(&player, &ball.circle)) {
        Vec2 dir = (ball.circle.center - player.center);
        dir.x *= 0.5f;
        dir = normalize(dir);
        ball.velocity = BALL_SPEED * dir;
    }

    // TODO(pedro s.): Correct position on collision
    // TODO(pedro s.): Get collision normals to reflect ball
    for (int i = 0; i < aliveTiles; i++) {
        if (areColliding(&tiles[i], &ball.circle)) {
            aliveTiles--;
            tiles[i] = tiles[aliveTiles];
            ball.velocity.y = -ball.velocity.y;
            break;
        }
    }

    if (ball.circle.center.y - ball.circle.radius <= 0) {
        ball.velocity.y = fabs(ball.velocity.y);
    }
    if (ball.circle.center.x - ball.circle.radius <= 0) {
        ball.velocity.x = fabs(ball.velocity.x);
    } else if (ball.circle.center.x + ball.circle.radius >= g_window.width) {
        ball.velocity.x = -fabs(ball.velocity.x);
    }

    if (ball.circle.center.y + ball.circle.radius >= g_window.height) {
        startedRound = false;
        resetBall();
    }
}

int main() {
    {
        const wchar_t wndClassName[] = L"WndClassName";
        WNDCLASSEXW wndClass = {};
        wndClass.cbSize        = sizeof(WNDCLASSEXW);
        wndClass.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wndClass.lpfnWndProc   = win32_windowProc;
        wndClass.hInstance     = GetModuleHandle(NULL);
        wndClass.lpszClassName = wndClassName;
        ATOM wndClassAtom = RegisterClassExW(&wndClass);
        ASSERT(wndClassAtom != 0);

        const wchar_t title[] = L"Breakout";
        DWORD style = WS_OVERLAPPEDWINDOW;
        int width  = WIDTH;
        int height = HEIGHT;
        RECT rect = {};
        rect.right  = width;
        rect.bottom = height;
        AdjustWindowRectEx(&rect, style, 0, 0);
        width  = rect.right - rect.left;
        height = rect.bottom - rect.top;
        g_window.handle = CreateWindowExW(0, wndClassName, title, style, 
                                          CW_USEDEFAULT, CW_USEDEFAULT,
                                          width, height, NULL, NULL, 
                                          GetModuleHandle(NULL), NULL);
        ASSERT(g_window.handle != NULL);
        g_window.deviceContext = GetWindowDC(g_window.handle);
        ASSERT(g_window.deviceContext != NULL);

        GetClientRect(g_window.handle, &rect);
        g_window.width  = rect.right;
        g_window.height = rect.bottom;
        win32_createBackBuffer(g_window.width, g_window.height);

        ShowWindow(g_window.handle, SW_SHOWDEFAULT);
        UpdateWindow(g_window.handle);
    }

    resetPlayer();
    resetBall();
    makeTileGrid();

    i64 frequency;
    i64 timeStamp;
    QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
    QueryPerformanceCounter((LARGE_INTEGER*)&timeStamp);

    while (g_running) {
        i64 lastTimeStamp = timeStamp;
        QueryPerformanceCounter((LARGE_INTEGER*)&timeStamp);
        float deltaSeconds = (float)(timeStamp - lastTimeStamp) / frequency;

        MSG msg;
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) {
                g_running = false;
                break;
            }
        }

        update(deltaSeconds);
        render();
        win32_blitToWindow();
    }

    free(g_backBuffer.bitmap.data);

    return 0;
}
