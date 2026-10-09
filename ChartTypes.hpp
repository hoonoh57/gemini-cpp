#include <cstdint>
#include <string>
#pragma pack(push, 1)
struct PipeHeader {
    char magic[4];       // 'G','B','R','G'
    uint32_t msgType;    // 1: REQ, 2: RES_CANDLES, 3: REAL_TICK, 11: CHEJAN, 99: ERROR
    uint32_t payloadLen;
};

struct BridgeCandle {
    wchar_t date[16];
    wchar_t time[16];
    float open;
    float high;
    float low;
    float close;
    uint64_t volume;
    float ofi;
};

#pragma pack(push, 1)
struct MarketEyeItem {
    wchar_t code[16];
    wchar_t name[32];
    float curPrice;
    float diff;
    float diffRate;
    uint64_t volume;
    float askPrice;
    float bidPrice;
    int64_t instNetBuy;    // 기관 순매수량
    int64_t foreignNetBuy; // 외인 순매수량
};

#pragma pack(push, 1)
struct MarketEyeRequestPacket {
    uint32_t count;
    wchar_t codes[200][16]; // 최대 200종목 단위 배치 요청
};

#pragma pack(push, 1)
struct ProgramTradeItem {
    wchar_t time[16];          // 시간 (HHMMSS)
    float price;               // 현재가
    float diff;                // 대비
    int64_t diffVolume;        // 차익 순매수량
    int64_t nonDiffVolume;     // 비차익 순매수량
    int64_t totalNetVolume;    // 전체 순매수량
    int64_t totalNetMoney;     // 전체 순매수금액 (백만원)
};

struct ProgramTradeRequestPacket {
    wchar_t code[16];
    uint32_t count;            // 요청 건수 (예: 60건)
};

#pragma pack(push, 1)
struct SectorRankingItem {
    wchar_t code[16];          // 업종코드
    wchar_t name[32];          // 업종명
    float curIndex;            // 현재지수
    float diff;                // 대비
    float diffRate;            // 등락률
    uint64_t volume;           // 거래량(천주)
    uint64_t amount;           // 거래대금(백만)
    uint32_t upCount;          // 상승종목수
    uint32_t downCount;        // 하락종목수
};
#pragma pack(pop)
#pragma pack(pop)
#pragma pack(pop)
#pragma pack(pop)
#pragma pack(pop)

#pragma once
#include "Common.hpp"

struct Candle {
    std::wstring date;
    std::wstring time;
    float open, high, low, close;
    uint64_t volume;
    float ofi = 0.0f;
};

struct StockMaster {
    std::wstring code;
    std::wstring name;
    float cur_price = 0.0f;
    float change_val = 0.0f;
    float change_rate = 0.0f;
    float bid_price = 0.0f;
    float ask_price = 0.0f;
};

struct ViewportTransform {
    float left, top, width, height;
    float min_price, max_price;
    int start_index, end_index;
    int future_bars;

    float PriceToY(float price) const {
        float range = (max_price - min_price < 1.0f) ? 1.0f : (max_price - min_price);
        return top + height - ((price - min_price) / range) * height;
    }

    float IndexToX(int index) const {
        int total_slots = (end_index - start_index) + future_bars;
        if (total_slots <= 0) return left;
        float step = width / (float)total_slots;
        return left + (index - start_index) * step + step * 0.5f;
    }
};

struct StrategyConfig {
    std::wstring strategy_name = L"ma cross";
    int position_type = 0;
    int qty = 10;
    int amount = 1000000;
    bool is_qty_based = true;
    float fee_pct = 0.015f;
    float slip_pct = 0.05f;

    // 강제 청산 6종 옵션
    bool chk_stop_loss = true;   float val_stop_loss = 2.0f;
    bool chk_trailing = true;    float val_trailing = 60.0f; float val_trail_start = 2.0f;
    bool chk_protect = true;     float val_protect = 2.5f;
    bool chk_target = true;      float val_target = 10.0f;
    bool chk_min_change = false; float val_min_change = 2.0f; int val_min_bars = 10;
    bool chk_day_exit = false;   std::wstring val_day_time = L"02:30";

