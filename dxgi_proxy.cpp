/*
 * ETS2 DXGI Chain Loader
 * ----------------------
 * Windows x64 DXGI 프록시 체인 로더
 * SnowyMoon.dll + ReShade64.dll 동시 로드를 위한 프록시 dxgi.dll
 *
 * 구조:
 *   ETS2 → proxy dxgi.dll (이 파일)
 *          → SnowyMoon.dll (primary)
 *          → ReShade64.dll (secondary)
 *          → system dxgi.dll (fallback)
 *
 * 라이선스: 퍼블릭 도메인 / 0BSD
 * 주의: 메모리 패치, 라이선스 우회, 내부 코드 변조 없음
 *       단순 DLL 로딩 및 DXGI export 전달만 수행
 */

#include <Windows.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>
#include <mutex>
#include <string>
#include <fstream>
#include <cstdio>

// ─── 전역 상태 ────────────────────────────────────────────────────────────────

static HMODULE  g_system_dxgi   = nullptr; // %SystemRoot%\System32\dxgi.dll
static HMODULE  g_primary        = nullptr; // SnowyMoon.dll (설정 가능)
static HMODULE  g_secondary      = nullptr; // ReShade64.dll (설정 가능)
static std::once_flag g_init_flag;
static std::ofstream  g_log;
static std::wstring   g_dll_dir;            // 이 DLL이 있는 폴더 (끝에 '\\' 포함)
static std::wstring   g_primary_name   = L"SnowyMoon.dll";
static std::wstring   g_secondary_name = L"ReShade64.dll";

// ─── 로그 ─────────────────────────────────────────────────────────────────────

void ChainLog(const char* msg)
{
    if (g_log.is_open()) {
        g_log << msg << "\n";
        g_log.flush();
    }
}

static void ChainLogf(const char* fmt, ...)
{
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, args);
    va_end(args);
    ChainLog(buf);
}

// ─── 이 DLL 자신의 경로로부터 폴더 경로 계산 ────────────────────────────────

static std::wstring GetSelfDirectory()
{
    HMODULE self = nullptr;
    // 이 함수 주소를 기준으로 자신의 HMODULE 획득
    GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&GetSelfDirectory),
        &self);

    wchar_t path[MAX_PATH] = {};
    GetModuleFileNameW(self, path, MAX_PATH);

    std::wstring dir(path);
    const size_t slash = dir.find_last_of(L"\\/");
    if (slash != std::wstring::npos)
        dir.resize(slash + 1); // 끝에 '\\' 포함
    return dir;
}

// ─── INI 설정 읽기 ───────────────────────────────────────────────────────────

static void ReadConfig()
{
    const std::wstring ini_path = g_dll_dir + L"dxgi_chain.ini";
    wchar_t buf[MAX_PATH] = {};

    GetPrivateProfileStringW(L"chain", L"primary",   L"SnowyMoon.dll", buf, MAX_PATH, ini_path.c_str());
    g_primary_name = buf;

    GetPrivateProfileStringW(L"chain", L"secondary", L"ReShade64.dll", buf, MAX_PATH, ini_path.c_str());
    g_secondary_name = buf;

    ChainLogf("[Config] primary   = %ls", g_primary_name.c_str());
    ChainLogf("[Config] secondary = %ls", g_secondary_name.c_str());
}

// ─── DLL 로드 헬퍼 (중복 방지) ───────────────────────────────────────────────

static HMODULE LoadOnce(const std::wstring& full_path, const char* label)
{
    // 이미 프로세스에 로드된 경우 재사용
    HMODULE existing = GetModuleHandleW(full_path.c_str());
    if (existing) {
        ChainLogf("[Load] %s already loaded (handle=%p)", label, existing);
        return existing;
    }

    HMODULE h = LoadLibraryW(full_path.c_str());
    if (h) {
        ChainLogf("[Load] %s loaded OK (handle=%p)", label, h);
    } else {
        const DWORD err = GetLastError();
        ChainLogf("[Load] %s FAILED (GetLastError=%lu)", label, err);
    }
    return h;
}

// ─── 초기화 (once) ───────────────────────────────────────────────────────────

static void InitOnce()
{
    // 1. 폴더 경로 확정
    g_dll_dir = GetSelfDirectory();

    // 2. 로그 파일 열기
    {
        std::string log_path;
        log_path.reserve(g_dll_dir.size() + 16);
        for (wchar_t c : g_dll_dir) log_path += static_cast<char>(c);
        log_path += "dxgi_chain.log";
        g_log.open(log_path, std::ios::out | std::ios::trunc);
    }
    ChainLog("[ETS2DxgiChainLoader] Proxy chain loader starting");
    ChainLogf("[ETS2DxgiChainLoader] DLL directory: %ls", g_dll_dir.c_str());

    // 3. 설정 파일 읽기
    ReadConfig();

    // 4. 시스템 dxgi.dll 로드 (재귀 방지: 절대 경로 사용)
    {
        wchar_t sys_dir[MAX_PATH] = {};
        GetSystemDirectoryW(sys_dir, MAX_PATH);
        const std::wstring sys_dxgi = std::wstring(sys_dir) + L"\\dxgi.dll";
        ChainLogf("[Init] Loading system dxgi: %ls", sys_dxgi.c_str());
        g_system_dxgi = LoadOnce(sys_dxgi, "system dxgi.dll");
        if (!g_system_dxgi) {
            ChainLog("[Init] CRITICAL: cannot load system dxgi.dll — proxy will fail");
        }
    }

    // 5. Primary DLL (SnowyMoon) 로드
    {
        const std::wstring path = g_dll_dir + g_primary_name;
        ChainLogf("[Init] Loading primary: %ls", path.c_str());
        g_primary = LoadOnce(path, "SnowyMoon.dll (primary)");
    }

    // 6. Secondary DLL (ReShade64) 로드
    {
        const std::wstring path = g_dll_dir + g_secondary_name;
        ChainLogf("[Init] Loading secondary: %ls", path.c_str());
        g_secondary = LoadOnce(path, "ReShade64.dll (secondary)");
    }

    ChainLog("[Init] Initialization complete");
}

// ─── 공개 API ────────────────────────────────────────────────────────────────

void EnsureInitialized()
{
    std::call_once(g_init_flag, InitOnce);
}

FARPROC GetChainedProc(const char* name)
{
    EnsureInitialized();

    FARPROC proc = nullptr;

    // 우선순위 1: primary (SnowyMoon)
    if (g_primary) {
        proc = GetProcAddress(g_primary, name);
        if (proc) {
            // (매 호출마다 로그하면 너무 많으므로 생략)
            return proc;
        }
    }

    // 우선순위 2: secondary (ReShade64)
    if (g_secondary) {
        proc = GetProcAddress(g_secondary, name);
        if (proc) return proc;
    }

    // 우선순위 3: system dxgi
    if (g_system_dxgi) {
        proc = GetProcAddress(g_system_dxgi, name);
    }

    return proc;
}

// ─── DllMain ─────────────────────────────────────────────────────────────────

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*reserved*/)
{
    if (reason == DLL_PROCESS_ATTACH) {
        // DllMain 내에서 무거운 작업 금지 — 첫 export 호출 시 초기화
        DisableThreadLibraryCalls(nullptr);
    }
    return TRUE;
}
