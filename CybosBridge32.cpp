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
    uint32_t msgType;    // 1: REQ_CANDLES, 2: RES_CANDLES, 3: REAL_TICK, 99: ERROR
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

struct RealTickPacket {
    wchar_t code[16];
    float price;
    uint64_t volume;
    wchar_t time[16];
};
#pragma pack(pop)

const wchar_t* PIPE_NAME = L"\\\\.\\pipe\\GeminiBridgePipe";

// 실시간 시세 구독 (CpSysDib.StockCur)
bool SubscribeRealTick(const std::wstring& code, IDispatch*& pOutCur, std::wstring& outErrMsg) {
    CLSID clsidCur;
    if (FAILED(CLSIDFromProgID(L"CpSysDib.StockCur", &clsidCur))) {
        outErrMsg = L"CpSysDib.StockCur COM not registered.";
        return false;
    }
    IDispatch* pCur = nullptr;
    if (FAILED(CoCreateInstance(clsidCur, nullptr, CLSCTX_INPROC_SERVER, IID_IDispatch, (void**)&pCur))) {
        outErrMsg = L"Failed to create CpSysDib.StockCur instance.";
        return false;
    }

    DISPID dispidSetInput;
    OLECHAR* n1 = (OLECHAR*)L"SetInputValue";
    pCur->GetIDsOfNames(IID_NULL, &n1, 1, LOCALE_USER_DEFAULT, &dispidSetInput);

    // 0: 종목코드 설정
    std::wstring stockCode = (code.rfind(L"A", 0) == 0) ? code : (L"A" + code);
    VARIANT vCode; VariantInit(&vCode); vCode.vt = VT_BSTR; vCode.bstrVal = SysAllocString(stockCode.c_str());

    VARIANT args[2];
    VariantInit(&args[0]); args[0] = vCode;
    VariantInit(&args[1]); args[1].vt = VT_I4; args[1].lVal = 0;
    DISPPARAMS params{ args, nullptr, 2, 0 };
    pCur->Invoke(dispidSetInput, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &params, nullptr, nullptr, nullptr);
    SysFreeString(vCode.bstrVal);

    // Subscribe 호출
    DISPID dispidSub;
    OLECHAR* n2 = (OLECHAR*)L"Subscribe";
    pCur->GetIDsOfNames(IID_NULL, &n2, 1, LOCALE_USER_DEFAULT, &dispidSub);
    DISPPARAMS noParams{ nullptr, nullptr, 0, 0 };
    pCur->Invoke(dispidSub, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &noParams, nullptr, nullptr, nullptr);

    pOutCur = pCur;
    return true;
}

// 차트 TR 조회 (CpSysDib.StockChart)
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

    DISPID dispidConnect;
    OLECHAR* nameConnect = (OLECHAR*)L"IsConnect";
    long isConnect = 0;
    if (SUCCEEDED(pCybos->GetIDsOfNames(IID_NULL, &nameConnect, 1, LOCALE_USER_DEFAULT, &dispidConnect))) {
        DISPPARAMS params{ nullptr, nullptr, 0, 0 };
        VARIANT res; VariantInit(&res);
        if (SUCCEEDED(pCybos->Invoke(dispidConnect, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &params, &res, nullptr, nullptr))) {
            isConnect = res.lVal;
            VariantClear(&res);
        }
    }
    pCybos->Release();

    if (isConnect != 1) {
        outErrMsg = L"Cybos Plus is not connected/logged in. Please start Cybos Starter.";
        return false;
    }

    IDispatch* pChart = nullptr;
    if (FAILED(CoCreateInstance(clsidChart, nullptr, CLSCTX_INPROC_SERVER, IID_IDispatch, (void**)&pChart))) {
        outErrMsg = L"Failed to create CpSysDib.StockChart instance.";
        return false;
    }

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
        VariantInit(&args[0]); args[0] = val;
        VariantInit(&args[1]); args[1].vt = VT_I4; args[1].lVal = type;
        DISPPARAMS params{ args, nullptr, 2, 0 };
        pChart->Invoke(dispidSetInputValue, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &params, nullptr, nullptr, nullptr);
    };

    std::wstring stockCode = (code.rfind(L"A", 0) == 0) ? code : (L"A" + code);
    VARIANT vCode; VariantInit(&vCode); vCode.vt = VT_BSTR; vCode.bstrVal = SysAllocString(stockCode.c_str());
    CallSetInput(0, vCode);
    SysFreeString(vCode.bstrVal);

    VARIANT vReqType; VariantInit(&vReqType); vReqType.vt = VT_UI1; vReqType.bVal = '2';
    CallSetInput(1, vReqType);

    VARIANT vCount; VariantInit(&vCount); vCount.vt = VT_I4; vCount.lVal = count;
    CallSetInput(4, vCount);

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

    VARIANT vChartType; VariantInit(&vChartType); vChartType.vt = VT_UI1; vChartType.bVal = (BYTE)tfType;
    CallSetInput(6, vChartType);

    VARIANT vUnit; VariantInit(&vUnit); vUnit.vt = VT_I4; vUnit.lVal = tfUnit;
    CallSetInput(7, vUnit);

    VARIANT vVolType; VariantInit(&vVolType); vVolType.vt = VT_UI1; vVolType.bVal = '1';
    CallSetInput(10, vVolType);

    DISPPARAMS noParams{ nullptr, nullptr, 0, 0 };
    pChart->Invoke(dispidBlockRequest, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &noParams, nullptr, nullptr, nullptr);

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
        int targetIdx = received - 1 - i;
        auto GetData = [pChart, dispidGetDataValue, i](int fieldIdx) -> VARIANT {
            VARIANT args[2];
            VariantInit(&args[0]); args[0].vt = VT_I4; args[0].lVal = fieldIdx; // col
            VariantInit(&args[1]); args[1].vt = VT_I4; args[1].lVal = i;        // row
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

    IDispatch* pActiveCur = nullptr;

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

                // 실시간 시세 구독 연계
                if (pActiveCur) { pActiveCur->Release(); pActiveCur = nullptr; }
                std::wstring subErr;
                SubscribeRealTick(code, pActiveCur, subErr);
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

    if (pActiveCur) pActiveCur->Release();
    CloseHandle(hPipe);
    CoUninitialize();
    return 0;
}