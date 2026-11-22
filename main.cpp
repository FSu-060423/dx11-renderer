#include <windows.h>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <sstream>
#include <stdexcept>
#include <string>

#include "Profiler.h"
#include "Renderer.h"

namespace {

double ParseFixedTime(PWSTR commandLine) {
    double fixedTime = -1.0;
    std::wistringstream stream(commandLine != nullptr ? commandLine : L"");
    std::wstring token;

    while (stream >> token) {
        if (token == L"--fixed-time") {
            double value = -1.0;
            if (stream >> value && value >= 0.0) {
                fixedTime = value;
            }
        } else if (token.rfind(L"--fixed-time=", 0) == 0) {
            try {
                const double value = std::stod(token.substr(13));
                if (value >= 0.0) {
                    fixedTime = value;
                }
            } catch (const std::exception&) {
                // Ignore malformed --fixed-time= values.
            }
        }
    }

    return fixedTime;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam,
                            LPARAM lParam) {
    switch (message) {
    case WM_ERASEBKGND:
        return 1;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

} // namespace

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR commandLine,
                    int nCmdShow) {
    const double fixedTime = ParseFixedTime(commandLine);
    const wchar_t* className = L"DX11RendererWindowClass";

    WNDCLASSEX windowClass = {};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground =
        reinterpret_cast<HBRUSH>(static_cast<INT_PTR>(COLOR_WINDOW) + 1);
    windowClass.lpszClassName = className;

    if (RegisterClassEx(&windowClass) == 0) {
        MessageBoxA(nullptr, "RegisterClassEx failed.", "DX11 Renderer",
                    MB_OK | MB_ICONERROR);
        return 1;
    }

    const DWORD windowStyle = WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME &
                              ~WS_MAXIMIZEBOX;

    RECT windowRect = { 0, 0, 1280, 720 };
    AdjustWindowRect(&windowRect, windowStyle, FALSE);

    HWND hwnd = CreateWindowEx(
        0, className, L"DX11 PBR Basketball Court", windowStyle,
        CW_USEDEFAULT, CW_USEDEFAULT, windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top, nullptr, nullptr, hInstance,
        nullptr);

    if (hwnd == nullptr) {
        MessageBoxA(nullptr, "CreateWindowEx failed.", "DX11 Renderer",
                    MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    try {
        Renderer renderer;
        renderer.Initialize(hwnd, 1280, 720);

        Profiler profiler;
        const auto startTime = std::chrono::steady_clock::now();
        MSG message = {};

        while (message.message != WM_QUIT) {
            if (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE) != 0) {
                TranslateMessage(&message);
                DispatchMessage(&message);
            } else {
                const auto now = std::chrono::steady_clock::now();
                const float elapsedSeconds =
                    fixedTime >= 0.0
                        ? static_cast<float>(fixedTime)
                        : std::chrono::duration<float>(now - startTime).count();

                profiler.BeginFrame();
                renderer.Render(elapsedSeconds, profiler);
                profiler.EndFrame();
                renderer.Present();

                const std::uint64_t frameNumber = profiler.FrameCount();
                if (frameNumber == 1 || frameNumber % 10 == 0) {
                    wchar_t title[256] = {};
                    swprintf_s(
                        title, _countof(title),
                        L"DrawCalls: %u | FrameTime: %.2f ms | "
                        L"DrawSubmit: %.3f ms (%.3f us)",
                        profiler.DrawCalls(),
                        profiler.AverageFrameTimeMs(),
                        profiler.DrawSubmitMs(),
                        profiler.DrawSubmitMs() * 1000.0);
                    SetWindowTextW(hwnd, title);
                }
            }
        }
    } catch (const std::exception& error) {
        MessageBoxA(hwnd, error.what(), "DX11 Renderer Error",
                    MB_OK | MB_ICONERROR);
        return 1;
    }

    return 0;
}
