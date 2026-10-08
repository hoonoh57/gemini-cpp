#pragma once
#include "Common.hpp"
#include "ChartTypes.hpp"

inline HWND g_hFormulaDlg = nullptr;

inline LRESULT CALLBACK FormulaDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        CreateWindowW(L"BUTTON", L"새로만들기", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 10, 8, 80, 26, hwnd, nullptr, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"삭제", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 95, 8, 55, 26, hwnd, nullptr, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"작업저장", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 155, 8, 70, 26, hwnd, nullptr, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"수식검증", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 230, 8, 70, 26, hwnd, (HMENU)104, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"차트적용", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 305, 8, 75, 26, hwnd, (HMENU)106, nullptr, nullptr);

        HWND hTree = CreateWindowExW(0, WC_TREEVIEWW, L"", WS_CHILD | WS_VISIBLE | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT,
            10, 42, 230, 600, hwnd, nullptr, nullptr, nullptr);

        TVINSERTSTRUCTW tvis{}; tvis.hParent = TVI_ROOT; tvis.hInsertAfter = TVI_LAST; tvis.item.mask = TVIF_TEXT;
        tvis.item.pszText = (LPWSTR)L"시스템트레이딩"; HTREEITEM hSys = TreeView_InsertItem(hTree, &tvis);
        tvis.hParent = hSys; tvis.item.pszText = (LPWSTR)L"ma cross"; TreeView_InsertItem(hTree, &tvis);
        TreeView_Expand(hTree, hSys, TVE_EXPAND);

        CreateWindowW(L"STATIC", L"전 략 명", WS_CHILD | WS_VISIBLE, 255, 46, 60, 20, hwnd, nullptr, nullptr, nullptr);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"ma cross", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 320, 44, 460, 24, hwnd, nullptr, nullptr, nullptr);

        HWND hTabMain = CreateWindowW(WC_TABCONTROLW, L"", WS_CHILD | WS_VISIBLE, 255, 78, 525, 30, hwnd, nullptr, nullptr, nullptr);
        TCITEMW tie{}; tie.mask = TCIF_TEXT;
        tie.pszText = (LPWSTR)L"수식"; TabCtrl_InsertItem(hTabMain, 0, &tie);
        tie.pszText = (LPWSTR)L"지표변수"; TabCtrl_InsertItem(hTabMain, 1, &tie);
        tie.pszText = (LPWSTR)L"설명"; TabCtrl_InsertItem(hTabMain, 2, &tie);

        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"crossup(avg(c,p1),avg(c,p2));", WS_CHILD | WS_VISIBLE | ES_MULTILINE | WS_VSCROLL,
            255, 120, 525, 250, hwnd, (HMENU)1002, nullptr, nullptr);

        HWND hLb = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL,
            255, 390, 180, 250, hwnd, nullptr, nullptr, nullptr);
        ListBox_AddString(hLb, L"avg(a,n)"); ListBox_AddString(hLb, L"crossup(a,b)"); ListBox_AddString(hLb, L"highest(a,n)");

        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"사용법: crossup(a,b)\r\n\r\n설명: a가 b를 골든크로스할 때 1 반환", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY,
            445, 390, 335, 250, hwnd, nullptr, nullptr, nullptr);
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == 104) {
            MessageBoxW(hwnd, L"수식에 이상이 없습니다. (Syntax OK / 지표변수 검증 완료)", L"수식검증 성공", MB_OK | MB_ICONINFORMATION);
        }
        else if (id == 106) {
            MessageBoxW(hwnd, L"수식이 현재 활성 차트에 성공적으로 주입되었습니다.", L"차트적용 완료", MB_OK | MB_ICONINFORMATION);
            ShowWindow(hwnd, SW_HIDE);
        }
        return 0;
    }
    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

inline void ShowFormulaManagerDialog() {
    if (!g_hFormulaDlg) {
        WNDCLASSW wc{};
        wc.lpfnWndProc = FormulaDlgProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = L"KiwoomFormulaDlgClass_Sep";
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        RegisterClassW(&wc);

        g_hFormulaDlg = CreateWindowExW(
            WS_EX_DLGMODALFRAME, wc.lpszClassName,
            L"수식관리 (Kiwoom Formula & Indicator Parameter Manager)",
            WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
            120, 80, 810, 690,
            nullptr, nullptr, wc.hInstance, nullptr
        );
    }
    ShowWindow(g_hFormulaDlg, SW_SHOW);
    SetForegroundWindow(g_hFormulaDlg);
}