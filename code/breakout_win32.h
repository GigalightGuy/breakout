#ifndef BREAKOUT_WIN32_H_
#define BREAKOUT_WIN32_H_

#include "audio.h"
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <Windows.h>
#include <wingdi.h>
#include <winuser.h>

struct win32_RenderBackBuffer {
    Bitmap bitmap;
    BITMAPINFO info;
};
static win32_RenderBackBuffer g_backBuffer;

struct win32_Window {
    HWND handle;
    u32 width;
    u32 height;
};
static win32_Window g_window;

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

static b32 audioTrackIsPlaying    = false;
static u32 audioTrackCurrentFrame = 0;

LRESULT WINAPI
win32_windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP: {
        WORD vkCode    = LOWORD(wParam);
        WORD keyFlags  = HIWORD(lParam);
        bool isPressed = !((keyFlags & KF_UP) == KF_UP);
        if (vkCode == VK_LEFT) {
            playerInput.left = isPressed;
        } else if (vkCode == VK_RIGHT) {
            playerInput.right = isPressed;
        } else if (vkCode == 'A') {
            playerInput.a = isPressed;
        } else if (vkCode == 'D') {
            playerInput.d = isPressed;
        } else if (vkCode == 'K' && isPressed) {
            audioTrackIsPlaying    = true;
            audioTrackCurrentFrame = 0;
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

        GetClientRect(g_window.handle, &rect);
        g_window.width  = rect.right;
        g_window.height = rect.bottom;
        win32_createBackBuffer(g_window.width, g_window.height);

        ShowWindow(g_window.handle, SW_SHOWDEFAULT);
        UpdateWindow(g_window.handle);
    }

    Arena backingMem = {};
    backingMem.capacity = (usize)MB(16);
    backingMem.memory   = (u8*)VirtualAlloc(NULL, backingMem.capacity, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    ASSERT(backingMem.memory != NULL);
    Arena permanentMem = {};
    permanentMem.capacity = (usize)MB(8);
    permanentMem.memory   = (u8*)allocate(&backingMem, permanentMem.capacity);
    ASSERT(permanentMem.memory != NULL);
    Arena tempMem = {};
    tempMem.capacity = (usize)MB(4);
    tempMem.memory   = (u8*)allocate(&backingMem, tempMem.capacity);
    ASSERT(tempMem.memory != NULL);

    Arena audioMem = {};
    audioMem.capacity = (usize)MB(4);
    audioMem.memory   = (u8*)allocate(&permanentMem, audioMem.capacity);
    ASSERT(audioMem.memory != NULL);

    AudioContext* audioCtx = audioInit(&audioMem, &tempMem);

    // TODO(pedro s.): Create tempMem frame and reset it after call
    AudioTrack* woohAudio = readWaveFile(&audioMem, &tempMem, "data/sounds/wooh.wav");
    (void)woohAudio;


    float submitAheadSeconds = 0.066f;

    gameInit();

    float currentTime = 0;
    i64 startTimeStamp;
    i64 frequency;
    i64 timeStamp;
    QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
    QueryPerformanceCounter((LARGE_INTEGER*)&startTimeStamp);
    timeStamp = startTimeStamp;

    while (g_running) {
        i64 lastTimeStamp = timeStamp;
        QueryPerformanceCounter((LARGE_INTEGER*)&timeStamp);
        currentTime = (float)(timeStamp - startTimeStamp) / frequency;
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

        #if 1
        if (audioCtx->playBackTime <= currentTime + submitAheadSeconds) {
            audioCtx->submittedFrameCount = 0;
            memset(audioCtx->audioMixToSubmit, 0, 2*2*audioCtx->submitAheadFrameCount);
            if (audioCtx->playBackTime < currentTime) {
                audioCtx->playBackTime = currentTime;
            }

            if (audioTrackIsPlaying) {
                u32 frameCount = audioCtx->submitAheadFrameCount;
                u32 remainingFramesInTrack = woohAudio->frameCount - audioTrackCurrentFrame;
                if (frameCount > remainingFramesInTrack) {
                    frameCount = remainingFramesInTrack;
                    audioTrackIsPlaying = false;
                }
                for (u32 i = 0; i < frameCount; i++) {
                    audioCtx->audioMixToSubmit[2*i]     = woohAudio->sampledData[2*audioTrackCurrentFrame];
                    audioCtx->audioMixToSubmit[2*i + 1] = woohAudio->sampledData[2*audioTrackCurrentFrame + 1];
                    audioTrackCurrentFrame++;
                }
            }

        }
        #else
        if (audioCtx->playBackTime <= currentTime + submitAheadSeconds) {
            audioCtx->submittedFrameCount = 0;
            if (audioCtx->playBackTime < currentTime) {
                audioCtx->playBackTime = currentTime;
            }
        }
        #endif
        fillAudioBuffer(audioCtx);

        gameUpdate(deltaSeconds);
        render();
        win32_blitToWindow();
    }

    free(g_backBuffer.bitmap.data);

    audioDeinit(audioCtx);

    return 0;
}

#endif // BREAKOUT_WIN32_H_
