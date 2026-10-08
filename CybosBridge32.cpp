#include "ChartTypes.hpp"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <comdef.h>
#include <thread>
#include <atomic>



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

const wchar_t* REAL_PIPE_NAME = L"\\\\.\\pipe\\GeminiBridgeRealPipe";
HANDLE g_hRealPipe = INVALID_HANDLE_VALUE;
std::atomic<bool> g_realPipeConnected{false};

void StartRealPipeServer() {
    std::thread([]() {
        while (true) {
            HANDLE hPipe = CreateNamedPipeW(
                REAL_PIPE_NAME,
                PIPE_ACCESS_OUTBOUND,
                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                1, 4096, 4096, 0, nullptr
            );

            if (hPipe == INVALID_HANDLE_VALUE) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }

            if (ConnectNamedPipe(hPipe, nullptr) || GetLastError() == ERROR_PIPE_CONNECTED) {
                g_hRealPipe = hPipe;
                g_realPipeConnected = true;
                std::wcout << L"[BRIDGE-32] Real-time pipe connected to 64-bit engine." << std::endl;

                while (g_realPipeConnected) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                }
            }

            g_realPipeConnected = false;
            CloseHandle(hPipe);
            g_hRealPipe = INVALID_HANDLE_VALUE;
        }
    }).detach();
}

void BroadcastRealTick(const RealTickPacket& tick) {
    if (!g_realPipeConnected || g_hRealPipe == INVALID_HANDLE_VALUE) return;

    PipeHeader hdr{};
    memcpy(hdr.magic, "GBRG", 4);
    hdr.msgType = 3; // REAL_TICK
    hdr.payloadLen = sizeof(RealTickPacket);

    DWORD written = 0;
    if (!WriteFile(g_hRealPipe, &hdr, sizeof(hdr), &written, nullptr) ||
        !WriteFile(g_hRealPipe, &tick, sizeof(tick), &written, nullptr)) {
        g_realPipeConnected = false;
    }
}

