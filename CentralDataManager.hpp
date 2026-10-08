#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <iostream>
#include "ChartTypes.hpp"

#pragma pack(push, 1)
struct PipeHeader {
    char magic[4];       // 'G','B','R','G'
    uint32_t msgType;    // 1: REQ_CANDLES, 2: RES_CANDLES, 99: ERROR
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
#pragma pack(pop)

class CentralDataManager {
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

    std::vector<Candle> GetCandles(const std::wstring& code) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_candleStore.find(code);
        if (it != m_candleStore.end()) return it->second;
        return {};
    }
};