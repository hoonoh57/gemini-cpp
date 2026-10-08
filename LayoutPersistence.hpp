#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include "ChartTypes.hpp"

struct SlotPersistData {
    std::wstring code;
    char tf_type = 'm';
    int tf_unit = 1;
    int visible_bars = 60;
    int scroll_offset = 0;
};

struct LayoutPersistData {
    int layout_mode = 4; // 4: 2x2, 1: 1x1
    std::vector<SlotPersistData> slots;
};

class LayoutPersistence {
public:
    static bool SaveToFile(const std::wstring& filePath, const LayoutPersistData& data) {
        std::wofstream ofs(filePath);
        if (!ofs.is_open()) return false;

        ofs << L"{\n";
        ofs << L"  \"layout_mode\": " << data.layout_mode << L",\n";
        ofs << L"  \"slots\": [\n";

        for (size_t i = 0; i < data.slots.size(); ++i) {
            const auto& s = data.slots[i];
            ofs << L"    {\n";
            ofs << L"      \"code\": \"" << s.code << L"\",\n";
            ofs << L"      \"tf_type\": \"" << (wchar_t)s.tf_type << L"\",\n";
            ofs << L"      \"tf_unit\": " << s.tf_unit << L",\n";
            ofs << L"      \"visible_bars\": " << s.visible_bars << L",\n";
            ofs << L"      \"scroll_offset\": " << s.scroll_offset << L"\n";
            ofs << L"    }" << (i + 1 < data.slots.size() ? L"," : L"") << L"\n";
        }

        ofs << L"  ]\n";
        ofs << L"}\n";
        return true;
    }

    static bool LoadFromFile(const std::wstring& filePath, LayoutPersistData& outData) {
        std::wifstream ifs(filePath);
        if (!ifs.is_open()) return false;

        std::wstringstream buffer;
        buffer << ifs.rdbuf();
        std::wstring json = buffer.str();

        outData.slots.clear();
        outData.layout_mode = 4;

        size_t modePos = json.find(L"\"layout_mode\": ");
        if (modePos != std::wstring::npos) {
            outData.layout_mode = _wtoi(&json[modePos + 15]);
        }

        size_t pos = 0;
        while ((pos = json.find(L"\"code\": \"", pos)) != std::wstring::npos) {
            pos += 9;
            size_t endPos = json.find(L"\"", pos);
            if (endPos == std::wstring::npos) break;

            SlotPersistData s;
            s.code = json.substr(pos, endPos - pos);

            size_t tfPos = json.find(L"\"tf_type\": \"", endPos);
            if (tfPos != std::wstring::npos) {
                s.tf_type = (char)json[tfPos + 12];
            }

            size_t unitPos = json.find(L"\"tf_unit\": ", endPos);
            if (unitPos != std::wstring::npos) {
                s.tf_unit = _wtoi(&json[unitPos + 11]);
            }

            size_t barsPos = json.find(L"\"visible_bars\": ", endPos);
            if (barsPos != std::wstring::npos) {
                s.visible_bars = _wtoi(&json[barsPos + 16]);
            }

            size_t scrollPos = json.find(L"\"scroll_offset\": ", endPos);
            if (scrollPos != std::wstring::npos) {
                s.scroll_offset = _wtoi(&json[scrollPos + 17]);
            }

            outData.slots.push_back(s);
            pos = endPos;
        }

        return !outData.slots.empty();
    }
};