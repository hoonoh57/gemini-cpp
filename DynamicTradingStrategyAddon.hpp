#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include "ChartTypes.hpp"
#include "CentralDataManager.hpp"

// 축 2 (Addon Engine): 시스템 매매 전략 및 자동 주문 라우팅 애드온
class DynamicTradingStrategyAddon {
private:
    std::wstring m_targetCode = L"005930";
    bool m_autoOrderEnabled = false;
    uint32_t m_defaultQty = 10;
    float m_lastSignalPrice = 0.0f;
    int m_lastSignal = 0; // 0: None, 1: Buy, 2: Sell

public:
    DynamicTradingStrategyAddon(const std::wstring& code = L"005930")
        : m_targetCode(code) {}

    void SetTargetCode(const std::wstring& code) {
        m_targetCode = code;
    }

    void SetAutoOrder(bool enable, uint32_t qty = 10) {
        m_autoOrderEnabled = enable;
        m_defaultQty = qty;
    }

    bool IsAutoOrderEnabled() const { return m_autoOrderEnabled; }
    uint32_t GetDefaultQty() const { return m_defaultQty; }
    int GetLastSignal() const { return m_lastSignal; }

    // 틱 또는 캔들 수신 시 전략 시그널 평가 및 키움 자동 주문 발주
    void OnEvaluateSignal(float currentPrice, float maFast, float maSlow) {
        if (currentPrice <= 0.0f || maFast <= 0.0f || maSlow <= 0.0f) return;

        // 골든크로스 매수 시그널 (단기 이평 상향 돌파)
        if (maFast > maSlow && m_lastSignal != 1) {
            m_lastSignal = 1;
            m_lastSignalPrice = currentPrice;
            if (m_autoOrderEnabled) {
                ExecuteOrder(KiwoomOrderType::BUY, currentPrice);
            }
        }
        // 데드크로스 매도 시그널 (단기 이평 하향 이탈)
        else if (maFast < maSlow && m_lastSignal != 2) {
            m_lastSignal = 2;
            m_lastSignalPrice = currentPrice;
            if (m_autoOrderEnabled) {
                ExecuteOrder(KiwoomOrderType::SELL, currentPrice);
            }
        }
    }

private:
    void ExecuteOrder(KiwoomOrderType type, float price) {
        KiwoomOrderRequest req{};
        req.orderType = type;
        wcsncpy_s(req.code, sizeof(req.code) / sizeof(wchar_t), m_targetCode.c_str(), _TRUNCATE);
        req.qty = m_defaultQty;
        req.price = 0; // 03: 시장가
        wcsncpy_s(req.hogaType, sizeof(req.hogaType) / sizeof(wchar_t), L"03", _TRUNCATE);

        CentralDataManager::Instance().SendKiwoomOrder(req);
    }
};