    COLORREF colors[6] = {
        RGB(0, 162, 232),   // 0. 최대허용손실 (시안)
        RGB(196, 156, 0),   // 1. 최대수익대비하락 (골드)
        RGB(237, 28, 36),   // 2. 최고/최저가 이익보존율 (적색)
        RGB(255, 127, 39),  // 3. 목표수익 (주황)
        RGB(0, 84, 166),    // 4. 최소가격변화 (청색)
        RGB(34, 177, 76)    // 5. 당일청산 (녹색)
    };

    void SaveToFile(const std::wstring& fname) const {
        std::wofstream out(fname);
        if (!out.is_open()) return;
        out << L"{\n";
        out << L"  \"val_stop_loss\": " << val_stop_loss << L",\n";
        out << L"  \"chk_stop_loss\": " << (chk_stop_loss ? 1 : 0) << L",\n";
        out << L"  \"val_trailing\": " << val_trailing << L",\n";
        out << L"  \"val_trail_start\": " << val_trail_start << L",\n";
        out << L"  \"chk_trailing\": " << (chk_trailing ? 1 : 0) << L",\n";
        out << L"  \"val_protect\": " << val_protect << L",\n";
        out << L"  \"chk_protect\": " << (chk_protect ? 1 : 0) << L",\n";
        out << L"  \"val_target\": " << val_target << L",\n";
        out << L"  \"chk_target\": " << (chk_target ? 1 : 0) << L",\n";
        out << L"  \"colors\": [";
        for (int i = 0; i < 6; ++i) out << (uint32_t)colors[i] << (i < 5 ? L", " : L"");
        out << L"]\n}\n";
    }

    void LoadFromFile(const std::wstring& fname) {
        std::wifstream in(fname);
        if (!in.is_open()) return;
        std::wstring line;
        while (std::getline(in, line)) {
            auto parse_val = [&](const std::wstring& key, float& dest) {
                if (line.find(key) != std::wstring::npos) {
                    size_t c = line.find(L':');
                    dest = (float)_wtof(line.substr(c + 1).c_str());
                }
            };
            auto parse_chk = [&](const std::wstring& key, bool& dest) {
                if (line.find(key) != std::wstring::npos) {
                    size_t c = line.find(L':');
                    dest = (_wtoi(line.substr(c + 1).c_str()) == 1);
                }
            };
            parse_val(L"\"val_stop_loss\":", val_stop_loss);
            parse_chk(L"\"chk_stop_loss\":", chk_stop_loss);
            parse_val(L"\"val_trailing\":", val_trailing);
            parse_val(L"\"val_trail_start\":", val_trail_start);
            parse_chk(L"\"chk_trailing\":", chk_trailing);
            parse_val(L"\"val_protect\":", val_protect);
            parse_chk(L"\"chk_protect\":", chk_protect);
            parse_val(L"\"val_target\":", val_target);
            parse_chk(L"\"chk_target\":", chk_target);
        }
    }
};

class IChartAddon {
public:
    virtual ~IChartAddon() = default;
    virtual std::wstring GetId() const = 0;
    virtual std::wstring GetName() const = 0;
    virtual void OnUpdate(const std::vector<Candle>& candles) = 0;
    virtual void OnRender(ID2D1HwndRenderTarget* pRT, const ViewportTransform& vp, IDWriteTextFormat* pFont) = 0;
    virtual bool IsEnabled() const = 0;
    virtual void SetEnabled(bool b) = 0;
};

class DynamicTradingStrategyAddon : public IChartAddon {
public:
    bool m_autoOrderEnabled = false;
    uint32_t m_defaultQty = 10;
    void SetAutoOrder(bool enable, uint32_t qty = 10) { m_autoOrderEnabled = enable; m_defaultQty = qty; }
    bool IsAutoOrderEnabled() const { return m_autoOrderEnabled; }
    uint32_t GetDefaultQty() const { return m_defaultQty; }
    StrategyConfig cfg;
    int p1 = 5, p2 = 20;
    bool enabled = true;

    struct SignalItem {
        int idx;
        bool is_buy;
        float price;
        std::wstring label;
        D2D1_COLOR_F color;
    };
    std::vector<SignalItem> signals;

