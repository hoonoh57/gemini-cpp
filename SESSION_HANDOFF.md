# Gemini C++ Modular HTS & System Trading Engine 개발 세션 인계 문서

> **최신 동기화 커밋**: `9c85635` (main 브랜치)
> **동기화 일자**: 2026-10-09
> **핵심 원칙**: 4축 모듈 설계(LAYA), 엄격한 증권사 역할 분리(대신 Cybos: 배치 데이터 / 키움 Open API: 트레이딩), 이모지 사용 금지, UTF-8 무손실 인코딩

---

## 1. 3대 독립 바이너리 빌드 파이프라인 및 실행 상태

| 바이너리 | 타깃 | 전담 역할 및 데이터 프로토콜 | 빌드 스크립트 |
| :--- | :---: | :--- | :--- |
| `kiwoom_modular_app.exe` | **64-bit** | D2D 하드웨어 가속 렌더러, DirectWrite HUD(체결/조건검색/주도섹터 TOP 3), MarketEye 관심종목 스냅샷, 프로그램 매매 추이 패널, QuickOrderDlg(F8 수동주문), StrategyConditionDlg(F9 시스템매매), CentralDataManager 허브 | `build.bat` |
| `KiwoomBridge32.exe` | **32-bit** | **키움 전담 트레이딩 엔진**: SendOrder 주문 파이프(`GeminiKiwoomOrderPipe`), 실시간 체결/잔고 파이프(`GeminiKiwoomChejanPipe`), 실시간 조건검색 파이프(`GeminiKiwoomConditionPipe`) | `build_kiwoom_bridge32.bat` |
| `CybosBridge32.exe` | **32-bit** | **대신 Cybos 전담 배치 데이터 엔진**: StockChart 캔들 대량 다운로드, MarketEye 다중시세 스냅샷, CpSvr7254 프로그램 매매 추이, CpSvr7043 주도섹터 랭킹, StockCur 실시간 틱 푸시 파이프 | `build_bridge32.bat` |
| `run_all.bat` | Batch | 기존 3개 프로세스 정리 -> 32비트 브릿지 2종 백그라운드 구동 -> IPC 파이프 연결 -> 64비트 메인 엔진 가동 | - |

---

## 2. 주요 단축키 및 인터페이스

* **F8**: 키움 즉시 수동 주문 패널 (`QuickOrderDlg`) - 신규매수/매도, 수량, 시장가/지정가 즉시 발주
* **F9**: 키움 0601 스타일 시스템매매 조건설정창 (`StrategyConditionDlg`) - 자동 주문 활성화 체크 및 기본 수량 설정
* **화면 상단 HUD**: 주도 섹터 랭킹 TOP 3 오버레이, 키움 실시간 체결 알림, 키움 실시간 조건검색 편입/이탈 알림
* **화면 좌측 하단**: Cybos 다중 종목 시세 스냅샷 (`MarketEye`)
* **화면 우측 하단**: Cybos 프로그램 매매 순매수 금액/수량 추이 (`CpSvr7254`)
