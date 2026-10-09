#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <iostream>
#include "ChartTypes.hpp"



class CentralDataManager {
public:
    // 대신 Cybos Plus 전용 업종/섹터 랭킹 배치 다운로드 IPC (Strict Separation)
    std::vector<SectorRankingItem> RequestSectorRanking() {
        std::vector<SectorRankingItem> result;

        const wchar_t* PIPE_NAME = L"\\\\.\\pipe\\GeminiBridgePipe";
        HANDLE hPipe = CreateFileW(
            PIPE_NAME,
            GENERIC_READ | GENERIC_WRITE,
            0, nullptr, OPEN_EXISTING, 0, nullptr
        );

        if (hPipe == INVALID_HANDLE_VALUE) return result;

        PipeHeader reqHdr{};
        memcpy(reqHdr.magic, "GBRG", 4);
        reqHdr.msgType = 8; // REQ_SECTOR_RANK
        reqHdr.payloadLen = 0;

        DWORD written = 0;
        if (WriteFile(hPipe, &reqHdr, sizeof(reqHdr), &written, nullptr)) {
            PipeHeader resHdr{};
            DWORD readBytes = 0;
            if (ReadFile(hPipe, &resHdr, sizeof(resHdr), &readBytes, nullptr) &&
                resHdr.msgType == 9 && resHdr.payloadLen > 0) {
                
                uint32_t recCount = resHdr.payloadLen / sizeof(SectorRankingItem);
                result.resize(recCount);
                ReadFile(hPipe, result.data(), resHdr.payloadLen, &readBytes, nullptr);
            }
        }

        CloseHandle(hPipe);
        return result;
    }
    // 대신 Cybos Plus 전용 프로그램 매매 동향 배치 다운로드 IPC (Strict Separation)
    std::vector<ProgramTradeItem> RequestProgramTrade(const std::wstring& code, uint32_t count = 60) {
        std::vector<ProgramTradeItem> result;
        if (code.empty()) return result;

        const wchar_t* PIPE_NAME = L"\\\\.\\pipe\\GeminiBridgePipe";
        HANDLE hPipe = CreateFileW(
            PIPE_NAME,
            GENERIC_READ | GENERIC_WRITE,
            0, nullptr, OPEN_EXISTING, 0, nullptr
        );

        if (hPipe == INVALID_HANDLE_VALUE) return result;

        ProgramTradeRequestPacket req{};
        wcsncpy_s(req.code, sizeof(req.code) / sizeof(wchar_t), code.c_str(), _TRUNCATE);
        req.count = count;

        PipeHeader reqHdr{};
        memcpy(reqHdr.magic, "GBRG", 4);
        reqHdr.msgType = 6; // REQ_PROGRAM_TRADE
        reqHdr.payloadLen = sizeof(req);

        DWORD written = 0;
        if (WriteFile(hPipe, &reqHdr, sizeof(reqHdr), &written, nullptr) &&
            WriteFile(hPipe, &req, sizeof(req), &written, nullptr)) {
            
            PipeHeader resHdr{};
            DWORD readBytes = 0;
            if (ReadFile(hPipe, &resHdr, sizeof(resHdr), &readBytes, nullptr) &&
                resHdr.msgType == 7 && resHdr.payloadLen > 0) {
                
                uint32_t recCount = resHdr.payloadLen / sizeof(ProgramTradeItem);
                result.resize(recCount);
                ReadFile(hPipe, result.data(), resHdr.payloadLen, &readBytes, nullptr);
            }
        }

        CloseHandle(hPipe);
        return result;
    }
    // 대신 Cybos Plus 전용 다중 종목 배치 데이터 조회 IPC (Strict Separation)
    std::vector<MarketEyeItem> RequestMarketEye(const std::vector<std::wstring>& codes) {
        std::vector<MarketEyeItem> result;
        if (codes.empty()) return result;

        const wchar_t* PIPE_NAME = L"\\\\.\\pipe\\GeminiBridgePipe";
        HANDLE hPipe = CreateFileW(
            PIPE_NAME,
            GENERIC_READ | GENERIC_WRITE,
            0, nullptr, OPEN_EXISTING, 0, nullptr
        );

        if (hPipe == INVALID_HANDLE_VALUE) return result;

        MarketEyeRequestPacket req{};
        req.count = static_cast<uint32_t>(std::min<size_t>(codes.size(), 200));
        for (uint32_t i = 0; i < req.count; ++i) {
            wcsncpy_s(req.codes[i], codes[i].c_str(), _TRUNCATE);
        }

        PipeHeader reqHdr{};
        memcpy(reqHdr.magic, "GBRG", 4);
        reqHdr.msgType = 4; // REQ_MARKET_EYE
        reqHdr.payloadLen = sizeof(req);

        DWORD written = 0;
        if (WriteFile(hPipe, &reqHdr, sizeof(reqHdr), &written, nullptr) &&
            WriteFile(hPipe, &req, sizeof(req), &written, nullptr)) {
            
            PipeHeader resHdr{};
            DWORD readBytes = 0;
            if (ReadFile(hPipe, &resHdr, sizeof(resHdr), &readBytes, nullptr) &&
                resHdr.msgType == 5 && resHdr.payloadLen > 0) {
                
                uint32_t count = resHdr.payloadLen / sizeof(MarketEyeItem);
                result.resize(count);
                ReadFile(hPipe, result.data(), resHdr.payloadLen, &readBytes, nullptr);
            }
        }

        CloseHandle(hPipe);
        return result;
    }
    // 키움 체결/잔고 및 조건검색 리스너 인터페이스
    using ChejanNotifyCallback = std::function<void(const KiwoomChejanPacket&)>;
    using ConditionNotifyCallback = std::function<void(const KiwoomConditionRealPacket&)>;

