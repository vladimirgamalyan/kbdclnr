/*
 * kbdclnr — keyboard lock for cleaning.
 *
 * On start it installs a low-level WH_KEYBOARD_LL hook and swallows all
 * key events. The mouse keeps working. Closing the window removes the
 * hook and the keyboard works again. Ctrl+Alt+Del cannot be intercepted
 * (the Secure Attention Sequence bypasses hooks) — that is a Windows
 * limitation.
 */

#define UNICODE
#define _UNICODE
#include <windows.h>

#define IDC_BTN_UNLOCK 1001

static HHOOK g_hook = NULL;
static HFONT g_fontText = NULL;
static HFONT g_fontTitle = NULL;

static LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
        return 1; /* swallow every keyboard event */
    return CallNextHookEx(g_hook, nCode, wParam, lParam);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_BTN_UNLOCK) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_CTLCOLORSTATIC:
        SetBkMode((HDC)wParam, TRANSPARENT);
        return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow)
{
    (void)hPrev; (void)lpCmdLine;

    SetProcessDPIAware();

    HDC screen = GetDC(NULL);
    int dpi = GetDeviceCaps(screen, LOGPIXELSX);
    ReleaseDC(NULL, screen);
    double scale = dpi / 96.0;
#define S(x) ((int)((x) * scale + 0.5))

    g_hook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardHookProc, hInstance, 0);
    if (!g_hook) {
        MessageBoxW(NULL, L"Failed to install the keyboard hook.",
                    L"kbdclnr", MB_ICONERROR);
        return 1;
    }

    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"kbdclnrWnd";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassW(&wc);

    int winW = S(480), winH = S(240);
    int scrW = GetSystemMetrics(SM_CXSCREEN);
    int scrH = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST, L"kbdclnrWnd",
        L"kbdclnr — keyboard locked",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        (scrW - winW) / 2, (scrH - winH) / 2, winW, winH,
        NULL, NULL, hInstance, NULL);
    if (!hwnd) {
        UnhookWindowsHookEx(g_hook);
        return 1;
    }

    g_fontTitle = CreateFontW(S(24), 0, 0, 0, FW_SEMIBOLD, 0, 0, 0,
                              DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0,
                              L"Segoe UI");
    g_fontText = CreateFontW(S(16), 0, 0, 0, FW_NORMAL, 0, 0, 0,
                             DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0,
                             L"Segoe UI");

    RECT rc;
    GetClientRect(hwnd, &rc);
    int cw = rc.right - rc.left;

    HWND title = CreateWindowExW(0, L"STATIC",
        L"Keyboard is locked",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        S(10), S(20), cw - S(20), S(32), hwnd, NULL, hInstance, NULL);
    SendMessageW(title, WM_SETFONT, (WPARAM)g_fontTitle, TRUE);

    HWND text = CreateWindowExW(0, L"STATIC",
        L"You can clean the keyboard now — key presses are disabled.\n"
        L"The mouse works as usual.",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        S(10), S(64), cw - S(20), S(48), hwnd, NULL, hInstance, NULL);
    SendMessageW(text, WM_SETFONT, (WPARAM)g_fontText, TRUE);

    int btnW = S(240), btnH = S(36);
    HWND btn = CreateWindowExW(0, L"BUTTON",
        L"Unlock and exit",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        (cw - btnW) / 2, S(130), btnW, btnH,
        hwnd, (HMENU)(INT_PTR)IDC_BTN_UNLOCK, hInstance, NULL);
    SendMessageW(btn, WM_SETFONT, (WPARAM)g_fontText, TRUE);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    UnhookWindowsHookEx(g_hook);
    g_hook = NULL;
    if (g_fontTitle) DeleteObject(g_fontTitle);
    if (g_fontText) DeleteObject(g_fontText);
    return 0;
}
