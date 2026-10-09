#pragma once
#ifndef WM_USER_CHEJAN
#define WM_USER_CHEJAN      (WM_USER + 102)
#endif
#ifndef WM_USER_CONDITION
#define WM_USER_CONDITION   (WM_USER + 103)
#endif
#include "Common.hpp"
#include "ChartTypes.hpp"
#include "CentralDataManager.hpp"
#include "LayoutPersistence.hpp"
#include "StrategyConditionDlg.hpp"
#include "FormulaManagerDlg.hpp"

inline HWND g_hStrategyDlg = nullptr;

inline void ShowStrategyConditionDialog(const std::wstring& stock_name, const std::wstring& strat_name) {
    if (!g_hStrategyDlg) {
        WNDCLASSW wc{};
        wc.lpfnWndProc = StrategyCondDlgProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = L"KiwoomStrategyCondDlgClass_Sep";
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        RegisterClassW(&wc);

        g_hStrategyDlg = CreateWindowExW(
            WS_EX_DLGMODALFRAME, wc.lpszClassName,
            (L"매매전략 조건 설정 - " + strat_name).c_str(),
            WS_POPUP | WS_CAPTION | WS_SYSMENU,
            200, 100, 575, 665,
            nullptr, nullptr, wc.hInstance, nullptr
        );
    } else {
        SetWindowTextW(g_hStrategyDlg, (L"매매전략 조건 설정 - " + strat_name).c_str());
    }
    ShowWindow(g_hStrategyDlg, SW_SHOW);
    SetForegroundWindow(g_hStrategyDlg);
}

struct ChartSlotState {
    std::wstring code;
    int visible_bars = 60;
    int scroll_offset = 0;
    int future_bars = 6;
    D2D1_RECT_F badge_rect{ 0, 0, 0, 0 };
    std::vector<std::shared_ptr<IChartAddon>> addons;
};

class ChartCoreEngine {
public:
    std::wstring status_msg = L"[READY] Standby";
    bool status_is_error = false;
public:
        HWND hEditSymbol = nullptr;
    WNDPROC origEditProc = nullptr;

    static LRESULT CALLBACK SymbolEditSubclassProc(HWND hEdit, UINT msg, WPARAM wParam, LPARAM lParam) {
        auto* pEngine = reinterpret_cast<ChartCoreEngine*>(GetWindowLongPtrW(hEdit, GWLP_USERDATA));
        if (msg == WM_KEYDOWN && wParam == VK_RETURN) {
            wchar_t buf[32]{};
            GetWindowTextW(hEdit, buf, 32);
            if (wcslen(buf) >= 6 && pEngine) {
                pEngine->ChangeSlotSymbol(buf);
            }
            return 0;
        }
        if (pEngine && pEngine->origEditProc) {
            return CallWindowProcW(pEngine->origEditProc, hEdit, msg, wParam, lParam);
        }
        return DefWindowProcW(hEdit, msg, wParam, lParam);
    }

        void ChangeSlotSymbol(const std::wstring& newCode) {
        if (active_slot < 0 || active_slot >= (int)slots.size()) return;
        
        status_msg = L"[REQ] Requesting " + newCode + L"...";
        status_is_error = false;
        InvalidateRect(hwnd, nullptr, FALSE);

        bool ok = CentralDataManager::Instance().RequestDataFromBridge(newCode, tf_type, tf_unit, 150);
        const auto& c = CentralDataManager::Instance().GetCandles(newCode);

        if (ok && !c.empty()) {
            slots[active_slot].code = newCode;
            for (auto& a : slots[active_slot].addons) {
                a->OnUpdate(c);
            }
            status_msg = L"[OK] " + newCode + L" (" + std::to_wstring(c.size()) + L" bars)";
            status_is_error = false;
            std::wcout << L"[CHART] Switched slot " << active_slot << L" to " << newCode << std::endl;
        } else {
            status_msg = L"[FAIL] Bridge error for " + newCode;
            status_is_error = true;
            std::wcout << L"[CHART][ERROR] Bridge fetch failed for: " << newCode << std::endl;
        }
        InvalidateRect(hwnd, nullptr, FALSE);
    }
public:
    HWND hwnd = nullptr;
    ID2D1Factory* pD2D = nullptr;
    ID2D1HwndRenderTarget* pRT = nullptr;
    IDWriteFactory* pDW = nullptr;
    IDWriteTextFormat* pFSmall = nullptr;
    IDWriteTextFormat* pFBold = nullptr;
    ID2D1StrokeStyle* pDashStroke = nullptr;

