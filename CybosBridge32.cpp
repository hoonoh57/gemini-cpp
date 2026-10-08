#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <comdef.h>

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

// Cybos Plus COM CLSID / ProgID
// CpUtil.CpCybos: 연결 상태 확인
// CpSysDib.StockChart: 차트 TR 조회
bool FetchCybosCandles(const std::wstring& code, char tfType, int tfUnit, int count, std::vector<BridgeCandle>& outCandles, std::wstring& outErrMsg) {
    CLSID clsidCybos, clsidChart;
    if (FAILED(CLSIDFromProgID(L"CpUtil.CpCybos", &clsidCybos))) {
        outErrMsg = L"Cybos Plus COM (CpUtil.CpCybos) not registered.";
        return false;
    }
    if (FAILED(CLSIDFromProgID(L"CpSysDib.StockChart", &clsidChart))) {
        outErrMsg = L"Cybos Plus COM (CpSysDib.StockChart) not registered.";
        return false;
    }

    IDispatch* pCybos = nullptr;
    if (FAILED(CoCreateInstance(clsidCybos, nullptr, CLSCTX_INPROC_SERVER, IID_IDispatch, (void**)&pCybos))) {
        outErrMsg = L"Failed to create CpUtil.CpCybos instance.";
        return false;
    }

    // IsConnect 상태 확인 (DISPID 1 또는 Invoke)
    DISPID dispidConnect;
    OLECHAR* nameConnect = (OLECHAR*)L"IsConnect";
    long isConnect = 0;
    if (SUCCEEDED(pCybos->GetIDsOfNames(IID_NULL, &nameConnect, 1, LOCALE_USER_DEFAULT, &dispidConnect))) {
        DISPPARAMS params{ nullptr, nullptr, 0, 0 };
        VARIANT res;
        VariantInit(&res);
        if (SUCCEEDED(pCybos->Invoke(dispidConnect, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &params, &res, nullptr, nullptr))) {
            isConnect = res.lVal;
            VariantClear(&res);
        }
    }
    pCybos->Release();

    if (isConnect != 1) {
        outErrMsg = L"Cybos Plus is not connected/logged in. Please start and log into Cybos Starter.";
        return false;
    }

    IDispatch* pChart = nullptr;
    if (FAILED(CoCreateInstance(clsidChart, nullptr, CLSCTX_INPROC_SERVER, IID_IDispatch, (void**)&pChart))) {
        outErrMsg = L"Failed to create CpSysDib.StockChart instance.";
        return false;
    }

    auto SetProp = [pChart](DISPID dispid, VARIANT val) {
        DISPID dispidNamed = DISPID_PROPERTYPUT;
        DISPPARAMS params{ &val, &dispidNamed, 1, 1 };
        return pChart->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT, &params, nullptr, nullptr, nullptr);
    };

    DISPID dispidSetInputValue, dispidBlockRequest, dispidGetHeaderValue, dispidGetDataValue;
    OLECHAR* n1 = (OLECHAR*)L"SetInputValue";
    OLECHAR* n2 = (OLECHAR*)L"BlockRequest";
    OLECHAR* n3 = (OLECHAR*)L"GetHeaderValue";
    OLECHAR* n4 = (OLECHAR*)L"GetDataValue";
    pChart->GetIDsOfNames(IID_NULL, &n1, 1, LOCALE_USER_DEFAULT, &dispidSetInputValue);
    pChart->GetIDsOfNames(IID_NULL, &n2, 1, LOCALE_USER_DEFAULT, &dispidBlockRequest);
    pChart->GetIDsOfNames(IID_NULL, &n3, 1, LOCALE_USER_DEFAULT, &dispidGetHeaderValue);
    pChart->GetIDsOfNames(IID_NULL, &n4, 1, LOCALE_USER_DEFAULT, &dispidGetDataValue);

    auto CallSetInput = [pChart, dispidSetInputValue](int type, VARIANT val) {
        VARIANT args[2];
        VariantInit(&args[0]); args[0] = val; // value
        VariantInit(&args[1]); args[1].vt = VT_I4; args[1].lVal = type; // type
        DISPPARAMS params{ args, nullptr, 2, 0 };
        pChart->Invoke(dispidSetInputValue, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &params, nullptr, nullptr, nullptr);
    };

    // 0: 종목코드 (A 접두사 처리)
    std::wstring stockCode = (code.rfind(L"A", 0) == 0) ? code : (L"A" + code);
    VARIANT vCode; VariantInit(&vCode); vCode.vt = VT_BSTR; vCode.bstrVal = SysAllocString(stockCode.c_str());
    CallSetInput(0, vCode);
    SysFreeString(vCode.bstrVal);

    // 1: 조회구분 ('1': 기간, '2': 개수)
    VARIANT vReqType; VariantInit(&vReqType); vReqType.vt = VT_UI1; vReqType.bVal = '2';
    CallSetInput(1, vReqType);

    // 4: 요청개수
    VARIANT vCount; VariantInit(&vCount); vCount.vt = VT_I4; vCount.lVal = count;
    CallSetInput(4, vCount);

    // 5: 필드배열 (0: 날짜, 1: 시간, 2: 시가, 3: 고가, 4: 저가, 5: 종가, 8: 거래량)
    SAFEARRAYBOUND sab[1] = { { 7, 0 } };
    SAFEARRAY* psa = SafeArrayCreate(VT_VARIANT, 1, sab);
    long flds[] = { 0, 1, 2, 3, 4, 5, 8 };
    for (long i = 0; i < 7; ++i) {
        VARIANT vf; VariantInit(&vf); vf.vt = VT_I4; vf.lVal = flds[i];
        SafeArrayPutElement(psa, &i, &vf);
    }
    VARIANT vArr; VariantInit(&vArr); vArr.vt = VT_ARRAY | VT_VARIANT; vArr.parray = psa;
    CallSetInput(5, vArr);
    SafeArrayDestroy(psa);

    // 6: 차트구분 ('m': 분봉, 'D': 일봉)
    VARIANT vChartType; VariantInit(&vChartType); vChartType.vt = VT_UI1; vChartType.bVal = (BYTE)tfType;
    CallSetInput(6, vChartType);

    // 7: 주기
    VARIANT vUnit; VariantInit(&vUnit); vUnit.vt = VT_I4; vUnit.lVal = tfUnit;
    CallSetInput(7, vUnit);

    // 10: 거래량구분 ('3': 시간외 미포함, '1': 일반)
    VARIANT vVolType; VariantInit(&vVolType); vVolType.vt = VT_UI1; vVolType.bVal = '1';
    CallSetInput(10, vVolType);

    // BlockRequest 호출
    DISPPARAMS noParams{ nullptr, nullptr, 0, 0 };
    pChart->Invoke(dispidBlockRequest, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &noParams, nullptr, nullptr, nullptr);

    // Header 3: 수신개수
    VARIANT vHdrIdx; VariantInit(&vHdrIdx); vHdrIdx.vt = VT_I4; vHdrIdx.lVal = 3;
    DISPPARAMS hdrParams{ &vHdrIdx, nullptr, 1, 0 };
    VARIANT vRetCount; VariantInit(&vRetCount);
    pChart->Invoke(dispidGetHeaderValue, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &hdrParams, &vRetCount, nullptr, nullptr);
    int received = (vRetCount.vt == VT_I4) ? vRetCount.lVal : 0;
    VariantClear(&vRetCount);

    if (received <= 0) {
        pChart->Release();
        outErrMsg = L"Cybos Plus returned 0 candles for " + code;
        return false;
    }

    outCandles.resize(received);
    for (int i = 0; i < received; ++i) {
        // Cybos는 최신 봉이 0번 인덱스 -> 역순 배치
        int targetIdx = received - 1 - i;
        auto GetData = [pChart, dispidGetDataValue, i](int fieldIdx) -> VARIANT {
            VARIANT args[2];
            VariantInit(&args[0]); args[0].vt = VT_I4; args[0].lVal = i; // row
            VariantInit(&args[1]); args[1].vt = VT_I4; args[1].lVal = fieldIdx; // col
            DISPPARAMS p{ args, nullptr, 2, 0 };
            VARIANT ret; VariantInit(&ret);
            pChart->Invoke(dispidGetDataValue, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &p, &ret, nullptr, nullptr);
            return ret;
        };

        VARIANT vD = GetData(0);
        VARIANT vT = GetData(1);
        VARIANT vO = GetData(2);
        VARIANT vH = GetData(3);
        VARIANT vL = GetData(4);
        VARIANT vC = GetData(5);
        VARIANT vV = GetData(6);

        BridgeCandle& bc = outCandles[targetIdx];
        swprintf_s(bc.date, L"%d", vD.lVal);
        swprintf_s(bc.time, L"%04d", vT.lVal);
        bc.open = (float)vO.lVal;
        bc.high = (float)vH.lVal;
        bc.low = (float)vL.lVal;
        bc.close = (float)vC.lVal;
        bc.volume = (uint64_t)vV.lVal;
        bc.ofi = 0.0f;

        VariantClear(&vD); VariantClear(&vT); VariantClear(&vO);
        VariantClear(&vH); VariantClear(&vL); VariantClear(&vC); VariantClear(&vV);
    }

    pChart->Release();
    return true;
}

