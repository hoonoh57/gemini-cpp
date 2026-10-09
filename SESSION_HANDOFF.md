# 세션 인수인계 문서 (SESSION_HANDOFF.md)

이 문서는 새 대화 세션 시작 시 "세션이어가!" 명령 한마디로 프로젝트의 전체 아키텍처, 빌드 파이프라인, 기구현 모듈, 향후 작업 로드맵을 100% 복원하기 위한 개발 세션 인계 문서입니다.

---

## 1. 프로젝트 개요 & 핵심 철학

* **프로젝트명**: Gemini C++ Modular HTS & System Trading Engine (키움 0607 / 0601 스타일)
* **저장소 (GitHub)**: [https://github.com/hoonoh57/gemini-cpp.git](https://github.com/hoonoh57/gemini-cpp.git) (main 브랜치)
* **언어 및 그래픽 런타임**:
  * C++17 (MSVC x64 컴파일러 cl.exe, UTF-8 인코딩)
  * 순수 Win32 API + Direct2D / DirectWrite 하드웨어 가속 렌더링
  * 의존성 최소화 (CRT 기본 라이브러리 및 OS 내장 DLL만 사용)
* **LAYA & 4축 설계 원칙**:
  1. Core Domain (축 1): 불변 도메인 모델 (Candle, StockMaster, 뷰포트 좌표계 ViewportTransform)
  2. Addon Engine (축 2): 차트 지표 및 매매 전략 플러그인 인터페이스 (IChartAddon, DynamicTradingStrategyAddon)
  3. Bridge/IPC (축 3): 64비트 메인 렌더러와 32비트 레거시 증권사 COM 간 분리 격리 및 IPC 파이프라인
  4. Persistence (축 4): 뷰포트 슬롯 상태 및 사용자 전략 설정의 무손실 JSON 영속화
* **개발 및 검증 파이프라인**:
  * 전체 코드 일괄 재작성을 금지하고 헤더/소스 단위 모듈화 유지
  * PowerShell 스크립트를 통한 특정 모듈 핀포인트 생성, 패치, 무인 빌드(build.bat), 실행 검증
  * UTF-8 인코딩 강제 ('cp949' 코덱 에러 방지, 터미널/로그 이모지 배제)

---

## 2. 64-bit / 32-bit 하이브리드 아키텍처 구조

[ 64-bit Main Engine (kiwoom_modular_app.exe) ]
+-- Direct2D / DirectWrite 고성능 차트 렌더러 (ChartCoreRenderer.hpp)
+-- 멀티차트 분할 뷰 (1x1, 2x2, 종목별 독립 뷰포트/스크롤/줌)
+-- 수식관리자 다이얼로그 (FormulaManagerDlg.hpp - 키움 0601 스타일)
+-- 시스템매매 조건설정창 (StrategyConditionDlg.hpp, F9 바인딩)
+-- CentralDataManager (데이터 캐싱 및 공급 허브)
     ^
     | (Named Pipes: \\.\pipe\GeminiBridgePipe, GeminiKiwoomChejanPipe 등)
     v
[ 32-bit Legacy Bridge Process (Bridge Broker) ]
+-- Step 1 (선행): 대신증권 Cybos Plus (CpUtil.CpCybos, CpSysDib - 32-bit COM)
+-- Step 2 (후행): 키움증권 Open API+ (KHOpenAPI.ocx - 32-bit COM)

### 32비트 COM 초기화 절대 원칙 (선후 관계)
1. 대신 Cybos Plus 검증 (선행): CpUtil.CpCybos.IsConnect == 1을 반드시 최우선으로 검증. 준비되지 않은 경우 사용자에게 재로그인 안내 메시지박스를 표시한 후 즉시 정상 종료.
2. 키움 Open API+ 초기화 (후행): Cybos Plus가 100% 정상 연결된 경우에만 키움 로그인 및 수신 파이프라인 가동.
3. 이 순서가 위배될 경우 두 32-bit COM 간 충돌로 인해 이후 모든 실시간/배치 IPC가 불가능해짐.

---

## 3. 현재까지 구현 완료 사항 (Completed)

1. Direct2D 하드웨어 가속 렌더링 코어:
   * 멀티슬롯(1x1, 2x2) 분할 레이아웃 엔진 완성
   * 캔들스틱, 거래량 바 차트, 이동평균선 오버레이 렌더링
   * 마우스 휠 줌(Zoom In/Out), 드래그 패닝(Panning), 크로스헤어 툴팁 완비
2. 다이얼로그 시스템 모듈화:
   * 키움 0601 스타일 수식관리자 (FormulaManagerDlg.hpp)
   * 시스템 트레이딩 전략 조건설정창 (StrategyConditionDlg.hpp, 단축키 F9 바인딩 완료)
3. 영속성 계층 (Persistence):
   * 슬롯별 종목코드, 표시 봉 개수, 스크롤 오프셋의 layout_config.json 자동 저장 및 복원
4. IPC 구조 분리 및 비동기화:
   * ChartCoreRenderer.hpp 내 관심종목 스냅샷(RequestMarketEye), 프로그램 매매(RequestProgramTrade), 섹터 랭킹(RequestSectorRanking)의 동기 호출을 백그라운드 std::thread로 분리하여 UI 프리징 방지
   * 에디트 컨트롤 생성 시 부모 핸들(hwnd = h) 대입 순서 교정
5. 통합 순차 실행 파이프라인 구성:
   * Cybos(선행 점검) -> 키움(후행 기동) -> 메인 렌더러 순차 기동 스크립트(run_all.bat) 뼈대 구축

---

## 4. 현재 당면 과제 및 문제점 (Current Issues)

1. 메인 앱(64-bit) UI 스레드 생성 블로킹 현상:
   * 증상: kiwoom_modular_app.exe 단독 기동 시 프로세스는 정상 생성되나 MainWindowHandle이 0으로 남고 화면에 프레임이 노출되지 않음.
   * 원인: WndProc의 WM_CREATE 메시지 핸들러가 동기 호출인데, 내부 g_Engine.Init(hwnd) 과정에서 32비트 브릿지 통신 또는 리스너 루틴이 스레드를 붙잡아 CreateWindowExW의 반환을 차단함.
2. 브릿지 프로세스 자동 구동 및 독립 콘솔 부재:
   * 64-bit 메인 엔진에서 32-bit 브릿지 프로세스를 직접 자식 프로세스로 분기 생성(CREATE_NEW_CONSOLE)하지 않아 브릿지가 자동으로 뜨지 않고 cmd 창이 각각 분리되지 않음.
3. Cybos 선결 조건 미충족 시 안정적 Fallback 체계 완성 필요:
   * Cybos Plus 미로그인 상태 시 브릿지 단에서 안내 팝업 후 안전 종료하고, 메인 렌더러는 오프라인 캐시(더미 뷰포트) 상태로 즉시 정상 노출되는 예외 처리가 통합 검증되어야 함.

---

## 5. 향후 작업 로드맵 (Next Action Items)

1. 메인 GUI 스레드 완전 넌블로킹 보장:
   * main.cpp의 WM_CREATE 처리를 0ms 즉각 반환으로 전환하여 창 생성(CreateWindowExW) 및 화면 노출(ShowWindow)을 최우선 완료.
   * Direct2D 렌더 타깃 생성 및 리소스 초기화 완료 후 첫 프레임 강제 갱신(InvalidateRect).
   * 브릿지 미응답 시에도 더미/오프라인 모드로 즉시 진입하여 빈 차트 그리드를 정상 렌더링.
2. 32비트 브릿지 프로세스 라이프사이클 관리자 구축:
   * 64비트 메인 엔진에서 CREATE_NEW_CONSOLE 속성으로 CybosBridge32.exe 및 KiwoomBridge32.exe를 순차 기동하는 프로세스 관리 루틴 연동.
   * Cybos 연결 성공 확인 이벤트(또는 파이프 신호) 수신 후 키움 브릿지 구동.
3. 실시간 패킷 수신 및 멀티 슬롯 동기화:
   * Cybos Plus 시세 틱 수신(RealTickPacket) 및 키움 체결/조건검색 패킷(KiwoomChejanPacket, KiwoomConditionRealPacket) 렌더링 갱신 연결.
4. 시스템 매매 모듈 실주문 IPC 연동:
   * 조건설정창(F9)에서 확정된 전략 파라미터를 키움 브릿지 주문 파이프로 전달하는 실행 루틴 연동.