    ID2D1SolidColorBrush* bBg = nullptr;
    ID2D1SolidColorBrush* bGrid = nullptr;
    ID2D1SolidColorBrush* bRed = nullptr;
    ID2D1SolidColorBrush* bBlue = nullptr;
    ID2D1SolidColorBrush* bGold = nullptr;
    ID2D1SolidColorBrush* bWhite = nullptr;
    ID2D1SolidColorBrush* bGray = nullptr;
    ID2D1SolidColorBrush* bToolBarBg = nullptr;
    ID2D1SolidColorBrush* bToolBtnBg = nullptr;
    ID2D1SolidColorBrush* bToolActive = nullptr;
    ID2D1SolidColorBrush* bGreen = nullptr;
    ID2D1SolidColorBrush* bFocusBorder = nullptr;
    ID2D1SolidColorBrush* bCrossPrice = nullptr;
    ID2D1SolidColorBrush* bCrossTime = nullptr;
    ID2D1SolidColorBrush* bBadgeBg = nullptr;

    std::vector<ChartSlotState> slots;
    int active_slot = 0;
    int layout_mode = 4;
    std::wstring cur_tf = L"1";
    char tf_type = 'm';
    int tf_unit = 1;

    bool crosshair_on = true;
    bool sync_tf = true;
    POINT mouse_pt{ -1, -1 };
    bool is_mouse_inside = false;
    bool is_dragging = false;
    POINT drag_start_pt{ 0, 0 };
    int drag_start_offset = 0;

