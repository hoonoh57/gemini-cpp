#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include "ChartTypes.hpp"

const wchar_t* KIWOOM_PIPE_NAME = L"\\\\.\\pipe\\GeminiKiwoomOrderPipe";
std::atomic<bool> g_running{true};

// 키움 OpenAPI 컨트롤 래퍼 (주문 및 조건검색 전담)
class CKiwoomBroker {
private:
    IDispatch* m_pDispatch = nullptr;
    HWND m_hWndHost = nullptr;

public:
    CKiwoomBroker() = default;
    ~CKiwoomBroker() { Release(); }

    bool Initialize(HWND hWndParent) {
        m_hWndHost = hWndParent;
        CLSID clsid;
        // KHOPENAPI.KHOpenAPICtrl.1
        if (FAILED(CLSIDFromProgID(L"KHOPENAPI.KHOpenAPICtrl.1", &clsid))) {
            std::wcout << L"[KIWOOM-32][FAIL] KHOPENAPI.KHOpenAPICtrl.1 not registered." << std::endl;
            return false;
        }

        HRESULT hr = CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER, IID_IDispatch, (void**)&m_pDispatch);
        if (FAILED(hr)) {
            std::wcout << L"[KIWOOM-32][FAIL] CoCreateInstance failed. HRESULT: " << hr << std::endl;
            return false;
        }

        std::wcout << L"[KIWOOM-32][OK] KHOpenAPI instance acquired successfully." << std::endl;
        return true;
    }

    void Release() {
        if (m_pDispatch) {
            m_pDispatch->Release();
            m_pDispatch = nullptr;
        }
    }

    // CommConnect (로그인 창 호출)
    long CommConnect() {
        if (!m_pDispatch) return -1;
        DISPID dispid;
        OLECHAR* name = (OLECHAR*)L"CommConnect";
        if (FAILED(m_pDispatch->GetIDsOfNames(IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &dispid))) return -1;

        DISPPARAMS params{ nullptr, nullptr, 0, 0 };
        VARIANT res; VariantInit(&res);
        m_pDispatch->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &params, &res, nullptr, nullptr);
        long ret = (res.vt == VT_I4) ? res.lVal : -1;
        VariantClear(&res);
        return ret;
    }

    // GetConnectState (0: 미연결, 1: 연결완료)
    long GetConnectState() {
        if (!m_pDispatch) return 0;
        DISPID dispid;
        OLECHAR* name = (OLECHAR*)L"GetConnectState";
        if (FAILED(m_pDispatch->GetIDsOfNames(IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &dispid))) return 0;

        DISPPARAMS params{ nullptr, nullptr, 0, 0 };
        VARIANT res; VariantInit(&res);
        m_pDispatch->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &params, &res, nullptr, nullptr);
        long ret = (res.vt == VT_I4) ? res.lVal : 0;
        VariantClear(&res);
        return ret;
    }

    // SendOrder 주문 집행
    long SendOrder(const KiwoomOrderRequest& req) {
        if (!m_pDispatch) return -1;
        DISPID dispid;
        OLECHAR* name = (OLECHAR*)L"SendOrder";
        if (FAILED(m_pDispatch->GetIDsOfNames(IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &dispid))) return -1;

        // BSTR 파라미터 준비
        BSTR sRQName = SysAllocString(L"ModularOrder");
        BSTR sScreenNo = SysAllocString(L"8001");
        BSTR sAccNo = SysAllocString(req.accNo);
        long nOrderType = static_cast<long>(req.orderType);
        BSTR sCode = SysAllocString(req.code);
        long nQty = req.qty;
        long nPrice = req.price;
        BSTR sHoga = SysAllocString(req.hogaType);
        BSTR sOrgOrderNo = SysAllocString(req.orgOrderNo);

        // DISPPARAMS 인자 역순 배치 (총 9개)
        VARIANT args[9];
        for (int i = 0; i < 9; ++i) VariantInit(&args[i]);
        args[8].vt = VT_BSTR; args[8].bstrVal = sRQName;
        args[7].vt = VT_BSTR; args[7].bstrVal = sScreenNo;
        args[6].vt = VT_BSTR; args[6].bstrVal = sAccNo;
        args[5].vt = VT_I4;   args[5].lVal = nOrderType;
        args[4].vt = VT_BSTR; args[4].bstrVal = sCode;
        args[3].vt = VT_I4;   args[3].lVal = nQty;
        args[2].vt = VT_I4;   args[2].lVal = nPrice;
        args[1].vt = VT_BSTR; args[1].bstrVal = sHoga;
        args[0].vt = VT_BSTR; args[0].bstrVal = sOrgOrderNo;

        DISPPARAMS params{ args, nullptr, 9, 0 };
        VARIANT res; VariantInit(&res);
        HRESULT hr = m_pDispatch->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &params, &res, nullptr, nullptr);
        long ret = (SUCCEEDED(hr) && res.vt == VT_I4) ? res.lVal : -1;

        VariantClear(&res);
        for (int i = 0; i < 9; ++i) VariantClear(&args[i]);
        return ret;
    }
};

CKiwoomBroker g_broker;

void RunOrderPipeServer() {
    std::thread([]() {
        while (g_running) {
            HANDLE hPipe = CreateNamedPipeW(
                KIWOOM_PIPE_NAME,
                PIPE_ACCESS_DUPLEX,
                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                1, 4096, 4096, 0, nullptr
            );

            if (hPipe == INVALID_HANDLE_VALUE) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }

            if (ConnectNamedPipe(hPipe, nullptr) || GetLastError() == ERROR_PIPE_CONNECTED) {
                std::wcout << L"[KIWOOM-32] Order IPC connected from 64-bit Engine." << std::endl;

                while (g_running) {
                    KiwoomOrderRequest req{};
                    DWORD readBytes = 0;
                    if (!ReadFile(hPipe, &req, sizeof(req), &readBytes, nullptr) || readBytes != sizeof(req)) {
                        break;
                    }

                    std::wcout << L"[KIWOOM-32] Order received: Code=" << req.code 
                               << L" Qty=" << req.qty << L" Price=" << req.price << std::endl;

                    long ret = g_broker.SendOrder(req);
                    DWORD written = 0;
                    WriteFile(hPipe, &ret, sizeof(ret), &written, nullptr);
                }
            }

            DisconnectNamedPipe(hPipe);
            CloseHandle(hPipe);
        }
    }).detach();
}

int wmain(int argc, wchar_t* argv[]) {
    OleInitialize(nullptr);
    std::wcout << L"=====================================================" << std::endl;
    std::wcout << L"[KIWOOM-32] Dedicated Trading & Condition IPC Broker" << std::endl;
    std::wcout << L"=====================================================" << std::endl;

    if (!g_broker.Initialize(nullptr)) {
        std::wcout << L"[KIWOOM-32][FAIL] OpenAPI initialization failed. Check 32-bit OCX registration." << std::endl;
    } else {
        std::wcout << L"[KIWOOM-32][STATUS] ConnectState=" << g_broker.GetConnectState() << std::endl;
    }

    RunOrderPipeServer();

    // 메시지 펌프 루프 (OCX 이벤트 및 통신 처리)
    MSG msg;
    while (g_running && GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    OleUninitialize();
    return 0;
}