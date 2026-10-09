#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#import "C:\DAISHIN\CYBOSPLUS\CpUtil.dll" no_namespace named_guids
#import "C:\DAISHIN\CYBOSPLUS\CpSysDib.dll" no_namespace named_guids

static const wchar_t* PIPE_NAME = L"\\\\.\\pipe\\GeminiBridgePipe";

struct SectorRankItem {
    int rank;
    std::string code;
    std::string name;
    double diff;
};

std::vector<SectorRankItem> g_topSectors;

bool CheckCybosConnection() {
    ICpCybosPtr pCybos;
    HRESULT hr = pCybos.CreateInstance(__uuidof(CpCybos));
    if (FAILED(hr)) {
        printf("[CYBOS] CpCybos CoCreateInstance FAILED (0x%08X)\n", (unsigned int)hr);
        return false;
    }

    short isConnect = pCybos->GetIsConnect();
    short serverType = pCybos->GetServerType();

    printf("[CYBOS] Connection Status : %d (%s)\n", 
           isConnect, (isConnect == 1 ? "CONNECTED (OK)" : "DISCONNECTED (Login Required)"));
    printf("[CYBOS] Server Type       : %d (%s)\n", 
           serverType, (serverType == 1 ? "CyBos Real Server" : "Simulation Server"));

    return (isConnect == 1);
}

bool FetchTopSectors() {
    g_topSectors.clear();
    try {
        ISysDibPtr pSectorRanking;
        HRESULT hr = pSectorRanking.CreateInstance(__uuidof(CpSvrNew7043));
        if (FAILED(hr)) {
            printf("[CYBOS] CpSvrNew7043 COM Instance creation FAILED.\n");
            return false;
        }

        // 전체 시장(0) 주도 업종 랭킹 조회
        pSectorRanking->SetInputValue(0, _variant_t((char)'0'));
        pSectorRanking->BlockRequest();

        short count = pSectorRanking->GetHeaderValue(0);
        printf("[CYBOS] CpSvrNew7043 Response: Count = %d items.\n", count);

        for (short i = 0; i < count; ++i) {
            _bstr_t bstrCode = pSectorRanking->GetDataValue(0, i);
            _bstr_t bstrName = pSectorRanking->GetDataValue(1, i);
            double diff = pSectorRanking->GetDataValue(3, i);

            SectorRankItem item;
            item.rank = (int)(i + 1);
            item.code = (const char*)bstrCode ? (const char*)bstrCode : "";
            item.name = (const char*)bstrName ? (const char*)bstrName : "";
            item.diff = diff;
            g_topSectors.push_back(item);
        }
        return true;
    }
    catch (_com_error& e) {
        printf("[CYBOS] CpSvrNew7043 COM Error: %s (0x%08X)\n", (const char*)e.Description(), e.Error());
        return false;
    }
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(CP_UTF8);

    printf("==================================================\n");
    printf("[BRIDGE-32] Gemini Cybos 32-bit IPC Broker Active\n");
    printf("==================================================\n");

    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        printf("[ERROR] CoInitialize FAILED (0x%08X)\n", (unsigned int)hr);
        return 1;
    }

    bool connected = CheckCybosConnection();
    if (connected) {
        FetchTopSectors();
    } else {
        printf("[WARN] Cybos Plus is not logged in or Admin privileges missing.\n");
    }

    printf("\n[PIPE] Creating Named Pipe: \\\\.\\pipe\\GeminiBridgePipe\n");
    HANDLE hPipe = CreateNamedPipeW(
        PIPE_NAME,
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        65536,
        65536,
        0,
        NULL
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        printf("[ERROR] CreateNamedPipe FAILED. Error Code: %lu\n", GetLastError());
        CoUninitialize();
        return 1;
    }

    printf("[PIPE] Waiting for 64-bit Main Engine connection...\n");
    BOOL clientConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

    if (clientConnected) {
        printf("[PIPE] 64-bit Main Engine Connected successfully.\n");
        
        // 1. 핸드셰이크 패킷 전송
        std::string initMsg = "CYBOS_BRIDGE_READY\n";
        DWORD bytesWritten = 0;
        WriteFile(hPipe, initMsg.c_str(), (DWORD)initMsg.size(), &bytesWritten, NULL);

        // 2. 수집된 업종 랭킹 데이터 일괄 전송
        std::stringstream ss;
        for (const auto& item : g_topSectors) {
            ss << "SECTOR_RANK|" << item.rank << "|" << item.code << "|" << item.name << "|" << item.diff << "\n";
        }
        std::string packetData = ss.str();
        if (!packetData.empty()) {
            WriteFile(hPipe, packetData.c_str(), (DWORD)packetData.size(), &bytesWritten, NULL);
            FlushFileBuffers(hPipe);
            printf("[PIPE] Sent %zu sector rank items to 64-bit Main Engine.\n", g_topSectors.size());
        }

        // 3. 메시지 대기 루프
        char buffer[1024];
        DWORD bytesRead = 0;
        while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            printf("[PIPE RECV] %s\n", buffer);
        }

        DisconnectNamedPipe(hPipe);
    }

    CloseHandle(hPipe);
    CoUninitialize();
    return 0;
}