// CpSysDib.MarketEye 다중 종목 배치 데이터 다운로드 (Cybos 전담)
bool RequestMarketEye(const std::vector<std::wstring>& codes, std::vector<MarketEyeItem>& outItems) {
    outItems.clear();
    if (codes.empty()) return false;

    CLSID clsid;
    if (FAILED(CLSIDFromProgID(L"CpSysDib.MarketEye", &clsid))) return false;

    IDispatch* pEye = nullptr;
    if (FAILED(CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER, IID_IDispatch, (void**)&pEye))) return false;

    DISPID dispidSetInputValue, dispidBlockRequest, dispidGetHeaderValue, dispidGetDataValue;
    OLECHAR* nameSetInput = (OLECHAR*)L"SetInputValue";
    OLECHAR* nameReq = (OLECHAR*)L"BlockRequest";
    OLECHAR* nameGetHdr = (OLECHAR*)L"GetHeaderValue";
    OLECHAR* nameGetData = (OLECHAR*)L"GetDataValue";

    pEye->GetIDsOfNames(IID_NULL, &nameSetInput, 1, LOCALE_USER_DEFAULT, &dispidSetInputValue);
    pEye->GetIDsOfNames(IID_NULL, &nameReq, 1, LOCALE_USER_DEFAULT, &dispidBlockRequest);
    pEye->GetIDsOfNames(IID_NULL, &nameGetHdr, 1, LOCALE_USER_DEFAULT, &dispidGetHeaderValue);
    pEye->GetIDsOfNames(IID_NULL, &nameGetData, 1, LOCALE_USER_DEFAULT, &dispidGetDataValue);

    // 필드 정의: 0(종목코드), 4(현재가), 5(전일대비), 6(등락률), 10(거래량), 7(매도호가), 8(매수호가), 17(종목명), 20(기관순매수), 21(외인순매수)
    long fields[] = { 0, 4, 5, 6, 10, 7, 8, 17, 20, 21 };
    int fieldCount = sizeof(fields) / sizeof(fields[0]);

    SAFEARRAYBOUND sabField{ (ULONG)fieldCount, 0 };
    SAFEARRAY* psaFields = SafeArrayCreate(VT_VARIANT, 1, &sabField);
    for (LONG i = 0; i < fieldCount; ++i) {
        VARIANT v; VariantInit(&v); v.vt = VT_I4; v.lVal = fields[i];
        SafeArrayPutElement(psaFields, &i, &v);
    }

    // SetInputValue(0, fields)
    {
        VARIANT a0; VariantInit(&a0); a0.vt = VT_I4; a0.lVal = 0;
        VARIANT a1; VariantInit(&a1); a1.vt = VT_ARRAY | VT_VARIANT; a1.parray = psaFields;
        VARIANT args[2] = { a1, a0 };
        DISPPARAMS p{ args, nullptr, 2, 0 };
        pEye->Invoke(dispidSetInputValue, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &p, nullptr, nullptr, nullptr);
    }
    SafeArrayDestroy(psaFields);

    // 종목 배열 생성 및 SetInputValue(1, codes)
    SAFEARRAYBOUND sabCodes{ (ULONG)codes.size(), 0 };
    SAFEARRAY* psaCodes = SafeArrayCreate(VT_VARIANT, 1, &sabCodes);
    for (LONG i = 0; i < (LONG)codes.size(); ++i) {
        VARIANT v; VariantInit(&v); v.vt = VT_BSTR; v.bstrVal = SysAllocString(codes[i].c_str());
        SafeArrayPutElement(psaCodes, &i, &v);
        VariantClear(&v);
    }

    {
        VARIANT a0; VariantInit(&a0); a0.vt = VT_I4; a0.lVal = 1;
        VARIANT a1; VariantInit(&a1); a1.vt = VT_ARRAY | VT_VARIANT; a1.parray = psaCodes;
        VARIANT args[2] = { a1, a0 };
        DISPPARAMS p{ args, nullptr, 2, 0 };
        pEye->Invoke(dispidSetInputValue, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &p, nullptr, nullptr, nullptr);
    }
    SafeArrayDestroy(psaCodes);

    // BlockRequest()
    {
        DISPPARAMS p{ nullptr, nullptr, 0, 0 };
        pEye->Invoke(dispidBlockRequest, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &p, nullptr, nullptr, nullptr);
    }

    // GetHeaderValue(2): 수신 데이터 개수
    long count = 0;
    {
        VARIANT arg; VariantInit(&arg); arg.vt = VT_I4; arg.lVal = 2;
        DISPPARAMS p{ &arg, nullptr, 1, 0 };
        VARIANT res; VariantInit(&res);
        pEye->Invoke(dispidGetHeaderValue, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &p, &res, nullptr, nullptr);
        if (res.vt == VT_I4) count = res.lVal;
        VariantClear(&res);
    }

    auto GetData = [pEye, dispidGetDataValue](int fieldIdx, int row) -> VARIANT {
        VARIANT args[2];
        VariantInit(&args[0]); args[0].vt = VT_I4; args[0].lVal = row;      // 행
        VariantInit(&args[1]); args[1].vt = VT_I4; args[1].lVal = fieldIdx; // 열
        DISPPARAMS p{ args, nullptr, 2, 0 };
        VARIANT ret; VariantInit(&ret);
        pEye->Invoke(dispidGetDataValue, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &p, &ret, nullptr, nullptr);
        return ret;
    };

    for (int r = 0; r < count; ++r) {
        MarketEyeItem item{};
        VARIANT vCode = GetData(0, r);
        VARIANT vPrice = GetData(1, r);
        VARIANT vDiff = GetData(2, r);
        VARIANT vRate = GetData(3, r);
        VARIANT vVol = GetData(4, r);
        VARIANT vAsk = GetData(5, r);
        VARIANT vBid = GetData(6, r);
        VARIANT vName = GetData(7, r);
        VARIANT vInst = GetData(8, r);
        VARIANT vFrgn = GetData(9, r);

        if (vCode.vt == VT_BSTR && vCode.bstrVal) wcsncpy_s(item.code, sizeof(item.code) / sizeof(wchar_t), vCode.bstrVal, _TRUNCATE);
        if (vName.vt == VT_BSTR && vName.bstrVal) wcsncpy_s(item.name, sizeof(item.name) / sizeof(wchar_t), vName.bstrVal, _TRUNCATE);
        item.curPrice = (float)vPrice.lVal;
        item.diff = (float)vDiff.lVal;
        item.diffRate = (vRate.vt == VT_R4) ? vRate.fltVal : (float)vRate.dblVal;
        item.volume = (uint64_t)vVol.lVal;
        item.askPrice = (float)vAsk.lVal;
        item.bidPrice = (float)vBid.lVal;
        item.instNetBuy = (int64_t)vInst.lVal;
        item.foreignNetBuy = (int64_t)vFrgn.lVal;

        VariantClear(&vCode); VariantClear(&vPrice); VariantClear(&vDiff);
        VariantClear(&vRate); VariantClear(&vVol); VariantClear(&vAsk);
        VariantClear(&vBid); VariantClear(&vName); VariantClear(&vInst); VariantClear(&vFrgn);

        outItems.push_back(item);
    }

    pEye->Release();
    return true;
}

int wmain(int argc, wchar_t* argv[]) {
    CoInitialize(nullptr);
    StartRealPipeServer();
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