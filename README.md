# ETS2 DXGI Chain Loader

Euro Truck Simulator 2에서 **SnowyMoon** 그래픽 플러그인과  
**ReShade 6.4 애드온**을 동시에 로드하기 위한 Windows x64 DXGI 프록시 체인 로더.

> ⚠️ **중요한 제한 사항** — 설치 전 반드시 읽으세요.  
> 컴파일 성공이 동시 작동 성공을 의미하지 않습니다.  
> SnowyMoon과 ReShade는 모두 DXGI를 후킹하며, 런타임 충돌이 발생할 수 있습니다.  
> 반드시 실제 ETS2 실행으로 확인하고, 문제가 생기면 복구 방법을 따르세요.

---

## 작동 방식

```
ETS2 (게임)
  └─ dxgi.dll  ← 이 프록시 (체인 로더)
        ├─ SnowyMoon.dll  로드 & DXGI export 우선 사용
        ├─ ReShade64.dll  로드 & 위에 없으면 사용
        └─ system dxgi.dll  최종 fallback
```

1. ETS2가 `dxgi.dll`을 로드하면 이 프록시가 실행된다.
2. 프록시는 `SnowyMoon.dll`과 `ReShade64.dll`을 `LoadLibraryW`로 로드한다.
3. ETS2가 `CreateDXGIFactory` 등을 호출하면 프록시가 체인 우선순위에 따라 전달한다.
4. 모든 동작은 `dxgi_chain.log`에 기록된다.

---

## 최종 파일 구조

```
Euro Truck Simulator 2/bin/win_x64/
├─ dxgi.dll                          ← 이 프록시 (새 파일)
├─ SnowyMoon.dll                     ← 기존 SnowyMoon의 dxgi.dll 이름 변경
├─ ReShade64.dll                     ← 기존 ReShade 6.4의 dxgi.dll 이름 변경
├─ dxgi_chain.ini                    ← 프록시 설정 파일
├─ ETS2ReverseScreenRuntime.addon64  ← ETS2 Reverse Posture Assistant
├─ ETS2ReverseEnvironment.exe        ← 환경 서비스
└─ plugins/
   └─ scs-telemetry.dll
```

---

## 설치 방법

### 1단계 — 기존 파일 이름 변경

```
bin/win_x64/ 폴더에서:

SnowyMoon의 dxgi.dll  →  SnowyMoon.dll  로 이름 변경
ReShade 6.4의 dxgi.dll  →  ReShade64.dll  로 이름 변경
```

> ReShade64.dll의 ReShade.ini, 셰이더 파일 등은 그대로 유지

### 2단계 — 새 프록시 파일 배치

GitHub Actions Artifact에서 받은 파일을 `bin/win_x64/`에 복사:

```
dxgi.dll          ← 프록시 체인 로더
dxgi_chain.ini    ← 설정 파일 (필요시 수정)
```

### 3단계 — addon64 파일 배치

```
ETS2ReverseScreenRuntime.addon64  →  bin/win_x64/ 에 복사
ETS2ReverseEnvironment.exe        →  bin/win_x64/ 에 복사
```

### 4단계 — 게임 실행 및 로그 확인

게임 실행 후 `bin/win_x64/dxgi_chain.log` 파일을 열어 로드 결과를 확인한다.

---

## 설정 파일 (dxgi_chain.ini)

```ini
[chain]
primary=SnowyMoon.dll    ; DXGI export를 먼저 찾을 DLL
secondary=ReShade64.dll  ; primary에 없을 때 사용
fallback=system          ; 정보용 (항상 system dxgi)
```

DLL 이름만 변경 가능. 경로는 이 `dxgi.dll`과 **같은 폴더** 기준.

---

## 로그 확인 방법

게임 실행 후 `bin/win_x64/dxgi_chain.log` 파일 확인:

```
[ETS2DxgiChainLoader] Proxy chain loader starting
[ETS2DxgiChainLoader] DLL directory: C:\...\bin\win_x64\
[Config] primary   = SnowyMoon.dll
[Config] secondary = ReShade64.dll
[Init] Loading system dxgi: C:\Windows\System32\dxgi.dll
[Load] system dxgi.dll loaded OK (handle=0x...)
[Init] Loading primary: C:\...\SnowyMoon.dll
[Load] SnowyMoon.dll loaded OK (handle=0x...)
[Init] Loading secondary: C:\...\ReShade64.dll
[Load] ReShade64.dll loaded OK (handle=0x...)
[Init] Initialization complete
```

`FAILED`가 보이면 해당 DLL이 없거나 의존 파일이 누락된 것이다.

---

## 복구 방법

작동하지 않을 경우 원래 상태로 복구:

```
1. bin/win_x64/dxgi.dll (프록시) 삭제
2. SnowyMoon.dll → dxgi.dll 로 이름 복원
3. ReShade64.dll 삭제 또는 별도 폴더로 이동
```

복구 후 게임이 정상 실행되면 SnowyMoon만 단독으로 작동하는 상태이다.

---

## ⚠️ 알려진 제한 사항 및 미확인 사항

| 항목 | 내용 |
|------|------|
| DXGI 충돌 | SnowyMoon과 ReShade가 모두 DXGI를 후킹하므로 런타임 충돌 가능 |
| 검은 화면 | CreateDXGIFactory 체인 순서가 맞지 않으면 렌더링 실패 가능 |
| 효과 미적용 | ReShade 셰이더가 적용 안 될 수 있음 |
| 컴파일 ≠ 작동 | 빌드 성공이 게임 내 정상 동작을 보장하지 않음 |
| DXGID3D10* 전달 | 비공개 export는 인자 없이 단순 전달 — 내부적으로 사용하지 않으면 문제 없음 |
| 실제 테스트 | 이 프로젝트는 Replit(Linux 환경)에서 작성됨 — Windows 실행 테스트 미완 |

---

## GitHub Actions 빌드

저장소에 파일 커밋 후:

1. GitHub → **Actions** 탭
2. **Build ETS2 DXGI Chain Loader** 선택
3. **Run workflow** 클릭
4. 완료 후 **Artifacts** → **ETS2-DXGI-Chain-Loader** 다운로드
5. ZIP 압축 해제 → `dxgi.dll`, `dxgi_chain.ini`, `README.md` 포함