    ChejanNotifyCallback m_chejanCallback = nullptr;
    ConditionNotifyCallback m_condCallback = nullptr;
    std::thread m_chejanListenerThread;
    std::thread m_condListenerThread;
    std::atomic<bool> m_stopChejanListener{false};
    std::atomic<bool> m_stopCondListener{false};

    void SetChejanCallback(ChejanNotifyCallback cb) { m_chejanCallback = cb; }
    void SetConditionCallback(ConditionNotifyCallback cb) { m_condCallback = cb; }

    void StartChejanListener() {
        m_stopChejanListener = false;
        m_chejanListenerThread = std::thread([this]() {
            const wchar_t* CHEJAN_PIPE = L"\\\\.\\pipe\\GeminiKiwoomChejanPipe";
            while (!m_stopChejanListener) {
                HANDLE hPipe = CreateFileW(CHEJAN_PIPE, GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
                if (hPipe == INVALID_HANDLE_VALUE) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    continue;
                }
                while (!m_stopChejanListener) {
                    PipeHeader hdr{};
                    DWORD readBytes = 0;
                    if (!ReadFile(hPipe, &hdr, sizeof(hdr), &readBytes, nullptr) || readBytes != sizeof(hdr)) break;
                    if (hdr.msgType == 11 && hdr.payloadLen == sizeof(KiwoomChejanPacket)) {
                        KiwoomChejanPacket packet{};
                        if (ReadFile(hPipe, &packet, sizeof(packet), &readBytes, nullptr) && readBytes == sizeof(packet)) {
                            if (m_chejanCallback) m_chejanCallback(packet);
                        }
                    } else {
                        std::vector<char> dummy(hdr.payloadLen);
                        ReadFile(hPipe, dummy.data(), hdr.payloadLen, &readBytes, nullptr);
                    }
                }
                CloseHandle(hPipe);
            }
        });
    }

    void StopChejanListener() {
        m_stopChejanListener = true;
        if (m_chejanListenerThread.joinable()) m_chejanListenerThread.detach();
    }

    void StartConditionListener() {
        m_stopCondListener = false;
        m_condListenerThread = std::thread([this]() {
            const wchar_t* COND_PIPE = L"\\\\.\\pipe\\GeminiKiwoomConditionPipe";
            while (!m_stopCondListener) {
                HANDLE hPipe = CreateFileW(COND_PIPE, GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
                if (hPipe == INVALID_HANDLE_VALUE) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    continue;
                }
                while (!m_stopCondListener) {
                    PipeHeader hdr{};
                    DWORD readBytes = 0;
                    if (!ReadFile(hPipe, &hdr, sizeof(hdr), &readBytes, nullptr) || readBytes != sizeof(hdr)) break;
                    if (hdr.msgType == 21 && hdr.payloadLen == sizeof(KiwoomConditionRealPacket)) {
                        KiwoomConditionRealPacket packet{};
                        if (ReadFile(hPipe, &packet, sizeof(packet), &readBytes, nullptr) && readBytes == sizeof(packet)) {
                            if (m_condCallback) m_condCallback(packet);
                        }
                    } else {
                        std::vector<char> dummy(hdr.payloadLen);
                        ReadFile(hPipe, dummy.data(), hdr.payloadLen, &readBytes, nullptr);
                    }
                }
                CloseHandle(hPipe);
            }
        });
    }

