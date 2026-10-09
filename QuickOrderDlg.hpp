#pragma once
#include <windows.h>
#include <string>
#include "ChartTypes.hpp"
#include "CentralDataManager.hpp"

class QuickOrderDlg {
private:
    HWND m_hDlg = nullptr;
    HWND m_hCodeEdit = nullptr;
    HWND m_hQtyEdit = nullptr;
    HWND m_hPriceEdit = nullptr;
    HWND m_hBuyBtn = nullptr;
    HWND m_hSellBtn = nullptr;
    std::wstring m_currentCode = L"005930";

public:
    static QuickOrderDlg& Instance() {
        static QuickOrderDlg instance;
        return instance;
    }

    void Show(HWND hParent, const std::wstring& defaultCode = L"005930") {
        m_currentCode = defaultCode;
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
        wc.lpszClassName = L"QuickOrderDlgClass";
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        RegisterClassExW(&wc);
    }

    void CreateDlgWindow(HWND hParent) {
        RECT rcParent{};
        GetWindowRect(hParent, &rcParent);
        int x = rcParent.left + 50;
        int y = rcParent.top + 80;

        m_hDlg = CreateWindowExW(
            WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
            L"QuickOrderDlgClass", L"키움 주문 패널 (Kiwoom OpenAPI)",
            WS_POPUPWINDOW | WS_CAPTION,
            x, y, 320, 220,
            hParent, nullptr, GetModuleHandle(nullptr), this
        );

        CreateWindowW(L"STATIC", L"종목코드:", WS_CHILD | WS_VISIBLE, 20, 20, 70, 20, m_hDlg, nullptr, nullptr, nullptr);
        m_hCodeEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", m_currentCode.c_str(), WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 100, 18, 180, 24, m_hDlg, (HMENU)101, nullptr, nullptr);

        CreateWindowW(L"STATIC", L"주문수량:", WS_CHILD | WS_VISIBLE, 20, 55, 70, 20, m_hDlg, nullptr, nullptr, nullptr);
        m_hQtyEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"10", WS_CHILD | WS_VISIBLE | ES_NUMBER, 100, 53, 180, 24, m_hDlg, (HMENU)102, nullptr, nullptr);

        CreateWindowW(L"STATIC", L"주문가격:", WS_CHILD | WS_VISIBLE, 20, 90, 70, 20, m_hDlg, nullptr, nullptr, nullptr);
        m_hPriceEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0", WS_CHILD | WS_VISIBLE | ES_NUMBER, 100, 88, 180, 24, m_hDlg, (HMENU)103, nullptr, nullptr);

        m_hBuyBtn = CreateWindowW(L"BUTTON", L"신규매수 (시장가)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 130, 125, 36, m_hDlg, (HMENU)201, nullptr, nullptr);
        m_hSellBtn = CreateWindowW(L"BUTTON", L"신규매도 (시장가)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 155, 130, 125, 36, m_hDlg, (HMENU)202, nullptr, nullptr);
    }

    static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        QuickOrderDlg* pThis = nullptr;
        if (msg == WM_NCCREATE) {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            pThis = reinterpret_cast<QuickOrderDlg*>(cs->lpCreateParams);
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
        } else {
            pThis = reinterpret_cast<QuickOrderDlg*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
        }
        if (pThis) return pThis->HandleMessage(hWnd, msg, wParam, lParam);
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }

    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            if (wmId == 201 || wmId == 202) {
                wchar_t codeBuf[16]{}, qtyBuf[16]{}, priceBuf[16]{};
                GetWindowTextW(m_hCodeEdit, codeBuf, 16);
                GetWindowTextW(m_hQtyEdit, qtyBuf, 16);
                GetWindowTextW(m_hPriceEdit, priceBuf, 16);

                KiwoomOrderRequest req{};
                req.orderType = static_cast<KiwoomOrderType>((wmId == 201) ? 1 : 2);
                wcsncpy_s(req.code, sizeof(req.code) / sizeof(wchar_t), codeBuf, _TRUNCATE);
                req.qty = _wtoi(qtyBuf);
                req.price = _wtoi(priceBuf);
                wcsncpy_s(req.hogaType, sizeof(req.hogaType) / sizeof(wchar_t), (req.price == 0) ? L"03" : L"00", _TRUNCATE);

                bool ok = CentralDataManager::Instance().SendKiwoomOrder(req);
                if (ok) {
                    MessageBoxW(hWnd, L"키움 브릿지로 주문이 전송되었습니다.", L"주문 성공", MB_OK | MB_ICONINFORMATION);
                } else {
                    MessageBoxW(hWnd, L"키움 브릿지 파이프 전송에 실패했습니다.", L"주문 오류", MB_OK | MB_ICONERROR);
                }
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
