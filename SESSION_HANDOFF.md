# Gemini C++ Modular HTS & System Trading Engine 개발 세션 인계 문서

본 문서는 새 세션 시작 시 '세션이어가!' 명령으로 프로젝트 전체 아키텍처, 3대 실행 바이너리 빌드 파이프라인, IPC 프로토콜, 기구현 모듈 상태를 100% 즉시 복원하기 위한 인계 문서입니다.

---

## 1. 프로젝트 개요 & 핵심 설계 철학

* 프로젝트명: Gemini C++ Modular HTS & System Trading Engine (키움 0607 / 0601 스타일)
* 저장소 (GitHub): https://github.com/hoonoh57/gemini-cpp.git (main 브랜치)
* 언어 및 그래픽 런타임:
  * C++17 (MSVC x64/x86 컴파일러 cl.exe, UTF-8 인코딩)
  * 순수 Win32 API + Direct2D / DirectWrite 하드웨어 가속 렌더링
  * 서드파티 외부 라이브러리 배제 (CRT 기본 라이브러리 및 Windows SDK OS 내장 DLL만 사용)
* 브로커 엄격 분리 원칙 (Strict Broker Role Separation):
  1. 대신증권 Cybos Plus (32비트 CybosBridge32.exe): 배치 데이터 및 장세/수급 전담 공급원
     * CpSysDib.StockChart: 분/일/주 히스토리컬 캔들 대량 다운로드 (msgType = 1 / 2)
     * CpSysDib.MarketEye: 다중 종목 시세/호가/수급 스냅샷 일괄 조회 (msgType = 4 / 5)
     * CpSysDib.CpSvr7254: 종목별 시간대별 프로그램 매매 추이 (msgType = 6 / 7)
     * CpSysDib.CpSvr7043: 주도 섹터 및 업종별 등락률 랭킹 (msgType = 8 / 9)
     * CpSysDib.StockCur: 실시간 틱 데이터 브로드캐스트 파이프 (GeminiBridgeRealPipe)
  2. 키움증권 Open API+ (32비트 KiwoomBridge32.exe): 실시간 트레이딩 및 발굴 전담 공급원
     * SendOrder: 64비트 메인 엔진 주문 수신 및 즉시 실행 (GeminiKiwoomOrderPipe)
     * OnReceiveChejanData: 실시간 체결 및 계좌 잔고 푸시 스트림 (GeminiKiwoomChejanPipe, msgType = 11)
     * SendCondition: 실시간 조건검색 편입/이탈 푸시 스트림 (GeminiKiwoomConditionPipe, msgType = 21)
* 인코딩 규칙:
  * 터미널 및 파일 인코딩 UTF-8 강제 (UnicodeEncodeError 방지, 터미널 이모지 배제)

---

## 2. 64-bit / 32-bit 3대 바이너리 아키텍처

[ 64-bit Main Engine (kiwoom_modular_app.exe) ]
+-- Direct2D / DirectWrite 고성능 차트 렌더러 (ChartCoreRenderer.hpp)
|   +-- 멀티 뷰포트 분할 렌더링 (1x1, 2x2, 종목별 독립 스크롤/줌)
|   +-- 상단 HUD 실시간 오버레이 (현재가, OFI 수급강도, 키움 체결알림, 키움 조건검색알림)
|   +-- Win32 메시지 펌프 바인딩 (WM_USER_REAL_TICK, WM_USER_CHEJAN, WM_USER_CONDITION)
+-- CentralDataManager.hpp (IPC 통신 클라이언트 및 백그라운드 리스너 허브)
|   +-- Cybos 파이프: RequestCandles, RequestMarketEye, RequestProgramTrade, RequestSectorRanking
|   +-- Kiwoom 파이프: SendKiwoomOrder, StartChejanListener, StartConditionListener
+-- 수식관리자 (FormulaManagerDlg.hpp) & 시스템매매 (StrategyConditionDlg.hpp)

  ▲ (Named Pipe: GeminiBridgePipe / RealPipe)
  |
[ 32-bit CybosBridge32.exe ]
+-- 대신증권 Cybos Plus COM (CpUtil, CpSysDib)

  ▲ (Named Pipe: GeminiKiwoomOrderPipe / ChejanPipe / ConditionPipe)
  |
[ 32-bit KiwoomBridge32.exe ]
+-- 키움증권 Open API+ OCX (KHOpenAPI.ocx)

---

## 3. 핵심 IPC 메시지 프로토콜 규격 (ChartTypes.hpp)

* 공통 파이프 헤더 (PipeHeader): magic[4] ('GBRG'), msgType (uint32_t), payloadLen (uint32_t)
* 메시지 타입 코드:
  * 1 / 2: Cybos 캔들 요청 / 응답 (BridgeCandle)
  * 3: Cybos 실시간 틱 통보 (RealTickPacket)
  * 4 / 5: Cybos MarketEye 복수 종목 요청 / 응답 (MarketEyeItem)
  * 6 / 7: Cybos 프로그램 매매 추이 요청 / 응답 (ProgramTradeItem)
  * 8 / 9: Cybos 주도 섹터 랭킹 요청 / 응답 (SectorRankingItem)
  * 11: Kiwoom 실시간 체결/잔고 통보 (KiwoomChejanPacket)
  * 21: Kiwoom 실시간 조건검색 편입/이탈 통보 (KiwoomConditionRealPacket)
  * 99: 에러 통보

---

## 4. 빌드 파이프라인 (무인 검증 배치 파일)

1. 64비트 메인 엔진: build.bat -> kiwoom_modular_app.exe
2. 32비트 키움 트레이딩 브릿지: build_kiwoom_bridge32.bat -> KiwoomBridge32.exe
3. 32비트 Cybos 배치 브릿지: build_bridge32.bat -> CybosBridge32.exe

---

## 5. 완료 작업 목록

* [작업 1~13] 64비트 메인 D2D 렌더러, 멀티뷰포트, 수식관리자/시스템매매, Cybos 기초 파이프라인 구축
* [작업 14] 키움 트레이딩/조건검색 도메인 모델 설계 (ChartTypes.hpp)
* [작업 15] 키움 전담 32비트 브릿지(KiwoomBridge32.cpp) 및 빌드 파이프라인(build_kiwoom_bridge32.bat) 구축
* [작업 16] CentralDataManager 키움 주문 집행 IPC 클라이언트(SendKiwoomOrder) 구현
* [작업 17] 키움 실시간 체결/잔고(Chejan) 스트리밍 파이프 서버 및 클라이언트 리스너 구현
* [작업 18] 키움 실시간 조건검색(Condition) 스트리밍 파이프 서버 및 클라이언트 리스너 구현
* [작업 19~20] Cybos MarketEye 복수 종목 시세 스냅샷 IPC 파이프라인 완성 및 클라이언트 연동
* [작업 21] Cybos CpSvr7254 프로그램 매매 추이 IPC 파이프라인 완성 및 클라이언트 연동
* [작업 22] Cybos CpSvr7043 주도 섹터 및 업종 랭킹 IPC 파이프라인 완성 및 클라이언트 연동
* [작업 23~24] 키움 실시간 체결/잔고 및 조건검색 이벤트 Win32 메시지 루프 연동 및 DirectWrite HUD 상단 오버레이 렌더링 구현
