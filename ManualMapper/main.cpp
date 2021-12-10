#include "inject.h"

DWORD GetProcessId(const wchar_t* procName) {
    DWORD procId = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 procEntry;
        procEntry.dwSize = sizeof(procEntry);

        if (Process32First(hSnap, &procEntry)) {
            do {
                if (!_wcsicmp(procEntry.szExeFile, procName)) {
                    procId = procEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnap, &procEntry));
        }
    }
    CloseHandle(hSnap);
    return procId;
}

int wmain(int argc, wchar_t* argv[]) {
    if (argc != 3) {
        std::cout << "Usage: ManualMapper.exe [DLL PATH] [TARGET PROCESS NAME]" << std::endl;
        getchar();
        return 0;
    }
    wchar_t* szDllFile = argv[1];
    wchar_t* szProcessName = argv[2];

    std::wcout << "> Looking for process with name '" << szProcessName << "'..." << std::endl;

    DWORD dwProcessId = 0;
    while (dwProcessId == 0) {
        dwProcessId = GetProcessId(szProcessName);
        Sleep(50);
    }
    std::wcout << "> Found process! Opening handle to process '" << szProcessName << "' with PID " << dwProcessId << std::endl;

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, dwProcessId);
    if (!hProcess || hProcess == INVALID_HANDLE_VALUE) {
        std::cout << "- Failed to open handle to process (" << GetLastError() << "). Exiting..." << std::endl;
        getchar();
        return 0;
    }
    std::wcout << "> Mapping DLL '" << szDllFile << "' into process PID " << dwProcessId << std::endl;
    if (!ManualMap(hProcess, szDllFile)) {
        CloseHandle(hProcess);
        std::cout << "- Failed to manually map DLL into process. Exiting..." << std::endl;
        getchar();
        return 0;
    }
    CloseHandle(hProcess);
    std::cout << "> Successfully mapped DLL into process." << std::endl;
    getchar();
    return 0;
}