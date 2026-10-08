# SESSION_HANDOFF.md: Gemini C++ Modular HTS 개발 세션 인계 문서

본 문서는 새 대화 세션 시작 시 "세션이어가!" 명령 한마디로 프로젝트의 전체 아키텍처, 빌드 파이프라인, 기구현 모듈, 향후 작업 로드맵을 100% 복원하기 위한 개발 세션 인계 문서입니다.

---

## 1. 프로젝트 개요 & 핵심 철학

* **프로젝트명**: Gemini C++ Modular HTS & System Trading Engine (키움 0607 / 0601 스타일)
* **저장소 (GitHub)**: `https://github.com/hoonoh57/gemini-cpp.git` (main 브랜치)
* **최신 커밋**: `e04eee5` (`feat(ui): add real-time status HUD with fail-fast visual feedback`)
* **언어 및 그래픽 런타임**:
  * C++17 (MSVC x64/x86 컴파일러 `cl.exe`, UTF-8 인코딩)
  * 순수 Win32 API + Direct2D / DirectWrite 하드웨어 가속 렌더링
  * 의존성 최소화 (순수 CRT 및 Windows 내장 API만 사용, 외부 거대 라이브러리 배제)
* **LAYA & 4축 설계 원칙**:
  1. **Core Domain (축 1)**: 불변 도메인 모델 (`ChartTypes.hpp`: `Candle`, `StockMaster`, `ViewportTransform`)
  2. **Addon Engine (축 2)**: 차트 지표 및 매매 전략 플러그인 인터페이스 (`IChartAddon`, `DynamicTradingStrategyAddon`)
  3. **Bridge/IPC (축 3)**: 64비트 메인 렌더러와 32비트 레거시 증권사 COM(대신 Cybos Plus / 키움 OpenAPI) 간 프로세스 격리 및 Named Pipe IPC
  4. **Persistence (축 4)**: 뷰포트 분할 상태(`layout_mode`) 및 슬롯 메타데이터의 경량 JSON 무손실 영속화 (`LayoutPersistence.hpp`)
* **데이터 무결성 원칙**:
  * 가짜 시뮬레이션 데이터를 조작하여 주입하는 행위를 엄격히 금지함 (데이터 오염 방지).
  * 32비트 브릿지 미실행 또는 TR 실패 시 원인을 콘솔과 화면 HUD에 명확히 출력하고 중단 (Fail-Fast 정책).
* **개발 및 검증 파이프라인**:
  * 전체 코드 일괄 재작성을 금지하고 헤더/소스 단위 핀포인트 패치 유지.
  * 관리자 PowerShell 스크립트를 통한 무인 빌드(`build.bat`, `build_bridge32.bat`) 및 실행 검증.
  * UTF-8 인코딩 강제 (`cp949` 에러 원천 방지, 터미널 이모지 사용 배제).

---

## 2. 하이브리드 아키텍처 구조

```text
[ 64-bit Main Engine (kiwoom_modular_app.exe) ]
├── Direct2D / DirectWrite 고성능 차트 렌더러 (ChartCoreRenderer.hpp)
├── 상단 툴바 및 실시간 상태 HUD / 종목코드 Win32 Edit 컨트롤
├── 멀티차트 분할 뷰 (1x1, 2x2, 종목별 독립 뷰포트/스크롤/줌)
├── 수식관리자 다이얼로그 (FormulaManagerDlg.hpp - 키움 0601 스타일)
├── 시스템매매 조건설정창 (StrategyConditionDlg.hpp)
├── CentralDataManager.hpp (Named Pipe 클라이언트 캐시 및 허브)
└── LayoutPersistence.hpp (JSON 직렬화/역직렬화 엔진)
                ▲
                │ IPC: \\.\pipe\GeminiBridgePipe (Binary Packet)
                ▼
[ 32-bit Legacy Bridge Process (CybosBridge32.exe) ]
├── Named Pipe 서버 루프
└── 대신증권 Cybos Plus (CpUtil.CpCybos, CpSysDib.StockChart 32-bit COM)
3. 기구현 모듈 명세
ChartTypes.hpp (축 1)

Candle: std::wstring date, std::wstring time, float open, high, low, close, uint64_t volume, float ofi

StockMaster: std::wstring code, std::wstring name

ViewportTransform: 가격/시간 좌표 <-> 화면 픽셀 매핑

DynamicTradingStrategyAddon.hpp & 다이얼로그군 (축 2)

DynamicTradingStrategyAddon: 골든/데드크로스, 슈퍼트렌드, 볼린저밴드 등 실시간 매매 시그널 생성 및 D2D 렌더링

FormulaManagerDlg.hpp: 0601 스타일 수식관리자 Win32 다이얼로그

StrategyConditionDlg.hpp: 전략 파라미터 튜닝 및 조건 설정창

CybosBridge32.cpp & CentralDataManager.hpp (축 3)

CybosBridge32.exe: x86 빌드. Cybos Plus 로그인 점검 및 StockChart TR 호출, BridgeCandle 바이너리 전송

CentralDataManager.hpp: RequestDataFromBridge()를 통한 온디맨드 TR 요청 및 캐싱

ChartCoreRenderer.hpp: 상단 에디트 컨트롤에서 엔터 시 활성 슬롯의 종목 즉시 교체 및 상태 HUD([OK], [FAIL]) 피드백

LayoutPersistence.hpp (축 4)

SaveToFile / LoadFromFile: layout_config.json을 통한 분할 레이아웃 모드(layout_mode), 슬롯별 종목코드, 봉수 영속화

main.cpp WM_DESTROY 시 자동 저장, Init() 시 자동 복원 바인딩

4. 향후 작업 로드맵
실시간 시세(Tick/체결가) 브릿지 스트리밍 확장:

현재 완성된 조회(TR, StockChart) 외에 Cybos Plus CpSysDib.StockCur 또는 키움 실시간 시세 이벤트를 브릿지 파이프라인에 추가 연동하여 틱 단위 실시간 캔들 갱신 구현.

동기화 토글 기능 고도화:

상단 =주기 버튼 활성화 시 4분할 슬롯 간 일괄 타임프레임 동기화 및 십자선 마우스 위치 연동 완성.

키움 Open API+ (32-bit OCX) 브릿지 지원 확장:

Cybos 외에 키움 OpenAPI(KHOpenAPI.ocx) 전담 브릿지 모듈 작성 및 멀티 브로커 셀렉터 구성.