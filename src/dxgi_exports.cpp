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

#include <Windows.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>
#include <cstdio>

#include "dxgi_proxy.h"

// ─── 함수 포인터 타입 정의 ───────────────────────────────────────────────────

typedef HRESULT(WINAPI* PFN_CreateDXGIFactory)(
    REFIID riid,
    void** ppFactory
);

typedef HRESULT(WINAPI* PFN_CreateDXGIFactory1)(
    REFIID riid,
    void** ppFactory
);

typedef HRESULT(WINAPI* PFN_CreateDXGIFactory2)(
    UINT Flags,
    REFIID riid,
    void** ppFactory
);

typedef HRESULT(WINAPI* PFN_DXGIDeclareAdapterRemovalSupport)();

typedef HRESULT(WINAPI* PFN_DXGIGetDebugInterface1)(
    UINT Flags,
    REFIID riid,
    void** pDebug
);

// 내부 / 비공개 export — void* 타입으로 단순 전달
typedef void*(WINAPI* PFN_Passthrough)();

// ─── 로그 헬퍼 ───────────────────────────────────────────────────────────────

static void LogDispatch(const char* fn_name, FARPROC proc)
{
    if (!proc) {
        char msg[128];

        snprintf(
            msg,
            sizeof(msg),
            "[Dispatch] %s -> NULL (no implementation found)",
            fn_name
        );

        ChainLog(msg);
    }
}

// ─── CreateDXGIFactory ───────────────────────────────────────────────────────

extern "C" HRESULT WINAPI CreateDXGIFactory(
    REFIID riid,
    void** ppFactory
)
{
    EnsureInitialized();

    auto fn = reinterpret_cast<PFN_CreateDXGIFactory>(
        GetChainedProc("CreateDXGIFactory")
    );

    LogDispatch(
        "CreateDXGIFactory",
        reinterpret_cast<FARPROC>(fn)
    );

    if (!fn) {
        return DXGI_ERROR_UNSUPPORTED;
    }

    return fn(riid, ppFactory);
}

// ─── CreateDXGIFactory1 ──────────────────────────────────────────────────────

extern "C" HRESULT WINAPI CreateDXGIFactory1(
    REFIID riid,
    void** ppFactory
)
{
    EnsureInitialized();

    auto fn = reinterpret_cast<PFN_CreateDXGIFactory1>(
        GetChainedProc("CreateDXGIFactory1")
    );

    LogDispatch(
        "CreateDXGIFactory1",
        reinterpret_cast<FARPROC>(fn)
    );

    if (!fn) {
        return DXGI_ERROR_UNSUPPORTED;
    }

    return fn(riid, ppFactory);
}

// ─── CreateDXGIFactory2 ──────────────────────────────────────────────────────

extern "C" HRESULT WINAPI CreateDXGIFactory2(
    UINT Flags,
    REFIID riid,
    void** ppFactory
)
{
    EnsureInitialized();

    auto fn = reinterpret_cast<PFN_CreateDXGIFactory2>(
        GetChainedProc("CreateDXGIFactory2")
    );

    LogDispatch(
        "CreateDXGIFactory2",
        reinterpret_cast<FARPROC>(fn)
    );

    if (!fn) {
        return DXGI_ERROR_UNSUPPORTED;
    }

    return fn(Flags, riid, ppFactory);
}

// ─── DXGIDeclareAdapterRemovalSupport ───────────────────────────────────────

extern "C" HRESULT WINAPI DXGIDeclareAdapterRemovalSupport()
{
    EnsureInitialized();

    auto fn = reinterpret_cast<PFN_DXGIDeclareAdapterRemovalSupport>(
        GetChainedProc("DXGIDeclareAdapterRemovalSupport")
    );

    if (!fn) {
        return DXGI_ERROR_UNSUPPORTED;
    }

    return fn();
}

// ─── DXGIGetDebugInterface1 ──────────────────────────────────────────────────

extern "C" HRESULT WINAPI DXGIGetDebugInterface1(
    UINT Flags,
    REFIID riid,
    void** pDebug
)
{
    EnsureInitialized();

    auto fn = reinterpret_cast<PFN_DXGIGetDebugInterface1>(
        GetChainedProc("DXGIGetDebugInterface1")
    );

    if (!fn) {
        return DXGI_ERROR_UNSUPPORTED;
    }

    return fn(Flags, riid, pDebug);
}

// ─── 내부/비공개 export 전달 ─────────────────────────────────────────────────

#define DEFINE_PASSTHROUGH(name)                                  \
    extern "C" void WINAPI name()                                 \
    {                                                             \
        EnsureInitialized();                                      \
        static FARPROC s_fn = GetChainedProc(#name);             \
        if (s_fn) {                                               \
            reinterpret_cast<PFN_Passthrough>(s_fn)();           \
        }                                                         \
    }

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
