#pragma once
#include "Common.hpp"
#include "ChartTypes.hpp"

extern HWND g_hFormulaDlg;
void ShowFormulaManagerDialog();

inline StrategyConfig g_ActiveConfig;
inline COLORREF g_CustomColors[16] = { 0 };

inline LRESULT CALLBACK StrategyCondDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"SK하이닉스 (000660)", WS_CHILD | WS_VISIBLE | ES_READONLY, 14, 12, 532, 24, hwnd, nullptr, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L" 포지션 설정 ", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 14, 42, 532, 48, hwnd, nullptr, nullptr, nullptr);
        HWND hR1 = CreateWindowW(L"BUTTON", L"매수/매수청산", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP, 32, 62, 130, 20, hwnd, nullptr, nullptr, nullptr);
        HWND hR2 = CreateWindowW(L"BUTTON", L"매도/매도청산", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 185, 62, 130, 20, hwnd, nullptr, nullptr, nullptr);
        HWND hR3 = CreateWindowW(L"BUTTON", L"모두 허용", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 345, 62, 100, 20, hwnd, nullptr, nullptr, nullptr);
        Button_SetCheck(hR1, BST_CHECKED);

        CreateWindowW(L"BUTTON", L" 신호시 주문처리 ", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 14, 95, 370, 148, hwnd, nullptr, nullptr, nullptr);
        CreateWindowW(L"STATIC", L"주문적용", WS_CHILD | WS_VISIBLE, 26, 118, 65, 18, hwnd, nullptr, nullptr, nullptr);
        HWND hCb1 = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 95, 114, 85, 100, hwnd, nullptr, nullptr, nullptr);
        ComboBox_AddString(hCb1, L"시험적용"); ComboBox_AddString(hCb1, L"실주문"); ComboBox_SetCurSel(hCb1, 0);

        CreateWindowW(L"STATIC", L"주문계좌", WS_CHILD | WS_VISIBLE, 190, 118, 65, 18, hwnd, nullptr, nullptr, nullptr);
        HWND hCb2 = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 255, 114, 118, 120, hwnd, nullptr, nullptr, nullptr);
        ComboBox_AddString(hCb2, L"80123456-11"); ComboBox_SetCurSel(hCb2, 0);

        CreateWindowW(L"STATIC", L"시작신호", WS_CHILD | WS_VISIBLE, 26, 144, 65, 18, hwnd, nullptr, nullptr, nullptr);
        HWND hCb3 = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 95, 140, 85, 100, hwnd, nullptr, nullptr, nullptr);
        ComboBox_AddString(hCb3, L"모든신호"); ComboBox_SetCurSel(hCb3, 0);

        CreateWindowW(L"STATIC", L"주문시점", WS_CHILD | WS_VISIBLE, 190, 144, 65, 18, hwnd, nullptr, nullptr, nullptr);
        HWND hCb4 = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 255, 140, 118, 100, hwnd, nullptr, nullptr, nullptr);
        ComboBox_AddString(hCb4, L"봉 완성시"); ComboBox_SetCurSel(hCb4, 0);

        HWND hRadQty = CreateWindowW(L"BUTTON", L"수량", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP, 26, 170, 50, 20, hwnd, nullptr, nullptr, nullptr);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"10", WS_CHILD | WS_VISIBLE, 85, 169, 95, 22, hwnd, (HMENU)4001, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"금액(원)", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 188, 170, 68, 20, hwnd, nullptr, nullptr, nullptr);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"1,000,000", WS_CHILD | WS_VISIBLE, 258, 169, 115, 22, hwnd, nullptr, nullptr, nullptr);
        Button_SetCheck(hRadQty, BST_CHECKED);

        CreateWindowW(L"STATIC", L"진입가격", WS_CHILD | WS_VISIBLE, 26, 204, 65, 18, hwnd, nullptr, nullptr, nullptr);
        HWND hCb5 = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 95, 200, 85, 100, hwnd, nullptr, nullptr, nullptr);
        ComboBox_AddString(hCb5, L"신호가+2호가"); ComboBox_SetCurSel(hCb5, 0);

        CreateWindowW(L"STATIC", L"청산가격", WS_CHILD | WS_VISIBLE, 190, 204, 65, 18, hwnd, nullptr, nullptr, nullptr);
        HWND hCb6 = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 255, 200, 118, 100, hwnd, nullptr, nullptr, nullptr);
        ComboBox_AddString(hCb6, L"신호가-2호가"); ComboBox_SetCurSel(hCb6, 0);

        CreateWindowW(L"BUTTON", L" 거래 비용 ", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 390, 95, 156, 148, hwnd, nullptr, nullptr, nullptr);
        CreateWindowW(L"STATIC", L"거래수수료", WS_CHILD | WS_VISIBLE, 402, 118, 80, 18, hwnd, nullptr, nullptr, nullptr);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0.015", WS_CHILD | WS_VISIBLE, 402, 138, 85, 22, hwnd, nullptr, nullptr, nullptr);
        CreateWindowW(L"STATIC", L"%", WS_CHILD | WS_VISIBLE, 492, 141, 20, 18, hwnd, nullptr, nullptr, nullptr);

        CreateWindowW(L"STATIC", L"주문편차", WS_CHILD | WS_VISIBLE, 402, 172, 80, 18, hwnd, nullptr, nullptr, nullptr);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0.05", WS_CHILD | WS_VISIBLE, 402, 192, 85, 22, hwnd, nullptr, nullptr, nullptr);
        CreateWindowW(L"STATIC", L"%", WS_CHILD | WS_VISIBLE, 492, 195, 20, 18, hwnd, nullptr, nullptr, nullptr);

        CreateWindowW(L"BUTTON", L" 알람설정 ", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 14, 248, 532, 52, hwnd, nullptr, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"매수", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 28, 268, 45, 20, hwnd, nullptr, nullptr, nullptr);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE, 78, 267, 110, 22, hwnd, nullptr, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"찾기", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 192, 266, 42, 24, hwnd, nullptr, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"▶", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 238, 266, 26, 24, hwnd, nullptr, nullptr, nullptr);

        CreateWindowW(L"BUTTON", L"매도", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 290, 268, 45, 20, hwnd, nullptr, nullptr, nullptr);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE, 340, 267, 110, 22, hwnd, nullptr, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"찾기", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 454, 266, 42, 24, hwnd, nullptr, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"▶", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 500, 266, 26, 24, hwnd, nullptr, nullptr, nullptr);

        // 강제 청산 (설정값 바인딩)
        CreateWindowW(L"BUTTON", L" 강제 청산 ", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 14, 305, 532, 260, hwnd, nullptr, nullptr, nullptr);
        int gy = 328;

        auto create_unit = [&](int x, int y) {
            HWND hCb = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, x, y, 42, 100, hwnd, nullptr, nullptr, nullptr);
            ComboBox_AddString(hCb, L"%"); ComboBox_AddString(hCb, L"pt"); ComboBox_AddString(hCb, L"원"); ComboBox_SetCurSel(hCb, 0);
            return hCb;
        };

        // 1. 최대허용손실
        HWND hChk1 = CreateWindowW(L"BUTTON", L"최대허용손실", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 28, gy, 150, 20, hwnd, (HMENU)4010, nullptr, nullptr);
        Button_SetCheck(hChk1, g_ActiveConfig.chk_stop_loss ? BST_CHECKED : BST_UNCHECKED);
        wchar_t buf[16]; swprintf_s(buf, L"%.1f", g_ActiveConfig.val_stop_loss);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", buf, WS_CHILD | WS_VISIBLE, 225, gy - 2, 55, 22, hwnd, (HMENU)4011, nullptr, nullptr);
        create_unit(285, gy - 2);
        CreateWindowW(L"STATIC", L"표시색상", WS_CHILD | WS_VISIBLE, 360, gy, 60, 18, hwnd, nullptr, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 430, gy - 3, 75, 23, hwnd, (HMENU)3000, nullptr, nullptr);
        gy += 30;

        // 2. 최대수익대비하락
        HWND hChk2 = CreateWindowW(L"BUTTON", L"최대수익대비하락", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 28, gy, 170, 20, hwnd, (HMENU)4020, nullptr, nullptr);
        Button_SetCheck(hChk2, g_ActiveConfig.chk_trailing ? BST_CHECKED : BST_UNCHECKED);
        swprintf_s(buf, L"%.0f", g_ActiveConfig.val_trailing);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", buf, WS_CHILD | WS_VISIBLE, 225, gy - 2, 55, 22, hwnd, (HMENU)4021, nullptr, nullptr);
        create_unit(285, gy - 2);
        CreateWindowW(L"STATIC", L"표시색상", WS_CHILD | WS_VISIBLE, 360, gy, 60, 18, hwnd, nullptr, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 430, gy - 3, 75, 23, hwnd, (HMENU)3001, nullptr, nullptr);
        gy += 25;
        swprintf_s(buf, L"%.1f", g_ActiveConfig.val_trail_start);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", buf, WS_CHILD | WS_VISIBLE, 225, gy - 2, 55, 22, hwnd, (HMENU)4022, nullptr, nullptr);
        create_unit(285, gy - 2);
        CreateWindowW(L"STATIC", L"수익 이후", WS_CHILD | WS_VISIBLE, 335, gy, 70, 18, hwnd, nullptr, nullptr, nullptr);
        gy += 30;

        // 3. 최고/최저가 이익보존율
        HWND hChk3 = CreateWindowW(L"BUTTON", L"최고/최저가 이익보존율", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 28, gy, 195, 20, hwnd, (HMENU)4030, nullptr, nullptr);
        Button_SetCheck(hChk3, g_ActiveConfig.chk_protect ? BST_CHECKED : BST_UNCHECKED);
        swprintf_s(buf, L"%.1f", g_ActiveConfig.val_protect);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", buf, WS_CHILD | WS_VISIBLE, 225, gy - 2, 55, 22, hwnd, (HMENU)4031, nullptr, nullptr);
        create_unit(285, gy - 2);
        CreateWindowW(L"STATIC", L"표시색상", WS_CHILD | WS_VISIBLE, 360, gy, 60, 18, hwnd, nullptr, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 430, gy - 3, 75, 23, hwnd, (HMENU)3002, nullptr, nullptr);
        gy += 30;

        // 4. 목표수익
        HWND hChk4 = CreateWindowW(L"BUTTON", L"목표수익", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 28, gy, 150, 20, hwnd, (HMENU)4040, nullptr, nullptr);
        Button_SetCheck(hChk4, g_ActiveConfig.chk_target ? BST_CHECKED : BST_UNCHECKED);
        swprintf_s(buf, L"%.1f", g_ActiveConfig.val_target);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", buf, WS_CHILD | WS_VISIBLE, 225, gy - 2, 55, 22, hwnd, (HMENU)4041, nullptr, nullptr);
        create_unit(285, gy - 2);
        CreateWindowW(L"STATIC", L"표시색상", WS_CHILD | WS_VISIBLE, 360, gy, 60, 18, hwnd, nullptr, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 430, gy - 3, 75, 23, hwnd, (HMENU)3003, nullptr, nullptr);
        gy += 30;

        // 5. 최소가격변화
        CreateWindowW(L"BUTTON", L"최소가격변화", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 28, gy, 150, 20, hwnd, nullptr, nullptr, nullptr);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"2.0", WS_CHILD | WS_VISIBLE, 225, gy - 2, 55, 22, hwnd, nullptr, nullptr, nullptr);
        create_unit(285, gy - 2);
        CreateWindowW(L"STATIC", L"표시색상", WS_CHILD | WS_VISIBLE, 360, gy, 60, 18, hwnd, nullptr, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 430, gy - 3, 75, 23, hwnd, (HMENU)3004, nullptr, nullptr);
        gy += 25;
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"10", WS_CHILD | WS_VISIBLE, 225, gy - 2, 55, 22, hwnd, nullptr, nullptr, nullptr);
        CreateWindowW(L"STATIC", L"봉 이내", WS_CHILD | WS_VISIBLE, 290, gy, 60, 18, hwnd, nullptr, nullptr, nullptr);
        gy += 30;

        // 6. 당일청산
        CreateWindowW(L"BUTTON", L"당일청산", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 28, gy, 90, 20, hwnd, nullptr, nullptr, nullptr);
        CreateWindowW(L"STATIC", L"시간", WS_CHILD | WS_VISIBLE, 185, gy, 35, 18, hwnd, nullptr, nullptr, nullptr);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"02:30", WS_CHILD | WS_VISIBLE, 225, gy - 2, 55, 22, hwnd, nullptr, nullptr, nullptr);
        HWND hAmPm = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 285, gy - 2, 50, 100, hwnd, nullptr, nullptr, nullptr);
        ComboBox_AddString(hAmPm, L"PM"); ComboBox_AddString(hAmPm, L"AM"); ComboBox_SetCurSel(hAmPm, 0);

        CreateWindowW(L"STATIC", L"표시색상", WS_CHILD | WS_VISIBLE, 360, gy, 60, 18, hwnd, nullptr, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 430, gy - 3, 75, 23, hwnd, (HMENU)3005, nullptr, nullptr);
        gy += 28;

        CreateWindowW(L"STATIC", L"강제청산 시점", WS_CHILD | WS_VISIBLE, 28, gy + 2, 110, 18, hwnd, nullptr, nullptr, nullptr);
        HWND hExitTm = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 225, gy, 110, 100, hwnd, nullptr, nullptr, nullptr);
        ComboBox_AddString(hExitTm, L"조건 만족시"); ComboBox_AddString(hExitTm, L"봉 완성시"); ComboBox_SetCurSel(hExitTm, 0);

        CreateWindowW(L"BUTTON", L"시스템 트레이딩 설정", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 14, 580, 155, 30, hwnd, (HMENU)2001, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"확인", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 365, 580, 85, 30, hwnd, (HMENU)IDOK, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"취소", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 460, 580, 85, 30, hwnd, (HMENU)IDCANCEL, nullptr, nullptr);
        return 0;
    }
    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT)lParam;
        if (pDIS->CtlID >= 3000 && pDIS->CtlID <= 3005) {
            int c_idx = pDIS->CtlID - 3000;
            HBRUSH hBrush = CreateSolidBrush(g_ActiveConfig.colors[c_idx]);
            FillRect(pDIS->hDC, &pDIS->rcItem, hBrush);
            DeleteObject(hBrush);

            DrawEdge(pDIS->hDC, &pDIS->rcItem, (pDIS->itemState & ODS_SELECTED) ? EDGE_SUNKEN : EDGE_RAISED, BF_RECT);
            RECT rTri = pDIS->rcItem; rTri.left = rTri.right - 14;
            HBRUSH hBlack = (HBRUSH)GetStockObject(BLACK_BRUSH);
            POINT pts[3] = { { rTri.left + 2, rTri.top + 8 }, { rTri.left + 8, rTri.top + 8 }, { rTri.left + 5, rTri.top + 12 } };
            HRGN hRgn = CreatePolygonRgn(pts, 3, WINDING);
            FillRgn(pDIS->hDC, hRgn, hBlack);
            DeleteObject(hRgn);
            return TRUE;
        }
        break;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id >= 3000 && id <= 3005) {
            int c_idx = id - 3000;
            CHOOSECOLORW cc{};
            cc.lStructSize = sizeof(cc);
            cc.hwndOwner = hwnd;
            cc.lpCustColors = g_CustomColors;
            cc.rgbResult = g_ActiveConfig.colors[c_idx];
            cc.Flags = CC_FULLOPEN | CC_RGBINIT;
            if (ChooseColorW(&cc)) {
                g_ActiveConfig.colors[c_idx] = cc.rgbResult;
                InvalidateRect(GetDlgItem(hwnd, id), nullptr, TRUE);
            }
            return 0;
        }
        else if (id == 2001) {
            ShowFormulaManagerDialog();
        }
        else if (id == IDOK) {
            wchar_t sbuf[32];
            // 1. 최대허용손실 수집
            GetWindowTextW(GetDlgItem(hwnd, 4011), sbuf, 32);
            g_ActiveConfig.val_stop_loss = (float)_wtof(sbuf);
            g_ActiveConfig.chk_stop_loss = (Button_GetCheck(GetDlgItem(hwnd, 4010)) == BST_CHECKED);

            // 2. 최대수익대비하락(트레일링) 수집
            GetWindowTextW(GetDlgItem(hwnd, 4021), sbuf, 32);
            g_ActiveConfig.val_trailing = (float)_wtof(sbuf);
            GetWindowTextW(GetDlgItem(hwnd, 4022), sbuf, 32);
            g_ActiveConfig.val_trail_start = (float)_wtof(sbuf);
            g_ActiveConfig.chk_trailing = (Button_GetCheck(GetDlgItem(hwnd, 4020)) == BST_CHECKED);

            // 3. 이익보존율 수집
            GetWindowTextW(GetDlgItem(hwnd, 4031), sbuf, 32);
            g_ActiveConfig.val_protect = (float)_wtof(sbuf);
            g_ActiveConfig.chk_protect = (Button_GetCheck(GetDlgItem(hwnd, 4030)) == BST_CHECKED);

            // 4. 목표수익 수집
            GetWindowTextW(GetDlgItem(hwnd, 4041), sbuf, 32);
            g_ActiveConfig.val_target = (float)_wtof(sbuf);
            g_ActiveConfig.chk_target = (Button_GetCheck(GetDlgItem(hwnd, 4040)) == BST_CHECKED);

            // 영구 저장
            std::wstring save_path = L"strategy_settings_" + g_ActiveConfig.strategy_name + L".json";
            g_ActiveConfig.SaveToFile(save_path);

            ShowWindow(hwnd, SW_HIDE);
        }
        else if (id == IDCANCEL) {
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