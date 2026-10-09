#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <functional>
#include <sstream>
#include <iostream>
#include <algorithm>
#include "ChartTypes.hpp"

class CentralDataManager {
public:
    using ChejanCallback = std::function<void(const KiwoomChejanPacket&)>;
    using ConditionCallback = std::function<void(const KiwoomConditionRealPacket&)>;
    using TickCallback = std::function<void(const std::wstring&)>;

private:
    std::map<std::wstring, StockMaster> m_masters;
    std::map<std::wstring, std::vector<Candle>> m_candles;
    std::vector<SectorRankingItem> m_sectorRanks;
    std::vector<MarketEyeItem> m_marketEyeItems;
    std::vector<ProgramTradeItem> m_progTradeItems;

    std::mutex m_dataMutex;
    std::mutex m_sectorMutex;

    ChejanCallback m_chejanCb;
    ConditionCallback m_conditionCb;
    TickCallback m_tickCb;

    std::thread m_pipeWorker;
    bool m_bRunning = false;

    CentralDataManager() = default;
    ~CentralDataManager() {
        StopRealTimeListener();
    }

    void EnsureSymbolData(const std::wstring& code) {
        if (m_masters.find(code) == m_masters.end()) {
            StockMaster m{};
            m.code = code;
            if (code.find(L"005930") != std::wstring::npos) m.name = L"삼성전자";
            else if (code.find(L"000660") != std::wstring::npos) m.name = L"SK하이닉스";
            else if (code.find(L"028050") != std::wstring::npos) m.name = L"삼성E&A";
            else if (code.find(L"047810") != std::wstring::npos) m.name = L"한국항공우주";
            else m.name = L"주도종목";

            m.cur_price = 70500.0f;
            m.change_val = 1000.0f;
            m.change_rate = 1.45f;
            m.bid_price = 70400.0f;
            m.ask_price = 70500.0f;
            m_masters[code] = m;
        }

        if (m_candles[code].empty()) {
            std::vector<Candle> list;
            float basePrice = 70000.0f;
            for (int i = 0; i < 120; ++i) {
                Candle c{};
                c.date = L"20261009";
                float delta = (float)((rand() % 800) - 400);
                c.open = basePrice + delta;
                c.high = c.open + (float)(rand() % 500);
                c.low = c.open - (float)(rand() % 500);
                c.close = (c.high + c.low) / 2.0f;
                c.volume = (uint64_t)(10000 + (rand() % 50000));
                basePrice = c.close;
                list.push_back(c);
            }
            m_candles[code] = list;
        }
    }

public:
    static CentralDataManager& Instance() {
        static CentralDataManager s_instance;
        return s_instance;
    }

    bool LoadInitialData(const std::wstring& pathOrCode, char timeframe = 'D', int count = 120) {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        const std::wstring defaultSymbols[] = { L"A000660", L"A005930", L"A028050", L"A047810" };
        for (const auto& sym : defaultSymbols) {
            EnsureSymbolData(sym);
        }
        if (pathOrCode.find(L".bin") == std::wstring::npos) {
            EnsureSymbolData(pathOrCode);
        }
        return true;
    }

    bool RequestDataFromBridge(const std::wstring& code, char timeframe = 'D', int count = 120, int slot = 0) {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        EnsureSymbolData(code);
        return true;
    }

    const StockMaster& GetMaster(const std::wstring& code) {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        EnsureSymbolData(code);
        return m_masters[code];
    }

    const std::vector<Candle>& GetCandles(const std::wstring& code) {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        EnsureSymbolData(code);
        return m_candles[code];
    }

    void SetChejanCallback(ChejanCallback cb) { m_chejanCb = cb; }
    void SetConditionCallback(ConditionCallback cb) { m_conditionCb = cb; }
    void SetTickCallback(TickCallback cb) { m_tickCb = cb; }

    void StartRealTimeListener() { StartPipeListener(); }
    void StopRealTimeListener() { StopPipeListener(); }
    void StartChejanListener() {}
    void StartConditionListener() {}

    std::vector<MarketEyeItem> RequestMarketEye(const std::vector<std::wstring>& codes) {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        return m_marketEyeItems;
    }

    std::vector<ProgramTradeItem> RequestProgramTrade(const std::wstring& code, int count = 60) {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        return m_progTradeItems;
    }

    std::vector<SectorRankingItem> RequestSectorRanking() {
        std::lock_guard<std::mutex> lock(m_sectorMutex);
        return m_sectorRanks;
    }

    bool SendKiwoomOrder(const KiwoomOrderRequest& req) {
        std::wcout << L"[ORDER] Kiwoom SendOrder: Code=" << req.code 
                   << L", Qty=" << req.qty << L", Price=" << req.price << std::endl;
        return true;
    }

    void StartPipeListener() {
        if (m_bRunning) return;
        m_bRunning = true;
        m_pipeWorker = std::thread(&CentralDataManager::PipeWorkerProc, this);
    }

    void StopPipeListener() {
        m_bRunning = false;
        if (m_pipeWorker.joinable()) {
            m_pipeWorker.detach();
        }
    }

private:
    void ParsePacketLine(const std::string& line) {
        if (line.rfind("SECTOR_RANK|", 0) == 0) {
            std::stringstream ss(line);
            std::string tag, sRank, code, name, sDiff;
            std::getline(ss, tag, '|');
            std::getline(ss, sRank, '|');
            std::getline(ss, code, '|');
            std::getline(ss, name, '|');
            std::getline(ss, sDiff, '|');

            SectorRankingItem item{};
            try {
                std::wstring wCode(code.begin(), code.end());
                wcsncpy_s(item.code, wCode.c_str(), _TRUNCATE);

                wchar_t wNameBuf[32] = {0};
                MultiByteToWideChar(CP_UTF8, 0, name.c_str(), -1, wNameBuf, 31);
                wcsncpy_s(item.name, wNameBuf, _TRUNCATE);

                item.diffRate = std::stof(sDiff);

                std::lock_guard<std::mutex> lock(m_sectorMutex);
                m_sectorRanks.push_back(item);
            } catch (...) {}
        }
    }

    void PipeWorkerProc() {
        const wchar_t* pipeName = L"\\\\.\\pipe\\GeminiBridgePipe";
        HANDLE hPipe = INVALID_HANDLE_VALUE;

        for (int i = 0; i < 20 && m_bRunning; ++i) {
            hPipe = CreateFileW(
                pipeName,
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
            );
            if (hPipe != INVALID_HANDLE_VALUE) break;
            Sleep(500);
        }

        if (hPipe == INVALID_HANDLE_VALUE) {
            return;
        }

        char buffer[4096];
        DWORD bytesRead = 0;
        std::string lineAcc = "";

        {
            std::lock_guard<std::mutex> lock(m_sectorMutex);
            m_sectorRanks.clear();
        }

        while (m_bRunning && ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            lineAcc += buffer;

            size_t pos = 0;
            while ((pos = lineAcc.find('\n')) != std::string::npos) {
                std::string line = lineAcc.substr(0, pos);
                if (!line.empty() && line.back() == '\r') line.pop_back();
                ParsePacketLine(line);
                lineAcc.erase(0, pos + 1);
            }
        }

        CloseHandle(hPipe);
    }
};