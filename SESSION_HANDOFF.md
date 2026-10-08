# SESSION_HANDOFF.md: Gemini C++ Modular HTS 개발 세션 인계 문서

본 문서는 새 대화 세션 시작 시 "세션이어가!" 명령 한마디로 프로젝트의 전체 아키텍처, 빌드 파이프라인, 기구현 모듈, 향후 작업 로드맵을 100% 복원하기 위한 개발 세션 인계 문서입니다.

---

## 1. 프로젝트 개요 & 핵심 철학

* **프로젝트명**: Gemini C++ Modular HTS & System Trading Engine (키움 0607 / 0601 스타일)
* **저장소 (GitHub)**: `https://github.com/hoonoh57/gemini-cpp.git` (main 브랜치)
* **언어 및 그래픽 런타임**:
  * C++17 (MSVC x64/x86 컴파일러 `cl.exe`, UTF-8 인코딩)
  * 순수 Win32 API + Direct2D / DirectWrite 하드웨어 가속 렌더링
  * 의존성 최소화 (순수 CRT 및 Windows 내장 API만 사용, 외부 거대 라이브러리 배제)
* **LAYA & 4축 설계 원칙**:
  1. **Core Domain (축 1)**: 불변 도메인 모델 (`ChartTypes.hpp`: `Candle`, `StockMaster`, `ViewportTransform`, `RealTickPacket`)
  2. **Addon Engine (축 2)**: 차트 지표 및 매매 전략 플러그인 인터페이스 (`IChartAddon`, `DynamicTradingStrategyAddon`)
  3. **Bridge/IPC (축 3)**: 64비트 메인 렌더러와 32비트 증권사 브릿지 프로세스 간 Named Pipe IPC 파이프라인
  4. **Persistence (축 4)**: 뷰포트 분할 상태(`layout_mode`) 및 슬롯 메타데이터의 경량 JSON 무손실 영속화 (`LayoutPersistence.hpp`)
* **데이터 무결성 및 브로커 역할 분리 절대 원칙 (Strict Separation Rule)**:
  * 가짜 모의 데이터를 조작하여 주입하는 행위를 엄격히 금지함 (데이터 오염 방지 및 Fail-Fast 정책).
  * **[Cybos Plus 역할 - 배치/수급 데이터 전담]**:
    - 대량 캔들 다운로드 (`CpSysDib.StockChart`: 분/일/주 히스토리컬 데이터)
    - 복수 종목 시세 스냅샷 (`CpSysDib.MarketEye`)
    - 수급 및 시장 분석: 프로그램 순매수/순매도 추이, 투자자별 매매동향, 주도 섹터/테마 정보
  * **[키움 Open API+ 역할 - 메인 트레이딩 & 실시간 이벤트 전담]**:
    - 고속 주문 실행 (`SendOrder`: 현물 매수/매도/정정/취소)
    - 실시간 체결 및 잔고 추적 (`OnReceiveChejanData`: 미체결 내역, 실시간 체결가, 계좌 포지션/예수금)
    - 실시간 조건검색 (`SendCondition`, `OnReceiveTrCondition`, `OnReceiveRealCondition`: 실시간 편입/이탈 감지)
* **개발 및 검증 파이프라인**:
  * 전체 코드 일괄 재작성을 금지하고 헤더/소스 단위 핀포인트 패치 유지.
  * 관리자 PowerShell 스크립트를 통한 무인 빌드(`build.bat`, `build_bridge32.bat`) 및 실행 검증.
  * UTF-8 인코딩 강제 (`cp949` 에러 원천 방지, 터미널 이모지 사용 배제).

---

## 2. 64-bit / 32-bit 하이브리드 아키텍처 구조

```text
[ 64-bit Main Engine (kiwoom_modular_app.exe) ]
├── Direct2D / DirectWrite 고성능 차트 렌더러 (ChartCoreRenderer.hpp)
├── 상단 툴바 및 실시간 상태 HUD / 종목코드 Win32 Edit 컨트롤
├── 멀티차트 분할 뷰 (1x1, 2x2, 종목별 독립 뷰포트/스크롤/줌)
├── 수식관리자 다이얼로그 (FormulaManagerDlg.hpp - 키움 0601 스타일)
├── 시스템매매 조건설정창 (StrategyConditionDlg.hpp)
├── CentralDataManager.hpp (데이터 캐싱, 틱 이벤트 디스패처, 파이프 리스너)
└── LayoutPersistence.hpp (JSON 직렬화/역직렬화 엔진)
                ▲
                │ IPC: Named Pipes (\\.\pipe\GeminiBridgePipe, \\.\pipe\GeminiBridgeRealPipe)
                ▼
[ 32-bit Legacy Bridge Process (Bridge Broker) ]
├── [Cybos Plus Module (배치/수급/섹터 전담)]
│   ├── 대용량 캔들 다운로드 (CpSysDib.StockChart)
│   ├── 일괄 시세/호가 스냅샷 (CpSysDib.MarketEye)
│   └── 프로그램 순매수 & 주도 섹터 분석 데이터 수집
└── [Kiwoom OpenAPI Module (트레이딩/조건검색 전담)]
    ├── 실시간 주문/정정/취소 실행 (SendOrder)
    ├── 실시간 체결/잔고 감시 (OnReceiveChejanData)
    └── 실시간 조건검색식 감시 (SendCondition / RealCondition)
3. 기구현 모듈 명세
ChartTypes.hpp (축 1)

Candle: date, time, open, high, low, close, volume, ofi

StockMaster: code, name

RealTickPacket: 실시간 틱 데이터 구조체 (code, price, volume, time)

ViewportTransform: 좌표 변환 수학 모델

DynamicTradingStrategyAddon.hpp & 다이얼로그군 (축 2)

DynamicTradingStrategyAddon: 골든/데드크로스, 슈퍼트렌드, 볼린저밴드 등 시그널 생성 및 D2D 렌더링

FormulaManagerDlg.hpp: 0601 수식관리자

StrategyConditionDlg.hpp: 전략 파라미터 튜닝 조건창

CybosBridge32.cpp & CentralDataManager.hpp (축 3)

CybosBridge32.exe: Cybos Plus COM 연결 확인 및 StockChart TR 다운로드, 전용 실시간 파이프 서버 운영

CentralDataManager.hpp: TR 데이터 요청 캐싱 및 실시간 리스너 스레드(StartRealTimeListener) 구비

ChartCoreRenderer.hpp: 툴바 에디트, 상태 HUD, 주기 동기화(sync_tf), 실시간 틱 수신 시 InvalidateRect 리프레시 바인딩

LayoutPersistence.hpp (축 4)

순수 CRT/Win32 JSON 파서: layout_mode, 슬롯별 메타 무손실 저장 및 복원

4. 향후 작업 로드맵
키움 Open API+ (32-bit OCX/COM) 트레이딩 전담 브릿지 구축:

주문 실행(SendOrder), 체결/잔고 감시(OnReceiveChejanData), 실시간 조건검색식 연동 모듈 구현.

대신 Cybos Plus 배치 모듈 고도화:

CpSysDib.MarketEye 및 프로그램 매매 동향 배치 다운로드 파이프라인 확장.

메인 엔진과 브로커 간 양방향 라우팅 허브 연계:

데이터 조회는 Cybos 파이프로 라우팅, 실시간 조건검색 및 주문 실행은 키움 파이프로 자동 분기 처리.