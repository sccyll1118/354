#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>

// 초기화 (첫 export 호출 시 자동 실행됨)
void EnsureInitialized();

// 이름(export명)으로 체인 우선순위에 따라 FARPROC 반환
// 우선순위: primary(SnowyMoon) → secondary(ReShade64) → system dxgi
FARPROC GetChainedProc(const char* name);

// 로그 기록 (내부 전용)
void ChainLog(const char* msg);
