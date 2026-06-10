#include "CrashHandler.h"

#ifdef _WIN32

#include <windows.h>
#include <dbghelp.h>
#include <cstdio>

// Note: MinGW builds carry DWARF debug info, not PDBs — analyze these dumps
// with the crash address against objdump/addr2line on the matching exe.

namespace
{
// Resolved at install() time so the exception filter does no allocation.
wchar_t g_dumpDir[MAX_PATH] = L"";

LONG WINAPI writeMinidump(EXCEPTION_POINTERS *info)
{
    if (g_dumpDir[0] == L'\0')
        return EXCEPTION_CONTINUE_SEARCH;

    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t path[MAX_PATH];
    _snwprintf(path, MAX_PATH, L"%s\\crash-%04u%02u%02u-%02u%02u%02u.dmp",
               g_dumpDir, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    path[MAX_PATH - 1] = L'\0';

    HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return EXCEPTION_CONTINUE_SEARCH;

    MINIDUMP_EXCEPTION_INFORMATION mei;
    mei.ThreadId = GetCurrentThreadId();
    mei.ExceptionPointers = info;
    mei.ClientPointers = FALSE;

    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file,
                      MiniDumpWithIndirectlyReferencedMemory, &mei, nullptr, nullptr);
    CloseHandle(file);
    return EXCEPTION_CONTINUE_SEARCH;  // let Windows show its crash dialog too
}
} // namespace

void CrashHandler::install()
{
    wchar_t localAppData[MAX_PATH];
    DWORD len = GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH);
    if (len == 0 || len >= MAX_PATH)
        return;

    // Matches QStandardPaths::AppLocalDataLocation for this app
    _snwprintf(g_dumpDir, MAX_PATH, L"%s\\EtherCATAliasGUI\\EtherCAT Alias Tool",
               localAppData);
    g_dumpDir[MAX_PATH - 1] = L'\0';

    wchar_t parent[MAX_PATH];
    _snwprintf(parent, MAX_PATH, L"%s\\EtherCATAliasGUI", localAppData);
    parent[MAX_PATH - 1] = L'\0';
    CreateDirectoryW(parent, nullptr);
    CreateDirectoryW(g_dumpDir, nullptr);

    SetUnhandledExceptionFilter(writeMinidump);
}

#else

void CrashHandler::install() {}

#endif
