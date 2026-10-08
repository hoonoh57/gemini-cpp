#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>

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

const wchar_t* PIPE_NAME = L"\\\\.\\pipe\\GeminiBridgePipe";

int wmain(int argc, wchar_t* argv[]) {
    std::wcout << L"[BRIDGE-32] Gemini 32-bit Cybos/Kiwoom IPC Bridge Broker Starting..." << std::endl;

    HANDLE hPipe = CreateNamedPipeW(
        PIPE_NAME,
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        65536,
        65536,
        0,
        nullptr
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::wcout << L"[BRIDGE-32][FATAL] Failed to create Named Pipe. Error: " << GetLastError() << std::endl;
        return 1;
    }

    std::wcout << L"[BRIDGE-32] Named Pipe Created: " << PIPE_NAME << std::endl;
    std::wcout << L"[BRIDGE-32] Waiting for 64-bit Main Engine connection..." << std::endl;

    while (true) {
        BOOL connected = ConnectNamedPipe(hPipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (!connected) {
            CloseHandle(hPipe);
            return 1;
        }

        std::wcout << L"[BRIDGE-32] Main Engine connected." << std::endl;

        PipeHeader reqHdr{};
        DWORD bytesRead = 0;
        BOOL ok = ReadFile(hPipe, &reqHdr, sizeof(reqHdr), &bytesRead, nullptr);
        if (ok && bytesRead == sizeof(reqHdr)) {
            std::vector<char> payload(reqHdr.payloadLen + 1, 0);
            if (reqHdr.payloadLen > 0) {
                ReadFile(hPipe, payload.data(), reqHdr.payloadLen, &bytesRead, nullptr);
            }

            std::wstring reqStr(reinterpret_cast<wchar_t*>(payload.data()), reqHdr.payloadLen / sizeof(wchar_t));
            std::wcout << L"[BRIDGE-32] Received Request: " << reqStr << std::endl;
        }

        DisconnectNamedPipe(hPipe);
    }

    CloseHandle(hPipe);
    return 0;
}