int wmain(int argc, wchar_t* argv[]) {
    CoInitialize(nullptr);
    std::wcout << L"[BRIDGE-32] Gemini 32-bit Cybos IPC Bridge Broker Active." << std::endl;

    HANDLE hPipe = CreateNamedPipeW(
        PIPE_NAME,
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1, 65536, 65536, 0, nullptr
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::wcout << L"[BRIDGE-32][FATAL] Named Pipe creation failed. Error: " << GetLastError() << std::endl;
        CoUninitialize();
        return 1;
    }

    std::wcout << L"[BRIDGE-32] Named Pipe: " << PIPE_NAME << std::endl;
    std::wcout << L"[BRIDGE-32] Waiting for 64-bit Main Engine connection..." << std::endl;

    while (true) {
        BOOL connected = ConnectNamedPipe(hPipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (!connected) {
            CloseHandle(hPipe);
            CoUninitialize();
            return 1;
        }

        std::wcout << L"[BRIDGE-32] Main Engine client connected." << std::endl;

        PipeHeader reqHdr{};
        DWORD bytesRead = 0;
        BOOL ok = ReadFile(hPipe, &reqHdr, sizeof(reqHdr), &bytesRead, nullptr);
        if (ok && bytesRead == sizeof(reqHdr)) {
            std::vector<char> payload(reqHdr.payloadLen + 1, 0);
            if (reqHdr.payloadLen > 0) {
                ReadFile(hPipe, payload.data(), reqHdr.payloadLen, &bytesRead, nullptr);
            }

            std::wstring reqStr(reinterpret_cast<wchar_t*>(payload.data()), reqHdr.payloadLen / sizeof(wchar_t));
            std::wcout << L"[BRIDGE-32] Request: " << reqStr << std::endl;

            // 파싱: CODE|TF_TYPE|TF_UNIT|COUNT
            std::wstringstream ss(reqStr);
            std::wstring item, code;
            char tfType = 'm';
            int tfUnit = 1, count = 150;
            if (std::getline(ss, item, L'|')) code = item;
            if (std::getline(ss, item, L'|') && !item.empty()) tfType = (char)item[0];
            if (std::getline(ss, item, L'|')) tfUnit = _wtoi(item.c_str());
            if (std::getline(ss, item, L'|')) count = _wtoi(item.c_str());

            std::vector<BridgeCandle> candles;
            std::wstring errMsg;
            bool success = FetchCybosCandles(code, tfType, tfUnit, count, candles, errMsg);

            if (success) {
                PipeHeader resHdr{};
                memcpy(resHdr.magic, "GBRG", 4);
                resHdr.msgType = 2; // RES_CANDLES
                resHdr.payloadLen = static_cast<uint32_t>(candles.size() * sizeof(BridgeCandle));

                DWORD written = 0;
                WriteFile(hPipe, &resHdr, sizeof(resHdr), &written, nullptr);
                WriteFile(hPipe, candles.data(), resHdr.payloadLen, &written, nullptr);
                std::wcout << L"[BRIDGE-32] Replied with " << candles.size() << L" candles for " << code << std::endl;
            } else {
                std::wcout << L"[BRIDGE-32][ERROR] " << errMsg << std::endl;
                PipeHeader errHdr{};
                memcpy(errHdr.magic, "GBRG", 4);
                errHdr.msgType = 99; // ERROR
                errHdr.payloadLen = static_cast<uint32_t>(errMsg.size() * sizeof(wchar_t));

                DWORD written = 0;
                WriteFile(hPipe, &errHdr, sizeof(errHdr), &written, nullptr);
                WriteFile(hPipe, errMsg.c_str(), errHdr.payloadLen, &written, nullptr);
            }
        }

        DisconnectNamedPipe(hPipe);
    }

    CloseHandle(hPipe);
    CoUninitialize();
    return 0;
}