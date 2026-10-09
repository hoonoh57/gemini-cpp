#include "Common.hpp"
#include "ChartCoreRenderer.hpp"

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        g_Engine.Init(hwnd);
        return 0;
    case WM_PAINT:
        g_Engine.Render();
        ValidateRect(hwnd, nullptr);
        return 0;
    case WM_MOUSEMOVE:
        g_Engine.OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_MOUSELEAVE:
        g_Engine.is_mouse_inside = false;
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_MOUSEWHEEL:
        g_Engine.OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
        return 0;
    case WM_LBUTTONDOWN:
        g_Engine.OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_LBUTTONUP:
        if (g_Engine.is_dragging) {
            g_Engine.is_dragging = false;
            ReleaseCapture();
        }
        return 0;
    case WM_LBUTTONDBLCLK:
        g_Engine.OnLButtonDblClk(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_RBUTTONUP:
        g_Engine.OnRButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_SIZE:
        if (g_Engine.pRT) {
            RECT rc; GetClientRect(hwnd, &rc);
            g_Engine.pRT->Resize(D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top));
        }
        return 0;
        case WM_DESTROY:
        g_Engine.SaveLayoutConfig();
        CentralDataManager::Instance().StopRealTimeListener();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nCmdShow) {
    INITCOMMONCONTROLSEX icex{ sizeof(icex), ICC_STANDARD_CLASSES | ICC_TREEVIEW_CLASSES | ICC_TAB_CLASSES };
    InitCommonControlsEx(&icex);

    WNDCLASSW wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"KiwoomModularHostWnd";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.style = CS_DBLCLKS;
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0, wc.lpszClassName,
        L"[0607] 키움 종합 멀티차트 (모듈 분리 완성본: 크로스헤어/줌/수식관리/조건설정)",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 800,
        nullptr, nullptr, hInst, nullptr
    );

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}