    void StopConditionListener() {
        m_stopCondListener = true;
        if (m_condListenerThread.joinable()) m_condListenerThread.detach();
    }
public:
    // 키움 Open API+ 전용 주문 전송 IPC (Strict Separation: Kiwoom handles trading)
    long SendKiwoomOrder(const KiwoomOrderRequest& req) {
        const wchar_t* ORDER_PIPE = L"\\\\.\\pipe\\GeminiKiwoomOrderPipe";
        HANDLE hPipe = CreateFileW(
            ORDER_PIPE,
            GENERIC_READ | GENERIC_WRITE,
            0, nullptr, OPEN_EXISTING, 0, nullptr
        );

        if (hPipe == INVALID_HANDLE_VALUE) {
            return -100; // 브릿지 미연결 (Fail-Fast)
        }

        DWORD written = 0, readBytes = 0;
        long retCode = -1;

        if (WriteFile(hPipe, &req, sizeof(req), &written, nullptr) && written == sizeof(req)) {
            ReadFile(hPipe, &retCode, sizeof(retCode), &readBytes, nullptr);
        }

        CloseHandle(hPipe);
        return retCode;
    }
public:
    using TickNotifyCallback = std::function<void(const std::wstring&)>;
    TickNotifyCallback m_tickCallback = nullptr;

        std::thread m_realListenerThread;
    std::atomic<bool> m_stopRealListener{false};

    void StartRealTimeListener() {
        m_stopRealListener = false;
        m_realListenerThread = std::thread([this]() {
            const wchar_t* REAL_PIPE = L"\\\\.\\pipe\\GeminiBridgeRealPipe";
            while (!m_stopRealListener) {
                HANDLE hPipe = CreateFileW(
                    REAL_PIPE,
                    GENERIC_READ,
                    0, nullptr, OPEN_EXISTING, 0, nullptr
                );

                if (hPipe == INVALID_HANDLE_VALUE) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    continue;
                }

                while (!m_stopRealListener) {
                    PipeHeader hdr{};
                    DWORD readBytes = 0;
                    if (!ReadFile(hPipe, &hdr, sizeof(hdr), &readBytes, nullptr) || readBytes != sizeof(hdr)) {
                        break;
                    }

                    if (hdr.msgType == 3 && hdr.payloadLen == sizeof(RealTickPacket)) {
                        RealTickPacket tick{};
                        if (ReadFile(hPipe, &tick, sizeof(tick), &readBytes, nullptr) && readBytes == sizeof(tick)) {
                            OnReceiveRealTick(tick);
                        }
                    } else {
                        std::vector<char> dummy(hdr.payloadLen);
                        ReadFile(hPipe, dummy.data(), hdr.payloadLen, &readBytes, nullptr);
                    }
                }
                CloseHandle(hPipe);
            }
        });
    }

    void StopRealTimeListener() {
        m_stopRealListener = true;
        if (m_realListenerThread.joinable()) {
            m_realListenerThread.detach();
        }
    }
    void SetTickCallback(TickNotifyCallback cb) {
        m_tickCallback = cb;
    }
private:
    std::mutex m_mutex;
    std::unordered_map<std::wstring, StockMaster> m_masters;
    std::unordered_map<std::wstring, std::vector<Candle>> m_candleStore;

    CentralDataManager() {}

