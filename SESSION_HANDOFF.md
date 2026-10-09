# 개발 세션 인계 문서 (SESSION_HANDOFF.md)

이 문서는 새 대화 세션 시작 시 "세션이어가!" 명령 한마디로 프로젝트의 전체 아키텍처, 빌드 파이프라인, 기구현 모듈, 향후 작업 로드맵을 100% 복원하기 위한 개발 세션 인계 문서입니다.

---

## 1. 프로젝트 개요 & 핵심 철학

* **프로젝트명**: Gemini C++ Modular HTS & System Trading Engine (키움 0607 / 0601 스타일)
* **저장소 (GitHub)**: `https://github.com/hoonoh57/gemini-cpp.git` (main 브랜치)
* **언어 및 그래픽 런타임**:
  * C++17 (MSVC x64 컴파일러 `cl.exe`, UTF-8 인코딩)
  * 순수 Win32 API + Direct2D / DirectWrite 하드웨어 가속 렌더링
  * 의존성 최소화 (CRT 기본 라이브러리 및 OS 내장 DLL만 사용)
* **LAYA & 4축 설계 원칙**:
  1. **Core Domain (축 1)**: 불변 도메인 모델 (`Candle`, `StockMaster`, 뷰포트 좌표계 `ViewportTransform`)
  2. **Addon Engine (축 2)**: 차트 지표 및 매매 전략 플러그인 인터페이스 (`IChartAddon`, `DynamicTradingStrategyAddon`)
  3. **Bridge/IPC (축 3)**: 64비트 메인 렌더러와 32비트 레거시 증권사 COM(키움 OpenAPI, 대신 Cybos Plus) 간 분리 격리 및 IPC 파이프라인
  4. **Persistence (축 4)**: 뷰포트 슬롯 상태 및 사용자 전략 설정의 무손실 JSON 영속화
* **개발 및 검증 파이프라인**:
  * 전체 코드 일괄 재작성을 금지하고 헤더/소스 단위 모듈화 유지
  * PowerShell 스크립트를 통한 특정 모듈 핀포인트 생성·패치·무인 빌드(`build.bat`)·실행 검증
  * UTF-8 인코딩 강제 (`UnicodeEncodeError: 'cp949'` 방지, 터미널 이모지 사용 배제)
  * **무결성 원칙**: 임의의 모의/가상 데이터(`rand()` 생성 등) 사용 절대 금지. 데이터 획득 실패 시 에러 원인을 명시하고 교정할 것.

---

## 2. 64-bit / 32-bit 하이브리드 아키텍처 구조

[ 64-bit Main Engine (kiwoom_modular_app.exe) ]
├── Direct2D / DirectWrite 고성능 차트 렌더러 (ChartCoreRenderer.hpp)
├── 멀티차트 분할 뷰 (1x1, 2x2, 종목별 독립 뷰포트/스크롤/줌)
├── 수식관리자 다이얼로그 (FormulaManagerDlg.hpp - 키움 0601)
├── 시스템매매 조건설정창 (StrategyConditionDlg.hpp)
└── CentralDataManager (데이터 캐싱 및 공급 허브)
       ▲
       │ (Named Pipe: \\.\pipe\GeminiBridgePipe)
       ▼
[ 32-bit Legacy Bridge Process (Bridge Broker) ]
├── 키움증권 Open API+ (KHOpenAPI.ocx, 32-bit COM)
└── 대신증권 Cybos Plus (CpUtil, CpSysDib, 32-bit COM)

---

## 3. 기구현 모듈 상태 & 파일 맵

* **저장소 위치**: `E:\2026\gemini\gemini-cpp\`

| 모듈 / 파일명 | 설계 축 | 주요 기능 및 구현 상태 |
| :--- | :---: | :--- |
| `ChartTypes.hpp` | 축 1 | 불변 모델 정의 (`StockMaster`, `Candle`, `SectorRankingItem`, `KiwoomOrderRequest`, `ViewportTransform` 등) |
| `Common.hpp` | 공통 | UI 테마 색상(키움 다크/라이트 테마), 폰트 캐시, 공용 매크로 |
| `ChartCoreRenderer.hpp` | 축 1 | Direct2D 기반 멀티 슬롯(1x1, 2x2) 렌더링, 십자선 도구, 가격/시간축 렌더러 |
| `CentralDataManager.hpp` | 축 1 | 실시간 틱/체결 및 마스터 데이터 인메모리 관리, 증권사 브리지 IPC 수신 허브 |
| `CybosBridge32.cpp` | 축 3 | 32비트 Cybos Plus 브리지 (`CpSysDib.CpSvrNew7043` 업종순위, `CpSysDib.StockChart` 캔들 다운로드) |
| `KiwoomBridge32.cpp` | 축 3 | 32비트 키움 OpenAPI 브리지 (조건검색식 및 주문 발주) |
| `build.bat` | 빌드 | 64비트 메인 엔진 컴파일 스크립트 |
| `build_bridge32.bat` | 빌드 | 32비트 Cybos 브리지 컴파일 스크립트 |
| `SESSION_HANDOFF.md` | 문서 | 개발 세션 연속성 유지 및 아키텍처 인계 명세서 |

---

## 4. 직전 진행 단계 및 확인된 사실 (Fact Log)

1. Cybos 32비트 브리지 단독 검증 완료 (`CpSysDib.CpSvrNew7043` 40개 항목 수신 확인).
2. 64비트 메인 엔진 컴파일 정합성 완료 (와이드 문자열 및 콜백 구조체 일치).
3. 런처와 브리지 간 Named Pipe 생성 동기화 확인.
4. **해결 대상 결함 확인**:
   - `ChartCoreRenderer.hpp`: 십자선 하단 가로축 일시(Date/Time) 툴팁 및 X축 날짜 눈금 렌더링 누락.
   - `CentralDataManager.hpp`: `rand()` 기반 가상 데이터 전면 제거 및 `CpSysDib.StockChart` 실제 데이터 파이프라인 안착 필요.
   - 실패 시 가상 데이터로 우회하지 않고 실패 원인 로깅 및 교정 원칙 준수.

---

## 5. 향후 작업 로드맵 (Next Action Plan)

1. **GitHub 동기화 완료**: 현재 인계 문서와 최신 로컬 코드를 원격 저장소에 완벽 동기화.
2. **동기화된 기준 코드 검증**:
   - `git diff` 및 저장소 기준으로 `ChartCoreRenderer.hpp` 십자선 날짜 표시 로직 확인.
   - `CentralDataManager.hpp`의 난수 발생 로직을 원격 기준선에서 제거하고 에러 리포팅 구조로 교정.
3. **Cybos StockChart TR 파이프라인 개통**:
   - `CybosBridge32.cpp`에서 실제 종목별 캔들을 조회하여 파이프로 전송.
   - 메인 엔진에서 수신하여 화면에 실제 일봉 데이터 및 날짜 렌더링.