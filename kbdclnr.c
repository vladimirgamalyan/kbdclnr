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
#define COBJMACROS
#include <windows.h>
#include <wincodec.h>
#include <shlwapi.h>

#include "resource.h"

static HHOOK g_hook = NULL;
static HBITMAP g_image = NULL;

static LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
        return 1; /* swallow every keyboard event */
    return CallNextHookEx(g_hook, nCode, wParam, lParam);
}

/* Decode the PNG stored as an RCDATA resource into a top-down 32-bit DIB. */
static HBITMAP LoadImageResource(HINSTANCE hInstance, int id)
{
    HRSRC res = FindResourceW(hInstance, MAKEINTRESOURCEW(id), RT_RCDATA);
    if (!res)
        return NULL;

    HGLOBAL data = LoadResource(hInstance, res);
    if (!data)
        return NULL;

    IStream *stream = SHCreateMemStream((const BYTE *)LockResource(data),
                                        SizeofResource(hInstance, res));
    if (!stream)
        return NULL;

    IWICImagingFactory *factory = NULL;
    IWICBitmapDecoder *decoder = NULL;
    IWICBitmapFrameDecode *frame = NULL;
    IWICFormatConverter *converter = NULL;
    HBITMAP bitmap = NULL;

    if (SUCCEEDED(CoCreateInstance(&CLSID_WICImagingFactory, NULL,
                                   CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory,
                                   (void **)&factory)) &&
        SUCCEEDED(IWICImagingFactory_CreateDecoderFromStream(
            factory, stream, NULL, WICDecodeMetadataCacheOnLoad, &decoder)) &&
        SUCCEEDED(IWICBitmapDecoder_GetFrame(decoder, 0, &frame)) &&
        SUCCEEDED(IWICImagingFactory_CreateFormatConverter(factory, &converter)) &&
        SUCCEEDED(IWICFormatConverter_Initialize(
            converter, (IWICBitmapSource *)frame, &GUID_WICPixelFormat32bppBGR,
            WICBitmapDitherTypeNone, NULL, 0.0, WICBitmapPaletteTypeCustom))) {

        UINT w = 0, h = 0;
        IWICFormatConverter_GetSize(converter, &w, &h);

        BITMAPINFO bi = {0};
        bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
        bi.bmiHeader.biWidth = (LONG)w;
        bi.bmiHeader.biHeight = -(LONG)h; /* negative height = top-down rows */
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;

        void *pixels = NULL;
        bitmap = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, &pixels, NULL, 0);
        if (bitmap && FAILED(IWICFormatConverter_CopyPixels(
                          converter, NULL, w * 4, w * h * 4, (BYTE *)pixels))) {
            DeleteObject(bitmap);
            bitmap = NULL;
        }
    }

    if (converter) IWICFormatConverter_Release(converter);
    if (frame) IWICBitmapFrameDecode_Release(frame);
    if (decoder) IWICBitmapDecoder_Release(decoder);
    if (factory) IWICImagingFactory_Release(factory);
    IStream_Release(stream);
    return bitmap;
}

static void PaintImage(HWND hwnd, HDC hdc)
{
    RECT rc;
    GetClientRect(hwnd, &rc);

    if (!g_image) {
        FillRect(hdc, &rc, GetSysColorBrush(COLOR_WINDOW));
        return;
    }

    BITMAP bm;
    GetObject(g_image, sizeof(bm), &bm);

    HDC mem = CreateCompatibleDC(hdc);
    HGDIOBJ old = SelectObject(mem, g_image);
    SetStretchBltMode(hdc, HALFTONE);
    StretchBlt(hdc, 0, 0, rc.right, rc.bottom,
               mem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
    SelectObject(mem, old);
    DeleteDC(mem);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        PaintImage(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1; /* the image covers the whole client area */
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
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

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

    g_image = LoadImageResource(hInstance, IDR_WINDOW_IMAGE);

    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"kbdclnrWnd";
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
    /* Load the title-bar icon at its own size so it isn't a scaled-down 32×32. */
    wc.hIconSm = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON),
                                   IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                                   GetSystemMetrics(SM_CYSMICON), 0);
    RegisterClassExW(&wc);

    /* Size the window so that its client area is exactly the image size. */
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
    RECT wr = {0, 0, S(480), S(240)};
    AdjustWindowRect(&wr, style, FALSE);
    int winW = wr.right - wr.left, winH = wr.bottom - wr.top;

    int scrW = GetSystemMetrics(SM_CXSCREEN);
    int scrH = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST, L"kbdclnrWnd",
        L"kbdclnr — keyboard locked",
        style,
        (scrW - winW) / 2, (scrH - winH) / 2, winW, winH,
        NULL, NULL, hInstance, NULL);
    if (!hwnd) {
        UnhookWindowsHookEx(g_hook);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    UnhookWindowsHookEx(g_hook);
    g_hook = NULL;
    if (g_image) DeleteObject(g_image);
    CoUninitialize();
    return 0;
}