public:
    static CentralDataManager& Instance() {
        static CentralDataManager instance;
        return instance;
    }

    bool LoadInitialData(const std::wstring& binPath, char tfType = 'm', int tfUnit = 1) {
        std::lock_guard<std::mutex> lock(m_mutex);
        FILE* fp = nullptr;
        _wfopen_s(&fp, binPath.c_str(), L"rb");
        if (!fp) {
            std::wcout << L"[CDM][WARN] Failed to open " << binPath << L". Please launch CybosBridge32 to fetch live data." << std::endl;
            return false;
        }

        m_masters.clear();
        m_candleStore.clear();

        uint32_t stockCount = 0;
        if (fread(&stockCount, sizeof(uint32_t), 1, fp) != 1) {
            fclose(fp);
            return false;
        }

        for (uint32_t i = 0; i < stockCount; ++i) {
            StockMaster master{};
            fread(&master, sizeof(StockMaster), 1, fp);
            m_masters[master.code] = master;

            uint32_t candleCount = 0;
            fread(&candleCount, sizeof(uint32_t), 1, fp);
            std::vector<Candle> candles(candleCount);
            if (candleCount > 0) {
                fread(candles.data(), sizeof(Candle), candleCount, fp);
            }
            m_candleStore[master.code] = std::move(candles);
        }

        fclose(fp);
        return true;
    }

    bool RequestDataFromBridge(const std::wstring& code, char tfType, int tfUnit, int count) {
        const wchar_t* PIPE_NAME = L"\\\\.\\pipe\\GeminiBridgePipe";
        HANDLE hPipe = CreateFileW(
            PIPE_NAME,
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );

        if (hPipe == INVALID_HANDLE_VALUE) {
            std::wcout << L"[CDM][ERROR] CybosBridge32 Named Pipe connection failed. Error: " << GetLastError() << std::endl;
            return false;
        }

        std::wstring reqPayload = code + L"|" + std::wstring(1, (wchar_t)tfType) + L"|" + std::to_wstring(tfUnit) + L"|" + std::to_wstring(count);
        uint32_t payloadBytes = static_cast<uint32_t>(reqPayload.size() * sizeof(wchar_t));

        PipeHeader reqHdr;
        memcpy(reqHdr.magic, "GBRG", 4);
        reqHdr.msgType = 1;
        reqHdr.payloadLen = payloadBytes;

        DWORD written = 0;
        if (!WriteFile(hPipe, &reqHdr, sizeof(reqHdr), &written, nullptr) ||
            !WriteFile(hPipe, reqPayload.c_str(), payloadBytes, &written, nullptr)) {
            CloseHandle(hPipe);
            return false;
        }

        PipeHeader resHdr{};
        DWORD readBytes = 0;
        if (!ReadFile(hPipe, &resHdr, sizeof(resHdr), &readBytes, nullptr) || readBytes != sizeof(resHdr)) {
            CloseHandle(hPipe);
            return false;
        }

        if (resHdr.msgType != 2 || resHdr.payloadLen == 0) {
            CloseHandle(hPipe);
            return false;
        }

        uint32_t candleCount = resHdr.payloadLen / sizeof(BridgeCandle);
        std::vector<BridgeCandle> bridgeCandles(candleCount);
        if (!ReadFile(hPipe, bridgeCandles.data(), resHdr.payloadLen, &readBytes, nullptr)) {
            CloseHandle(hPipe);
            return false;
        }

        CloseHandle(hPipe);

        std::vector<Candle> domainCandles;
        domainCandles.reserve(candleCount);
        for (const auto& bc : bridgeCandles) {
            Candle c;
            c.date = bc.date;
            c.time = bc.time;
            c.open = bc.open;
            c.high = bc.high;
            c.low = bc.low;
            c.close = bc.close;
            c.volume = bc.volume;
            c.ofi = bc.ofi;
            domainCandles.push_back(c);
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_candleStore[code] = std::move(domainCandles);
        }
        return true;
    }

    StockMaster GetMaster(const std::wstring& code) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_masters.find(code);
        if (it != m_masters.end()) return it->second;
        StockMaster m{};
        m.code = code;
        m.name = code;
        return m;
    }

        void OnReceiveRealTick(const RealTickPacket& tick) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_candleStore.find(tick.code);
        if (it != m_candleStore.end() && !it->second.empty()) {
            auto& last = it->second.back();
            last.close = tick.price;
            if (tick.price > last.high) last.high = tick.price;
            if (tick.price < last.low) last.low = tick.price;
            last.volume += tick.volume;
        }
        if (m_tickCallback) {
            m_tickCallback(tick.code);
        }
    }
    std::vector<Candle> GetCandles(const std::wstring& code) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_candleStore.find(code);
        if (it != m_candleStore.end()) return it->second;
        return {};
    }
};