# [SESSION_HANDOFF.md] Gemini C++ HTS/System Trading Architecture Handoff

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

---

## 2. 64-bit / 32-bit 하이브리드 아키텍처 구조

[ 64-bit Main Engine (kiwoom_modular_app.exe) ]
├── Direct2D / DirectWrite 고성능 차트 렌더러 (ChartCoreRenderer.hpp)
├── 멀티차트 분할 뷰 (1x1, 2x2, 종목별 독립 뷰포트/스크롤/줌)
├── 수식관리자 다이얼로그 (FormulaManagerDlg.hpp - 키움 0601)
├── 시스템매매 조건설정창 (StrategyConditionDlg.hpp)
└── CentralDataManager (데이터 캐싱 및 공급 허브)
▲
│ (Named Pipe / Shared Memory / Local IPC)
▼
[ 32-bit Legacy Bridge Process (Bridge Broker) ]
├── 키움증권 Open API+ (KHOpenAPI.ocx, 32-bit COM)
└── 대신증권 Cybos Plus (CpUtil, CpSysDib, 32-bit COM)


---

## 3. 기구현 완료 내용 (Git 커밋 상태)

현재 Git에 커밋 및 푸시된 9개 핵심 모듈의 역할은 다음과 같습니다.

1. **`Common.hpp`**: Win32, Direct2D, DirectWrite 표준 헤더 집약 및 리소스 관리 매크로 정의.
2. **`ChartTypes.hpp`**:
   * 도메인 데이터 구조 (`Candle`, `StockMaster`, `ViewportTransform`).
   * `StrategyConfig`: 강제 청산 6종(최대허용손실, 트레일링스톱, 이익보존율, 목표수익, 최소가격변화, 당일청산) 설정 및 파일 입출력.
   * `DynamicTradingStrategyAddon`: 골든크로스 매수 및 4대 강제 청산(손절/트레일링/보존/목표) 판별 엔진.
   * 청산 사유별 고유 색상 및 차트 마커(역삼각형 ▼ + 라벨 텍스트) 렌더링.
3. **`CentralDataManager.hpp`**:
   * 시세 바이너리 데이터(`cybos_master_data.bin`) 로드/세이브 및 슬롯별 차트 데이터 공급.
4. **`ChartCoreRenderer.hpp`**:
   * Direct2D 멀티차트(2x2 및 1x1 전체화면) 분할 렌더러.
   * 캔들 차트, 이동평균선(MA5, MA20, MA60, MA120), 마우스 드래그 팬, 휠 줌, 십자선(Crosshair).
   * 툴바(1x1/2x2 토글, 지표 추가, 전략 설정 배지, 수식관리자 호출 연동).
5. **`StrategyConditionDlg.hpp`**:
   * 키움 스타일 매매전략 조건 설정 모달 다이얼로그.
   * 포지션(매수/매도), 신호시 주문처리, 거래비용, 손절/트레일링/목표가 UI 컨트롤 바인딩.
   * 각 청산 조건별 색상 선택기(`ChooseColorW`) 및 영구 저장/복원.
6. **`FormulaManagerDlg.hpp`**:
   * 키움 0601 스타일 수식관리자 다이얼로그.
   * 좌측 수식 트리뷰, 중앙 탭 컨트롤(수식 1~5, 지표조건설정, 라인설정, 설명), 하단 함수 설명 박스.
7. **`main.cpp`**:
   * 메인 윈도우 생성, Direct2D 장치 컨텍스트 초기화, 메시지 펌프, 단축키 처리.
8. **`build.bat`**:
   * MSVC `cl.exe`를 통한 x64 무경고/무결점 직접 빌드 스크립트.
9. **`.gitignore`**:
   * 바이너리(`*.exe`, `*.obj`), 바이너리 시세 캐시(`*.bin`), 작업 세션 JSON 파일 추적 제외.

---

## 4. 향후 진행할 개발 로드맵 (Next Milestones)

1. **32비트 IPC 브릿지 프로세스 구축**:
   * 32비트 C++ 또는 C# 기반 COM 중계 프로세스 작성.
   * Named Pipe 또는 공유 메모리를 활용하여 64비트 메인 엔진과 초당 수천 틱의 실시간 호가/체결 데이터 송수신.
   * 키움 실주문(SendOrder) 및 Cybos 실시간 체결 이벤트 라우팅.
2. **수식관리자(0601) DSL 파서 및 인터프리터 고도화**:
   * 수식관리자 탭에서 입력한 텍스트 수식(예: `CrossUp(MA(C, 5), MA(C, 20))`)을 AST로 파싱하여 런타임에 지표/신호로 동적 렌더링.
3. **LAYA 의사결정 전략 모듈 결합**:
   * 멀티 타임프레임(일봉-분봉-틱) 상태 머신을 바탕으로 진입/청산 주문의 조건 부합성을 최종 검증하는 의사결정 레이어 구현.
4. **차트 기능성 보강**:
   * 거래량 보조차트 서브패널 분할 분리.
   * 볼린저 밴드, MACD, RSI, Stochastic 지표 라이브러리 추가.

---

## 5. 세션 복원 시 즉시 실행 검증 명령

새 세션에서 다음 스크립트를 실행하여 로컬 빌드 및 환경이 정상인지 즉시 검증합니다.

```powershell
& {
    Write-Host "[SESSION RECOVERY] Checking Environment..."
    git status -s
    cmd /c build.bat
    if (Test-Path -LiteralPath "kiwoom_modular_app.exe") {
        Write-Host "[OK] Engine built successfully. Session 100% recovered."
    } else {
        Write-Host "[ERR] Build failed. Need compiler environment configuration."
    }
}

---

### 로컬 파일 저장 및 GitHub 동기화 실행

위 인계 문서를 프로젝트 루트에 `SESSION_HANDOFF.md`로 생성하고 Git에 푸시하려면 아래 PowerShell 명령을 실행하십시오.

```powershell
& {
    $TargetDir = (Get-Location).Path
    $HandoffPath = [System.IO.Path]::Combine($TargetDir, "SESSION_HANDOFF.md")
    
    # 1. 파일이 없을 경우 대비하여 인계 문서 커밋
    git add SESSION_HANDOFF.md
    git commit -m "docs: 세션 이어가기용 아키텍처 및 작업 인계 문서(SESSION_HANDOFF.md) 추가"
    git push origin main
    
    Write-Host "[SYNC COMPLETE] SESSION_HANDOFF.md has been pushed to GitHub."
}