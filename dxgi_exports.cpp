/*
 * DXGI Export 전달 함수
 * ----------------------
 * 각 함수는 GetChainedProc()로 얻은 체인 우선순위 함수 포인터를 호출한다.
 * 함수 원형은 Windows SDK dxgi.h / dxgi1_2.h / dxgi1_3.h 기준.
 *
 * 우선순위: SnowyMoon.dll → ReShade64.dll → system dxgi.dll
 *
 * 로그: CreateDXGIFactory 계열은 어느 DLL로 전달됐는지 기록
 */
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>
#include "dxgi_proxy.h"

// ─── 함수 포인터 타입 정의 ───────────────────────────────────────────────────

typedef HRESULT (WINAPI *PFN_CreateDXGIFactory)(REFIID riid, void **ppFactory);
typedef HRESULT (WINAPI *PFN_CreateDXGIFactory1)(REFIID riid, void **ppFactory);
typedef HRESULT (WINAPI *PFN_CreateDXGIFactory2)(UINT Flags, REFIID riid, void **ppFactory);
typedef HRESULT (WINAPI *PFN_DXGIDeclareAdapterRemovalSupport)();
typedef HRESULT (WINAPI *PFN_DXGIGetDebugInterface1)(UINT Flags, REFIID riid, void **pDebug);

// 내부 / 비공개 export — void* 타입으로 단순 전달
typedef void*   (WINAPI *PFN_Passthrough)();

// ─── 로그 헬퍼 (내부 전용) ──────────────────────────────────────────────────

static void LogDispatch(const char* fn_name, FARPROC proc)
{
    if (!proc) {
        char msg[128];
        _snprintf_s(msg, sizeof(msg), _TRUNCATE,
            "[Dispatch] %s → NULL (no implementation found)", fn_name);
        ChainLog(msg);
    }
    // proc이 있는 경우는 성능상 로그 생략 (매 프레임 호출될 수 있음)
}

// ─── CreateDXGIFactory ───────────────────────────────────────────────────────

extern "C" HRESULT WINAPI CreateDXGIFactory(REFIID riid, void **ppFactory)
{
    EnsureInitialized();
    auto fn = reinterpret_cast<PFN_CreateDXGIFactory>(GetChainedProc("CreateDXGIFactory"));
    LogDispatch("CreateDXGIFactory", reinterpret_cast<FARPROC>(fn));
    if (!fn) return DXGI_ERROR_UNSUPPORTED;
    return fn(riid, ppFactory);
}

// ─── CreateDXGIFactory1 ──────────────────────────────────────────────────────

extern "C" HRESULT WINAPI CreateDXGIFactory1(REFIID riid, void **ppFactory)
{
    EnsureInitialized();
    auto fn = reinterpret_cast<PFN_CreateDXGIFactory1>(GetChainedProc("CreateDXGIFactory1"));
    LogDispatch("CreateDXGIFactory1", reinterpret_cast<FARPROC>(fn));
    if (!fn) return DXGI_ERROR_UNSUPPORTED;
    return fn(riid, ppFactory);
}

// ─── CreateDXGIFactory2 ──────────────────────────────────────────────────────

extern "C" HRESULT WINAPI CreateDXGIFactory2(UINT Flags, REFIID riid, void **ppFactory)
{
    EnsureInitialized();
    auto fn = reinterpret_cast<PFN_CreateDXGIFactory2>(GetChainedProc("CreateDXGIFactory2"));
    LogDispatch("CreateDXGIFactory2", reinterpret_cast<FARPROC>(fn));
    if (!fn) return DXGI_ERROR_UNSUPPORTED;
    return fn(Flags, riid, ppFactory);
}

// ─── DXGIDeclareAdapterRemovalSupport ───────────────────────────────────────

extern "C" HRESULT WINAPI DXGIDeclareAdapterRemovalSupport()
{
    EnsureInitialized();
    auto fn = reinterpret_cast<PFN_DXGIDeclareAdapterRemovalSupport>(
        GetChainedProc("DXGIDeclareAdapterRemovalSupport"));
    if (!fn) return DXGI_ERROR_UNSUPPORTED;
    return fn();
}

// ─── DXGIGetDebugInterface1 ──────────────────────────────────────────────────

extern "C" HRESULT WINAPI DXGIGetDebugInterface1(UINT Flags, REFIID riid, void **pDebug)
{
    EnsureInitialized();
    auto fn = reinterpret_cast<PFN_DXGIGetDebugInterface1>(
        GetChainedProc("DXGIGetDebugInterface1"));
    if (!fn) return DXGI_ERROR_UNSUPPORTED;
    return fn(Flags, riid, pDebug);
}

// ─── 내부/비공개 export — 시스템 dxgi로 단순 전달 ───────────────────────────
//
// DXGID3D10*, ApplyCompatResolutionQuirking, PIX* 등은
// 게임이 직접 사용하는 경우가 거의 없으나 SnowyMoon/ReShade 내부에서
// 체인 로딩 시 필요할 수 있으므로 시스템 DXGI로 투명하게 전달한다.
//
// 이 함수들의 원형은 공개 SDK에 없으므로 naked/raw 방식으로 전달한다.

#define DEFINE_PASSTHROUGH(name)                                   \
    extern "C" void WINAPI name()                                  \
    {                                                              \
        EnsureInitialized();                                       \
        static FARPROC s_fn = GetChainedProc(#name);              \
        if (s_fn) reinterpret_cast<PFN_Passthrough>(s_fn)();      \
    }

// 실제 dxgi.dll의 비공개 export 전달
// (원형 불명이므로 인자 전달 없이 단순 호출 — 이 export들은 D3D 내부 전용)
DEFINE_PASSTHROUGH(DXGID3D10CreateDevice)
DEFINE_PASSTHROUGH(DXGID3D10CreateLayeredDevice)
DEFINE_PASSTHROUGH(DXGID3D10GetLayeredDeviceSize)
DEFINE_PASSTHROUGH(DXGID3D10RegisterLayers)
DEFINE_PASSTHROUGH(ApplyCompatResolutionQuirking)
DEFINE_PASSTHROUGH(CompatString)
DEFINE_PASSTHROUGH(CompatValue)
DEFINE_PASSTHROUGH(DXGIReportAdapterConfiguration)
DEFINE_PASSTHROUGH(PIXBeginCapture)
DEFINE_PASSTHROUGH(PIXEndCapture)
DEFINE_PASSTHROUGH(PIXGetCaptureState)
DEFINE_PASSTHROUGH(SetAppCompatStringPointer)
