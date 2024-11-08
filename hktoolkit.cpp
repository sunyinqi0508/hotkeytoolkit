#include <windows.h>
#include <cstdio>

/*************** CALLBACK FUNCTIONS BEGIN *****************/
void ToggleTitleBar(HWND hWnd)
{
    LONG style = GetWindowLong(hWnd, GWL_STYLE);
    LONG exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
    RECT rect;
    GetWindowRect(hWnd, &rect);
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfo(MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST), &mi);

    UINT dpi = GetDpiForWindow(hWnd);
    float dpiScale = dpi / 96.0f;
    if (style & WS_CAPTION) {
        style &= ~(WS_CAPTION | WS_SYSMENU);
        exStyle &= ~WS_EX_DLGMODALFRAME;
        SetWindowPos(hWnd, NULL,
            rect.left,
            int(mi.rcWork.top - 30 * dpiScale),
            rect.right - rect.left,
            int(mi.rcWork.bottom - mi.rcWork.top + 30 * dpiScale),
            SWP_NOZORDER | SWP_NOACTIVATE);
    }
    else {
        style |= (WS_CAPTION | WS_SYSMENU);
        exStyle |= WS_EX_DLGMODALFRAME;
        SetWindowPos(hWnd, NULL,
            rect.left,
            mi.rcWork.top,
            rect.right - rect.left,
            mi.rcWork.bottom - mi.rcWork.top,
            SWP_NOZORDER | SWP_NOACTIVATE);
    }
    SetWindowLong(hWnd, GWL_STYLE, style);
    SetWindowLong(hWnd, GWL_EXSTYLE, exStyle);

    SetWindowPos(hWnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    UpdateWindow(hWnd);
}

void ActivateMoveMode(HWND hWnd) {
    if (IsWindow(hWnd)) {
        auto result = SetForegroundWindow(hWnd);
        result = PostMessage(hWnd, WM_SYSCOMMAND, SC_MOVE , HTCAPTION);
        printf("%d", result);
    }
    else {
        MessageBox(NULL, L"Invalid window handle", L"Error", MB_OK | MB_ICONERROR);
    }
}

void Quit(HWND) {
    PostQuitMessage(0);
}

enum class ResizeWindowSide {
    LEFT, TOP, RIGHT, BOTTOM, 
};
struct Context {
    bool m_resizeWindowDirection = true; 
} m_context;

template<bool dir> 
void ResizeWindowDirection(HWND) {
    m_context.m_resizeWindowDirection = dir;
}

template<ResizeWindowSide d, bool turbo>
void ResizeWindow(HWND hWnd) {
    LONG style = GetWindowLong(hWnd, GWL_STYLE);
    LONG exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
    RECT rect;
    GetWindowRect(hWnd, &rect);
    UINT dpi = GetDpiForWindow(hWnd);
    float dpiScale = dpi / 96.0f;
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfo(MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST), &mi);

    int quantity = static_cast<int>((turbo ? 15 : 5) * dpiScale);
    if (!m_context.m_resizeWindowDirection) quantity = -quantity;
    if constexpr (d == ResizeWindowSide::LEFT) rect.left -= quantity;
    else if constexpr (d == ResizeWindowSide::TOP) rect.top -= quantity;
    else if constexpr (d == ResizeWindowSide::RIGHT) rect.right += quantity;
    else rect.bottom += quantity;

    if (rect.left < 0) rect.left = 0;
    else if (rect.top < 0) rect.top = 0;
    else if (rect.right > mi.rcMonitor.right) rect.right = mi.rcMonitor.right;
    else if (rect.left > mi.rcMonitor.left) rect.left = mi.rcMonitor.left;

    SetWindowPos(hWnd, NULL,
        rect.left,
        rect.top,
        rect.right - rect.left,
        rect.bottom - rect.top,
        SWP_NOZORDER | SWP_NOACTIVATE);
}
/*************** CALLBACK FUNCTIONS END *****************/

struct HotKeys {
    unsigned char id;
    unsigned short m;
    unsigned char k;
    void (*call)(HWND);
    HotKeys(unsigned short m, unsigned char k, void(*call)(HWND)) : 
        m(m), k(k), call(call) {
        static int cnt = 1; 
        id = cnt++;
    }
} hotkeys[] = { // Register hotkeys here
    {MOD_CONTROL | MOD_SHIFT, 'M', ActivateMoveMode},
    {MOD_CONTROL | MOD_SHIFT, 'H', ToggleTitleBar}, 
    {MOD_CONTROL | MOD_ALT, 'A', ResizeWindow<ResizeWindowSide::LEFT, false>},
    {MOD_CONTROL | MOD_ALT, 'S', ResizeWindow<ResizeWindowSide::BOTTOM, false>},
    {MOD_CONTROL | MOD_ALT, 'D', ResizeWindow<ResizeWindowSide::RIGHT, false>},
    {MOD_CONTROL | MOD_ALT, 'W', ResizeWindow<ResizeWindowSide::TOP, false>},
    {MOD_CONTROL | MOD_SHIFT | MOD_ALT, 'A', ResizeWindow<ResizeWindowSide::LEFT, true>},
    {MOD_CONTROL | MOD_SHIFT | MOD_ALT, 'S', ResizeWindow<ResizeWindowSide::BOTTOM, true>},
    {MOD_CONTROL | MOD_SHIFT | MOD_ALT, 'D', ResizeWindow<ResizeWindowSide::RIGHT, true>},
    {MOD_CONTROL | MOD_SHIFT | MOD_ALT, 'W', ResizeWindow<ResizeWindowSide::TOP, true>},
    {MOD_CONTROL | MOD_SHIFT | MOD_ALT, VK_OEM_PLUS, ResizeWindowDirection<true>},
    {MOD_CONTROL | MOD_SHIFT | MOD_ALT, VK_OEM_MINUS, ResizeWindowDirection<false>},
    {MOD_CONTROL | MOD_SHIFT | MOD_ALT, 'Q', Quit},
};
constexpr int n_hotkeys = sizeof(hotkeys) / sizeof(HotKeys);
static_assert(n_hotkeys < 256, "More than 255 hotkeys registered.");

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_HOTKEY: {
        if (wParam > 0 && wParam <= n_hotkeys) {
            auto& hk = hotkeys[wParam - 1];
            HWND activeWindow = GetForegroundWindow();
            hk.call(activeWindow);
        }
        break;
    }
    case WM_DESTROY: {
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

int APIENTRY wWinMain(_In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_     LPWSTR    lpCmdLine,
    _In_     int       nCmdShow) {
    const wchar_t CLASS_NAME[] = L"HKToolkitClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hWnd = CreateWindowEx(0, CLASS_NAME, L"Hotkey Toolkit", 0, 0, 0, 0, 0,
        NULL, NULL, hInstance, NULL);

    if (!hWnd) return 0;

    for (auto& hk : hotkeys) {
        if (!RegisterHotKey(hWnd, hk.id, hk.m, hk.k)) {
            DWORD error = GetLastError();
            wchar_t errorMsg[64] = { 0 };
            swprintf(errorMsg, 63, L"Key: %d, Code: %d.", hk.id, error);
            MessageBox(NULL, errorMsg, L"Error Registering Hotkey", MB_OK | MB_ICONERROR);
            return 1;
        }
    }

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    for (auto& hk : hotkeys) 
        UnregisterHotKey(hWnd, hk.id);

    return 0;
}