    DynamicTradingStrategyAddon(const StrategyConfig& c, int fast_p = 5, int slow_p = 20)
        : cfg(c), p1(fast_p), p2(slow_p) {}

    std::wstring GetId() const override { return L"DYNAMIC_STRATEGY"; }
    std::wstring GetName() const override { return cfg.strategy_name; }
    bool IsEnabled() const override { return enabled; }
    void SetEnabled(bool b) override { enabled = b; }

    static D2D1_COLOR_F ConvertColor(COLORREF cr) {
        return D2D1::ColorF(GetRValue(cr) / 255.0f, GetGValue(cr) / 255.0f, GetBValue(cr) / 255.0f);
    }

    void OnUpdate(const std::vector<Candle>& candles) override {
        signals.clear();
        if (!enabled || candles.size() < (size_t)p2) return;

        auto ma = [&candles](int idx, int p) {
            float s = 0.0f;
            for (int k = idx - p + 1; k <= idx; ++k) s += candles[k].close;
            return s / (float)p;
        };

        bool in_position = false;
        float entry_price = 0.0f;
        float highest_since_entry = 0.0f;

        for (int i = p2; i < (int)candles.size(); ++i) {
            float pf = ma(i - 1, p1), ps = ma(i - 1, p2);
            float cf = ma(i, p1),     cs = ma(i, p2);

            // [매수 진입] 골든크로스
            if (!in_position && pf <= ps && cf > cs) {
                in_position = true;
                entry_price = candles[i].close;
                highest_since_entry = candles[i].high;
                signals.push_back({ i, true, candles[i].low * 0.998f, L"매수진입", D2D1::ColorF(1.0f, 0.15f, 0.15f) });
            }
            // [포지션 보유 중 강제 청산 판별]
            else if (in_position) {
                if (candles[i].high > highest_since_entry) {
                    highest_since_entry = candles[i].high;
                }

                float max_gain_pct = ((highest_since_entry - entry_price) / entry_price) * 100.0f;

                // 1. 최대허용손실 (Stop Loss)
                float stop_price = entry_price * (1.0f - cfg.val_stop_loss / 100.0f);
                bool hit_stop = cfg.chk_stop_loss && (candles[i].low <= stop_price);

                // 2. 최대수익대비하락 (Trailing Stop)
                bool hit_trailing = false;
                if (cfg.chk_trailing && max_gain_pct >= cfg.val_trail_start) {
                    float trail_cut_price = highest_since_entry - (highest_since_entry - entry_price) * (cfg.val_trailing / 100.0f);
                    if (candles[i].low <= trail_cut_price) hit_trailing = true;
                }

                // 3. 최고/최저가 이익보존율 (Profit Protect)
                bool hit_protect = false;
                if (cfg.chk_protect && max_gain_pct >= cfg.val_protect) {
                    float protect_price = entry_price * (1.0f + (cfg.val_protect * 0.5f) / 100.0f);
                    if (candles[i].low <= protect_price) hit_protect = true;
                }

                // 4. 목표수익 달성 (Target Profit)
                float target_price = entry_price * (1.0f + cfg.val_target / 100.0f);
                bool hit_target = cfg.chk_target && (candles[i].high >= target_price);

                // 5. 일반 이동평균 데드크로스 청산
                bool hit_dead = (pf >= ps && cf < cs);

                if (hit_stop) {
                    in_position = false;
                    signals.push_back({ i, false, candles[i].high * 1.002f, L"손절청산", ConvertColor(cfg.colors[0]) });
                } else if (hit_trailing) {
                    in_position = false;
                    signals.push_back({ i, false, candles[i].high * 1.002f, L"트레일링청산", ConvertColor(cfg.colors[1]) });
                } else if (hit_protect) {
                    in_position = false;
                    signals.push_back({ i, false, candles[i].high * 1.002f, L"이익보존청산", ConvertColor(cfg.colors[2]) });
                } else if (hit_target) {
                    in_position = false;
                    signals.push_back({ i, false, candles[i].high * 1.002f, L"목표달성", ConvertColor(cfg.colors[3]) });
                } else if (hit_dead) {
                    in_position = false;
                    signals.push_back({ i, false, candles[i].high * 1.002f, L"매도청산", D2D1::ColorF(0.2f, 0.6f, 1.0f) });
                }
            }
        }
    }