    void Init(HWND h) {
        if (!hEditSymbol) {
            hEditSymbol = CreateWindowExW(
                WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_UPPERCASE,
                240, 5, 75, 22,
                hwnd, (HMENU)9001, GetModuleHandleW(nullptr), nullptr
            );
            origEditProc = (WNDPROC)SetWindowLongPtrW(hEditSymbol, GWLP_WNDPROC, (LONG_PTR)SymbolEditSubclassProc);
            HFONT hFont = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"맑은 고딕");
            SendMessageW(hEditSymbol, WM_SETFONT, (WPARAM)hFont, TRUE);
        }
        hwnd = h;
        D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2D);
        D2D1_STROKE_STYLE_PROPERTIES sp = D2D1::StrokeStyleProperties(
            D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_ROUND,
            D2D1_LINE_JOIN_MITER, 10.0f, D2D1_DASH_STYLE_DASH, 0.0f
        );
        pD2D->CreateStrokeStyle(sp, nullptr, 0, &pDashStroke);

        DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&pDW);
        pDW->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &pFSmall);
        pDW->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 11.5f, L"en-us", &pFBold);

        const wchar_t* codes[] = { L"A000660", L"A005930", L"A028050", L"A047810" };
        for (int i = 0; i < 4; ++i) {
            ChartSlotState s; s.code = codes[i];
            s.addons.push_back(std::make_shared<DynamicTradingStrategyAddon>(g_ActiveConfig, 5, 20));
            slots.push_back(s);
        }

        CentralDataManager::Instance().LoadInitialData(L"cybos_master_data.bin", tf_type, tf_unit);
        LoadLayoutConfig();
        CentralDataManager::Instance().StartRealTimeListener();
        // 키움 전담 트레이딩 & 조건검색 리스너 자동 기동
        HWND targetHwnd = this->hwnd;
        CentralDataManager::Instance().SetChejanCallback([targetHwnd](const KiwoomChejanPacket& p) {
            std::wstring* pStatus = new std::wstring(
                std::wstring(p.code) + L" " + p.status + L" " + 
                std::to_wstring(p.filledQty) + L"주 @" + std::to_wstring(p.filledPrice)
            );
            PostMessage(targetHwnd, WM_USER_CHEJAN, (WPARAM)pStatus, 0);
        });
        CentralDataManager::Instance().StartChejanListener();

        CentralDataManager::Instance().SetConditionCallback([targetHwnd](const KiwoomConditionRealPacket& p) {
            std::wstring* pCond = new std::wstring(
                std::wstring(p.conditionName) + L" [" + (p.eventType == 'I' ? L"편입" : L"이탈") + L"] " + p.code
            );
            PostMessage(targetHwnd, WM_USER_CONDITION, (WPARAM)pCond, 0);
        });
        CentralDataManager::Instance().StartConditionListener();
        // Cybos 배치 스냅샷 초기 요청 (관심 종목)
        std::vector<std::wstring> watchList = { L"A005930", L"A000660" };
        m_marketEyeSnapshot = CentralDataManager::Instance().RequestMarketEye(watchList);
        CentralDataManager::Instance().SetTickCallback([this](const std::wstring& code) {
            for (const auto& s : slots) {
                if (s.code == code) {
                    if (hwnd) InvalidateRect(hwnd, nullptr, FALSE);
                    break;
                }
            }
        });
        for (auto& s : slots) {
            const auto& c = CentralDataManager::Instance().GetCandles(s.code);
            for (auto& a : s.addons) a->OnUpdate(c);
        }
    }

    void Render() {
        if (!pRT) {
            RECT rc; GetClientRect(hwnd, &rc);
            D2D1_SIZE_U sz = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
            pD2D->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hwnd, sz), &pRT);
            pRT->CreateSolidColorBrush(D2D1::ColorF(0.06f, 0.06f, 0.07f), &bBg);
            pRT->CreateSolidColorBrush(D2D1::ColorF(0.18f, 0.18f, 0.20f), &bGrid);
            pRT->CreateSolidColorBrush(D2D1::ColorF(0.92f, 0.22f, 0.25f), &bRed);
            pRT->CreateSolidColorBrush(D2D1::ColorF(0.20f, 0.55f, 0.95f), &bBlue);
            pRT->CreateSolidColorBrush(D2D1::ColorF(0.95f, 0.72f, 0.15f), &bGold);
            pRT->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f), &bWhite);
            pRT->CreateSolidColorBrush(D2D1::ColorF(0.65f, 0.65f, 0.68f), &bGray);
            pRT->CreateSolidColorBrush(D2D1::ColorF(0.14f, 0.14f, 0.16f), &bToolBarBg);
            pRT->CreateSolidColorBrush(D2D1::ColorF(0.22f, 0.22f, 0.25f), &bToolBtnBg);
            pRT->CreateSolidColorBrush(D2D1::ColorF(0.95f, 0.50f, 0.10f), &bToolActive);
            pRT->CreateSolidColorBrush(D2D1::ColorF(0.15f, 0.85f, 0.35f), &bGreen);
            pRT->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.40f, 0.0f), &bFocusBorder);
            pRT->CreateSolidColorBrush(D2D1::ColorF(0.70f, 0.10f, 0.15f), &bCrossPrice);
            pRT->CreateSolidColorBrush(D2D1::ColorF(0.10f, 0.30f, 0.65f), &bCrossTime);
            pRT->CreateSolidColorBrush(D2D1::ColorF(0.25f, 0.35f, 0.55f), &bBadgeBg);
        }

        pRT->BeginDraw();
        pRT->Clear(D2D1::ColorF(0.06f, 0.06f, 0.07f));

        RECT rc; GetClientRect(hwnd, &rc);
        float w = (float)(rc.right - rc.left), h = (float)(rc.bottom - rc.top);

        float topbar_h = 32.0f;
        DrawTopBar(w, topbar_h);

        float main_top = topbar_h + 1.0f;
        float chart_h = h - main_top;

        if (layout_mode == 4 && slots.size() >= 4) {
            float hw = w * 0.5f, hh = chart_h * 0.5f;
            DrawSlot(0, D2D1::RectF(0, main_top, hw, main_top + hh));
            DrawSlot(1, D2D1::RectF(hw, main_top, w, main_top + hh));
            DrawSlot(2, D2D1::RectF(0, main_top + hh, hw, h));
            DrawSlot(3, D2D1::RectF(hw, main_top + hh, w, h));
        } else if (!slots.empty()) {
            DrawSlot(active_slot, D2D1::RectF(0, main_top, w, h));
        }

        pRT->EndDraw();
    }

    void DrawTopBar(float w, float h) {
        pRT->FillRectangle(D2D1::RectF(0, 0, w, h), bToolBarBg);
                pRT->DrawLine(D2D1::Point2F(0, h), D2D1::Point2F(w, h), bGrid, 1.0f);

        // State HUD (Fail-Fast 시각 피드백)
        if (!status_msg.empty()) {
            D2D1_RECT_F sRect = D2D1::RectF(w - 280, 5, w - 10, h - 5);
            pRT->DrawText(status_msg.c_str(), (UINT32)status_msg.size(), pFSmall, sRect, status_is_error ? bRed : bGold);
        }

        auto btn = [this](float x, float y, float bw, float bh, const wchar_t* txt, bool act) {
            D2D1_RECT_F r = D2D1::RectF(x, y, x + bw, y + bh);
            pRT->FillRectangle(r, act ? bToolActive : bToolBtnBg);
            pRT->DrawRectangle(r, bGrid, 1.0f);
            pRT->DrawText(txt, (UINT32)wcslen(txt), pFSmall, D2D1::RectF(x + 2, y + 2, x + bw - 2, y + bh), act ? bWhite : bGray);
        };

        float cx = 6.0f;
        btn(cx, 5, 32, 22, L"2x2", layout_mode == 4); cx += 35;
        btn(cx, 5, 32, 22, L"1x1", layout_mode == 1); cx += 40;
        btn(cx, 5, 48, 22, L"=주기", sync_tf); cx += 52;

        const wchar_t* mins[] = { L"1", L"3", L"5", L"10", L"일" };
        for (const auto* m : mins) { btn(cx, 5, 24, 22, m, cur_tf == m); cx += 26; }
        cx += 10;

        const auto& c = CentralDataManager::Instance().GetCandles(slots[active_slot].code);
        wchar_t bcnt[32]; swprintf_s(bcnt, L"%d/%d", slots[active_slot].visible_bars, (int)c.size());
        btn(cx, 5, 60, 22, bcnt, false); cx += 64;

        btn(cx, 5, 24, 22, L"◀", false); cx += 26;
        btn(cx, 5, 24, 22, L"▶", false); cx += 26;
        btn(cx, 5, 24, 22, L"+", false); cx += 26;
        btn(cx, 5, 24, 22, L"-", false); cx += 30;
        btn(cx, 5, 48, 22, L"여백+", false); cx += 52;

        btn(cx, 5, 78, 22, L"수식관리자", false); cx += 82;
        btn(cx, 5, 54, 22, L"크로스", crosshair_on);
    }

    void DrawSlot(int slot_idx, D2D1_RECT_F b) {
        auto& slot = slots[slot_idx];
        auto master = CentralDataManager::Instance().GetMaster(slot.code);
        const auto& candles = CentralDataManager::Instance().GetCandles(slot.code);

        bool is_focused = (slot_idx == active_slot);
        pRT->DrawRectangle(b, is_focused ? bFocusBorder : bGrid, is_focused ? 2.5f : 1.0f);

        float hdr_h = 24.0f;
        pRT->FillRectangle(D2D1::RectF(b.left + 2, b.top + 2, b.right - 2, b.top + hdr_h), bToolBarBg);

        wchar_t htxt[256];
        swprintf_s(htxt, L"[%s] %s  %.0f  %s%.0f (%.2f%%)",
            master.code.c_str(), master.name.c_str(), master.cur_price,
            master.change_val >= 0 ? L"▲" : L"▼", std::abs(master.change_val), master.change_rate);
        pRT->DrawText(htxt, (UINT32)wcslen(htxt), pFBold, D2D1::RectF(b.left + 6, b.top + 4, b.left + 270, b.top + hdr_h),
            master.change_val >= 0 ? bRed : bBlue);

        // 전략 배지
        std::wstring strat_name = L"";
        for (const auto& a : slot.addons) {
            if (a->IsEnabled()) { strat_name = a->GetName(); break; }
        }

        if (!strat_name.empty()) {
            float bx = b.left + 275.0f;
            slot.badge_rect = D2D1::RectF(bx, b.top + 3.0f, bx + 125.0f, b.top + hdr_h - 3.0f);
            pRT->FillRectangle(slot.badge_rect, bBadgeBg);
            pRT->DrawRectangle(slot.badge_rect, bGold, 1.0f);
            std::wstring btxt = L"전략: " + strat_name;
            pRT->DrawText(btxt.c_str(), (UINT32)btxt.size(), pFSmall,
                D2D1::RectF(bx + 4, b.top + 5, bx + 120.0f, b.top + hdr_h), bGold);
        } else {
            slot.badge_rect = D2D1::RectF(0, 0, 0, 0);
        }

        if (candles.empty()) return;

        float y_scale_w = 64.0f, x_scale_h = 20.0f;
        float main_w = (b.right - b.left) - y_scale_w;
        float total_chart_h = (b.bottom - b.top) - hdr_h - x_scale_h;
        float candle_top = b.top + hdr_h, candle_h = total_chart_h * 0.68f, candle_bot = candle_top + candle_h;
        float vol_top = candle_bot + 1.0f, vol_bot = b.bottom - x_scale_h, vol_h = vol_bot - vol_top;

        pRT->FillRectangle(D2D1::RectF(b.left + main_w, candle_top, b.right, b.bottom), bToolBarBg);
        pRT->DrawLine(D2D1::Point2F(b.left + main_w, candle_top), D2D1::Point2F(b.left + main_w, b.bottom), bGrid, 1.0f);

        int total_cnt = (int)candles.size();
        int vis = (std::min)(total_cnt, slot.visible_bars);
        int start_i = total_cnt - vis - slot.scroll_offset;
        if (start_i < 0) start_i = 0;
        int end_i = start_i + vis;

        float min_p = candles[start_i].low, max_p = candles[start_i].high;
        uint64_t max_v = 1;
        for (int i = start_i; i < end_i; ++i) {
            min_p = (std::min)(min_p, candles[i].low); max_p = (std::max)(max_p, candles[i].high);
            max_v = (std::max)(max_v, candles[i].volume);
        }
        float p_range = (max_p - min_p < 1.0f) ? 1.0f : (max_p - min_p);

        ViewportTransform vp{ b.left, candle_top, main_w, candle_h, min_p, max_p, start_i, end_i, slot.future_bars };

        // 그리드 & Y축 라벨
        for (int g = 0; g <= 4; ++g) {
            float r = (float)g / 4.0f;
            float gy = candle_bot - r * candle_h;
            pRT->DrawLine(D2D1::Point2F(b.left, gy), D2D1::Point2F(b.left + main_w, gy), bGrid, 0.8f, pDashStroke);
            wchar_t vtxt[32]; swprintf_s(vtxt, L"%.0f", min_p + r * p_range);
            pRT->DrawText(vtxt, (UINT32)wcslen(vtxt), pFSmall, D2D1::RectF(b.left + main_w + 4, gy - 7, b.right - 2, gy + 8), bGray);
        }

        // 캔들 & 거래량 바
        int total_slots = (end_i - start_i) + slot.future_bars;
        float step = main_w / (float)total_slots, bar_w = step * 0.72f;

        for (int i = 0; i < (end_i - start_i); ++i) {
            int ci = start_i + i;
            const auto& c = candles[ci];
            float cx = b.left + i * step + step * 0.5f;
            float yh = vp.PriceToY(c.high), yl = vp.PriceToY(c.low);
            float yo = vp.PriceToY(c.open), yc = vp.PriceToY(c.close);

            ID2D1SolidColorBrush* cb = (c.close >= c.open) ? bRed : bBlue;
            pRT->DrawLine(D2D1::Point2F(cx, yh), D2D1::Point2F(cx, yl), cb, 1.2f);
            float tb = (std::min)(yo, yc), bb = (std::max)(yo, yc);
            if (bb - tb < 1.5f) bb = tb + 1.5f;
            pRT->FillRectangle(D2D1::RectF(cx - bar_w * 0.5f, tb, cx + bar_w * 0.5f, bb), cb);

            float vr = (float)c.volume / (float)max_v;
            pRT->FillRectangle(D2D1::RectF(cx - bar_w * 0.5f, vol_bot - vr * vol_h, cx + bar_w * 0.5f, vol_bot), bGold);
        }

        for (auto& a : slot.addons) a->OnRender(pRT, vp, pFSmall);

        // 크로스헤어
        if (crosshair_on && is_mouse_inside &&
            mouse_pt.x >= b.left && mouse_pt.x <= b.left + main_w &&
            mouse_pt.y >= candle_top && mouse_pt.y <= vol_bot) {

            float mx = (float)mouse_pt.x, my = (float)mouse_pt.y;
            pRT->DrawLine(D2D1::Point2F(b.left, my), D2D1::Point2F(b.left + main_w, my), bWhite, 0.8f, pDashStroke);
            pRT->DrawLine(D2D1::Point2F(mx, candle_top), D2D1::Point2F(mx, vol_bot), bWhite, 0.8f, pDashStroke);

            if (my <= candle_bot) {
                float cp = min_p + ((candle_bot - my) / candle_h) * p_range;
                D2D1_RECT_F pbox = D2D1::RectF(b.left + main_w + 2, my - 8, b.right - 2, my + 8);
                pRT->FillRectangle(pbox, bCrossPrice);
                pRT->DrawRectangle(pbox, bWhite, 1.0f);
                wchar_t ptxt[32]; swprintf_s(ptxt, L"%.0f", cp);
                pRT->DrawText(ptxt, (UINT32)wcslen(ptxt), pFBold, pbox, bWhite);
            }
        }
    }

    void OnLButtonDown(int x, int y) {
        if (y <= 32) {
            float cx = 6.0f;
            if (x >= cx && x <= cx + 32) layout_mode = 4; cx += 35;
            if (x >= cx && x <= cx + 32) layout_mode = 1; cx += 40;
            if (x >= cx && x <= cx + 48) sync_tf = !sync_tf; cx += 52;

            auto reload_tf = [this](const wchar_t* lbl, char t, int u) {
                cur_tf = lbl;
                CentralDataManager::Instance().LoadInitialData(L"cybos_master_data.bin", t, u);
                for (auto& s : slots) {
                    const auto& c = CentralDataManager::Instance().GetCandles(s.code);
                    for (auto& a : s.addons) a->OnUpdate(c);
                }
            };
            if (x >= cx && x <= cx + 24) reload_tf(L"1", 'm', 1); cx += 26;
            if (x >= cx && x <= cx + 24) reload_tf(L"3", 'm', 3); cx += 26;
            if (x >= cx && x <= cx + 24) reload_tf(L"5", 'm', 5); cx += 26;
            if (x >= cx && x <= cx + 24) reload_tf(L"10", 'm', 10); cx += 26;
            if (x >= cx && x <= cx + 24) reload_tf(L"일", 'D', 1); cx += 36;

            cx += 64; // 봉수 스킵

            if (x >= cx && x <= cx + 24) { for (auto& s : slots) s.scroll_offset += 10; } cx += 26;
            if (x >= cx && x <= cx + 24) { for (auto& s : slots) { if (s.scroll_offset > 0) s.scroll_offset -= 10; } } cx += 26;
            if (x >= cx && x <= cx + 24) { for (auto& s : slots) { if (s.visible_bars > 15) s.visible_bars -= 10; } } cx += 26;
            if (x >= cx && x <= cx + 24) { for (auto& s : slots) { if (s.visible_bars < 200) s.visible_bars += 10; } } cx += 30;
            if (x >= cx && x <= cx + 48) { slots[active_slot].future_bars = (slots[active_slot].future_bars >= 15) ? 6 : slots[active_slot].future_bars + 3; } cx += 52;

            if (x >= cx && x <= cx + 78) { ShowFormulaManagerDialog(); return; } cx += 82;
            if (x >= cx && x <= cx + 54) { crosshair_on = !crosshair_on; }
            InvalidateRect(hwnd, nullptr, FALSE);
            return;
        }

        RECT rc; GetClientRect(hwnd, &rc);
        float w = (float)(rc.right - rc.left), h = (float)(rc.bottom - rc.top);
        float main_top = 33.0f;
        float hw = w * 0.5f, hh = (h - main_top) * 0.5f;

        if (layout_mode == 4) {
            if (x < hw && y < (main_top + hh)) active_slot = 0;
            else if (x >= hw && y < (main_top + hh)) active_slot = 1;
            else if (x < hw && y >= (main_top + hh)) active_slot = 2;
            else active_slot = 3;
        }

        is_dragging = true;
        drag_start_pt.x = x; drag_start_pt.y = y;
        drag_start_offset = slots[active_slot].scroll_offset;
        SetCapture(hwnd);
        InvalidateRect(hwnd, nullptr, FALSE);
    }

    void OnLButtonDblClk(int x, int y) {
        for (size_t i = 0; i < slots.size(); ++i) {
            const auto& r = slots[i].badge_rect;
            if (r.right > r.left && x >= r.left && x <= r.right && y >= r.top && y <= r.bottom) {
                active_slot = (int)i;
                auto master = CentralDataManager::Instance().GetMaster(slots[i].code);
                std::wstring sname = L"ma cross";
                for (const auto& a : slots[i].addons) {
                    if (a->GetId() == L"DYNAMIC_STRATEGY") { sname = a->GetName(); break; }
                }
                ShowStrategyConditionDialog(master.name, sname);
                return;
            }
        }
    }

    void OnRButtonUp(int x, int y) {
        HMENU hMenu = CreatePopupMenu();
        AppendMenuW(hMenu, MF_STRING, 8001, L"[수식관리자 (키움 0601)] 열기");
        AppendMenuW(hMenu, MF_STRING, 8002, L"[매매전략 조건 설정] 열기");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hMenu, MF_STRING, 8003, L"[전략 삽입] ma cross (5-20 이평)");
        AppendMenuW(hMenu, MF_STRING, 8004, L"[현재 슬롯 전략 제거]");

        POINT pt = { x, y }; ClientToScreen(hwnd, &pt);
        int cmd = TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, nullptr);
        DestroyMenu(hMenu);

        auto master = CentralDataManager::Instance().GetMaster(slots[active_slot].code);
        if (cmd == 8001) ShowFormulaManagerDialog();
        else if (cmd == 8002) ShowStrategyConditionDialog(master.name, g_ActiveConfig.strategy_name);
        else if (cmd == 8003) {
            slots[active_slot].addons.clear();
            slots[active_slot].addons.push_back(std::make_shared<DynamicTradingStrategyAddon>(g_ActiveConfig, 5, 20));
            const auto& c = CentralDataManager::Instance().GetCandles(slots[active_slot].code);
            slots[active_slot].addons.back()->OnUpdate(c);
            InvalidateRect(hwnd, nullptr, FALSE);
        } else if (cmd == 8004) {
            slots[active_slot].addons.clear();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
    }

    void OnMouseMove(int x, int y) {
        mouse_pt.x = x; mouse_pt.y = y; is_mouse_inside = true;
        if (is_dragging) {
            int dx = x - drag_start_pt.x;
            slots[active_slot].scroll_offset = drag_start_offset + (dx / 8);
            if (slots[active_slot].scroll_offset < 0) slots[active_slot].scroll_offset = 0;
        }
        if (crosshair_on || is_dragging) InvalidateRect(hwnd, nullptr, FALSE);
    }

    void OnMouseWheel(short delta) {
        auto zoom = [delta](ChartSlotState& s) {
            if (delta > 0 && s.visible_bars > 15) s.visible_bars -= 5;
            else if (delta < 0 && s.visible_bars < 200) s.visible_bars += 5;
        };
        if (sync_tf) { for (auto& s : slots) zoom(s); }
        else zoom(slots[active_slot]);
        InvalidateRect(hwnd, nullptr, FALSE);
    }

    void SaveLayoutConfig() {
        LayoutPersistData d;
        d.layout_mode = layout_mode;
        for (const auto& s : slots) {
            SlotPersistData sp;
            sp.code = s.code;
            sp.tf_type = tf_type;
            sp.tf_unit = tf_unit;
            sp.visible_bars = s.visible_bars;
            sp.scroll_offset = s.scroll_offset;
            d.slots.push_back(sp);
        }
        LayoutPersistence::SaveToFile(L"layout_config.json", d);
        std::wcout << L"[PERSIST] Layout configuration saved to layout_config.json" << std::endl;
    }

    void LoadLayoutConfig() {
        LayoutPersistData d;
        if (LayoutPersistence::LoadFromFile(L"layout_config.json", d)) {
            layout_mode = d.layout_mode;
            for (size_t i = 0; i < d.slots.size() && i < slots.size(); ++i) {
                slots[i].code = d.slots[i].code;
                slots[i].visible_bars = d.slots[i].visible_bars;
                slots[i].scroll_offset = d.slots[i].scroll_offset;
            }
            std::wcout << L"[PERSIST] Layout configuration restored from layout_config.json" << std::endl;
        }
    }
};

inline ChartCoreEngine g_Engine;
