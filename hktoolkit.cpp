#include <windows.h>
#include <cstdio>

/*************** CALLBACK FUNCTIONS BEGIN *****************/
void ToggleTitleBar(HWND hWnd)
{
    LONG style = GetWindowLong(hWnd, GWL_STYLE);
    if (style & WS_CAPTION)
        style &= ~WS_CAPTION;
    else
        style |= WS_CAPTION;
    SetWindowLong(hWnd, GWL_STYLE, style);
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
/*************** CALLBACK FUNCTIONS END *****************/

struct HotKeys {
    char id;
    unsigned short m;
    unsigned char k;
    void (*call)(HWND);
} hotkeys[] = { // Register hotkeys here
    {1, MOD_CONTROL | MOD_SHIFT, 'M', ActivateMoveMode},
    {2, MOD_CONTROL | MOD_SHIFT, 'H', ToggleTitleBar}
};
constexpr int n_hotkeys = sizeof(hotkeys) / sizeof(HotKeys);

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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
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