    void OnRender(ID2D1HwndRenderTarget* pRT, const ViewportTransform& vp, IDWriteTextFormat* pFont) override {
        if (!enabled) return;

        for (const auto& s : signals) {
            if (s.idx < vp.start_index || s.idx >= vp.end_index) continue;
            float x = vp.IndexToX(s.idx);
            float y = vp.PriceToY(s.price);

            ID2D1SolidColorBrush* pBrush = nullptr;
            pRT->CreateSolidColorBrush(s.color, &pBrush);
            if (!pBrush) continue;

            if (s.is_buy) {
                // 매수 진입: ▲ 삼각 마커
                D2D1_POINT_2F pt1 = D2D1::Point2F(x, y - 9), pt2 = D2D1::Point2F(x - 5, y), pt3 = D2D1::Point2F(x + 5, y);
                pRT->DrawLine(pt1, pt2, pBrush, 2.0f);
                pRT->DrawLine(pt2, pt3, pBrush, 2.0f);
                pRT->DrawLine(pt3, pt1, pBrush, 2.0f);
                D2D1_RECT_F tr = D2D1::RectF(x - 20, y + 2, x + 40, y + 14);
                pRT->DrawText(s.label.c_str(), (UINT32)s.label.size(), pFont, tr, pBrush);
            } else {
                // 매도/강제 청산: ▼ 역삼각 마커 및 고유 색상 라벨
                D2D1_POINT_2F pt1 = D2D1::Point2F(x, y + 9), pt2 = D2D1::Point2F(x - 5, y), pt3 = D2D1::Point2F(x + 5, y);
                pRT->DrawLine(pt1, pt2, pBrush, 2.0f);
                pRT->DrawLine(pt2, pt3, pBrush, 2.0f);
                pRT->DrawLine(pt3, pt1, pBrush, 2.0f);
                D2D1_RECT_F tr = D2D1::RectF(x - 28, y - 14, x + 45, y - 2);
                pRT->DrawText(s.label.c_str(), (UINT32)s.label.size(), pFont, tr, pBrush);
            }
            pBrush->Release();
        }
    }
};



#pragma pack(push, 1)
struct RealTickPacket {
    wchar_t code[16];
    float price;
    uint64_t volume;
    wchar_t time[16];
};
#pragma pack(pop)

// -------------------------------------------------------------
// 키움 Open API+ 전담 패킷 규격 (주문 / 체결 / 실시간 조건검색)
// -------------------------------------------------------------


#pragma pack(push, 1)
enum class KiwoomOrderType : uint8_t {
    Buy = 1,       // 신규매수
    Sell = 2,      // 신규매도
    CancelBuy = 3, // 매수취소
    CancelSell = 4,// 매도취소
    ModifyBuy = 5, // 매수정정
    ModifySell = 6 // 매도정정
};

struct KiwoomOrderRequest {
    wchar_t accNo[16];      // 계좌번호
    KiwoomOrderType orderType;
    wchar_t code[16];       // 종목코드
    int32_t qty;            // 수량
    int32_t price;          // 가격 (시장가는 0)
    wchar_t hogaType[4];    // 00: 지정가, 03: 시장가 등
    wchar_t orgOrderNo[16]; // 원주문번호 (정정/취소 시)
};

struct KiwoomChejanPacket {
    wchar_t accNo[16];
    wchar_t orderNo[16];
    wchar_t code[16];
    wchar_t time[16];
    wchar_t status[16];     // 접수, 체결, 취소 등
    int32_t orderQty;
    int32_t orderPrice;
    int32_t filledQty;
    int32_t filledPrice;
    int32_t openQty;        // 미체결수량
};

struct KiwoomConditionRealPacket {
    int32_t conditionIndex;
    wchar_t conditionName[32];
    wchar_t code[16];
    char eventType;         // 'I': Insert(편입), 'D': Delete(이탈)
};
#pragma pack(pop)