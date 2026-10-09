#pragma once
#include <windows.h>
#include <string>
#include "DynamicTradingStrategyAddon.hpp"

class StrategyConditionDlg {
private:
    HWND m_hDlg = nullptr;
    HWND m_hChkAutoOrder = nullptr;
    HWND m_hEditQty = nullptr;
    DynamicTradingStrategyAddon* m_pStrategyAddon = nullptr;

public:
    static StrategyConditionDlg& Instance() {
        static StrategyConditionDlg instance;
        return instance;
    }

    void BindStrategyAddon(DynamicTradingStrategyAddon* pAddon) {
        m_pStrategyAddon = pAddon;
    }

    void Show(HWND hParent) {
        if (m_hDlg && IsWindow(m_hDlg)) {
            SetFocus(m_hDlg);
            return;
        }
        RegisterDlgClass(GetModuleHandle(nullptr));
        CreateDlgWindow(hParent);
        ShowWindow(m_hDlg, SW_SHOW);
        UpdateWindow(m_hDlg);
    }

private:
    void RegisterDlgClass(HINSTANCE hInstance) {
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = StaticWndProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = L"StrategyConditionDlgClass";
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        RegisterClassExW(&wc);
    }

    void CreateDlgWindow(HWND hParent) {
        RECT rcParent{};
        GetWindowRect(hParent, &rcParent);
        int x = rcParent.left + 80;
        int y = rcParent.top + 100;

        m_hDlg = CreateWindowExW(
            WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
            L"StrategyConditionDlgClass", L"시스템매매 조건설정 (키움 0601)",
            WS_POPUPWINDOW | WS_CAPTION,
            x, y, 360, 240,
            hParent, nullptr, GetModuleHandle(nullptr), this
        );

        CreateWindowW(L"STATIC", L"[전략 규칙] 이평선 골든/데드크로스 시스템", WS_CHILD | WS_VISIBLE, 20, 20, 310, 20, m_hDlg, nullptr, nullptr, nullptr);

        m_hChkAutoOrder = CreateWindowW(
            L"BUTTON", L"신호 발생 시 키움 자동주문 즉시 전송",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            20, 60, 300, 24, m_hDlg, (HMENU)301, nullptr, nullptr
        );

        CreateWindowW(L"STATIC", L"기본 주문수량:", WS_CHILD | WS_VISIBLE, 20, 100, 100, 20, m_hDlg, nullptr, nullptr, nullptr);
        m_hEditQty = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"10", WS_CHILD | WS_VISIBLE | ES_NUMBER, 130, 98, 120, 24, m_hDlg, (HMENU)302, nullptr, nullptr);
        CreateWindowW(L"STATIC", L"주", WS_CHILD | WS_VISIBLE, 260, 100, 30, 20, m_hDlg, nullptr, nullptr, nullptr);

        CreateWindowW(L"BUTTON", L"적용 및 저장", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 60, 145, 110, 32, m_hDlg, (HMENU)IDOK, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"닫기", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 180, 145, 110, 32, m_hDlg, (HMENU)IDCANCEL, nullptr, nullptr);

        if (m_pStrategyAddon && m_pStrategyAddon->IsAutoOrderEnabled()) {
            SendMessageW(m_hChkAutoOrder, BM_SETCHECK, BST_CHECKED, 0);
            SetWindowTextW(m_hEditQty, std::to_wstring(m_pStrategyAddon->GetDefaultQty()).c_str());
        }
    }

    static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        StrategyConditionDlg* pThis = nullptr;
        if (msg == WM_NCCREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            pThis = reinterpret_cast<StrategyConditionDlg*>(cs->lpCreateParams);
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
        } else {
            pThis = reinterpret_cast<StrategyConditionDlg*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
        }
        if (pThis) return pThis->HandleMessage(hWnd, msg, wParam, lParam);
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }

    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            if (wmId == IDOK) {
                bool isChecked = (SendMessageW(m_hChkAutoOrder, BM_GETCHECK, 0, 0) == BST_CHECKED);
                wchar_t qtyBuf[16]{};
                GetWindowTextW(m_hEditQty, qtyBuf, 16);
                uint32_t qty = static_cast<uint32_t>(_wtoi(qtyBuf));
                if (qty == 0) qty = 10;

                if (m_pStrategyAddon) {
                    m_pStrategyAddon->SetAutoOrder(isChecked, qty);
                }
                MessageBoxW(hWnd, isChecked ? L"자동 주문이 활성화되었습니다." : L"자동 주문이 비활성화되었습니다.", L"설정 완료", MB_OK | MB_ICONINFORMATION);
                DestroyWindow(hWnd);
                m_hDlg = nullptr;
            } else if (wmId == IDCANCEL) {
                DestroyWindow(hWnd);
                m_hDlg = nullptr;
            }
            return 0;
        }
        case WM_CLOSE:
            DestroyWindow(hWnd);
            m_hDlg = nullptr;
            return 0;
        }
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
};
