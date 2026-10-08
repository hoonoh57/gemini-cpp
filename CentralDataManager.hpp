#pragma once
#include "ChartTypes.hpp"
#include <map>
#include <fstream>

class CentralDataManager {
private:
    std::map<std::wstring, StockMaster> masters_;
    std::map<std::wstring, std::vector<Candle>> candle_db_;
public:
    static CentralDataManager& Instance() {
        static CentralDataManager inst;
        return inst;
    }

    void UpsertMaster(const StockMaster& m) { masters_[m.code] = m; }
    StockMaster GetMaster(const std::wstring& code) { return masters_[code]; }
    void SetCandles(const std::wstring& code, const std::vector<Candle>& list) { candle_db_[code] = list; }
    const std::vector<Candle>& GetCandles(const std::wstring& code) { return candle_db_[code]; }

    void LoadInitialData(const wchar_t* path, char tf_type, int tf_unit) {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) return;

        uint32_t slot_cnt = 0; file.read((char*)&slot_cnt, 4);
        if (slot_cnt == 0) return;

        for (uint32_t s = 0; s < slot_cnt; ++s) {
            char code_buf[9] = { 0 }; file.read(code_buf, 8);
            uint32_t cnt = 0; file.read((char*)&cnt, 4);
            std::wstring code = std::wstring(code_buf, code_buf + strlen(code_buf));
            std::vector<Candle> clist;

            for (uint32_t i = 0; i < cnt; ++i) {
                uint32_t dt = 0, tm = 0; float o, h, l, c; uint64_t v;
                file.read((char*)&dt, 4); file.read((char*)&tm, 4);
                file.read((char*)&o, 4); file.read((char*)&h, 4);
                file.read((char*)&l, 4); file.read((char*)&c, 4);
                file.read((char*)&v, 8);
                wchar_t tbuf[16];
                if (tf_type == 'D') swprintf_s(tbuf, L"%02d/%02d", (dt % 10000) / 100, dt % 100);
                else swprintf_s(tbuf, L"%02d:%02d", tm / 100, tm % 100);
                clist.push_back({ std::to_wstring(dt), tbuf, o, h, l, c, v, 0.0f });
            }
            SetCandles(code, clist);

            StockMaster sm; sm.code = code;
            if (code.find(L"000660") != std::wstring::npos) sm.name = L"SK하이닉스";
            else if (code.find(L"005930") != std::wstring::npos) sm.name = L"삼성전자";
            else if (code.find(L"028050") != std::wstring::npos) sm.name = L"삼성E&A";
            else if (code.find(L"047810") != std::wstring::npos) sm.name = L"한미글로벌";
            else sm.name = L"종목";

            if (!clist.empty()) {
                sm.cur_price = clist.back().close;
                float pc = (clist.size() > 1) ? clist[clist.size() - 2].close : clist.back().open;
                sm.change_val = sm.cur_price - pc;
                sm.change_rate = (sm.change_val / pc) * 100.0f;
            }
            UpsertMaster(sm);
        }
    }
};