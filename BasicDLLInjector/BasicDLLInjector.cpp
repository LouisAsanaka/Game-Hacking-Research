#include <iostream>
#include <string>
#include <windows.h>
#include <TlHelp32.h>

DWORD GetProcId(const wchar_t* procName) {
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

int main() {
    //std::string dllPathString;

    //std::cout << "Enter path to DLL: ";
    //std::getline(std::cin, dllPathString);
    //std::wstring dllPathWString = std::wstring(dllPathString.begin(), dllPathString.end());

    const char* dllPath = "DLL_PATH_HERE";
    const wchar_t* procName = L"ac_client.exe";

    DWORD procId = 0;
    while (procId == 0) {
        procId = GetProcId(procName);
        Sleep(50);
    }
    std::cout << "Found process! Opening process..." << std::endl;

    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, 0, procId);
    if (hProc && hProc != INVALID_HANDLE_VALUE) {
        std::cout << "Injecting..." << std::endl;
        void* loc = VirtualAllocEx(hProc, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (loc != 0) {
            std::cout << "Writing process memory..." << std::endl;
            WriteProcessMemory(hProc, loc, dllPath, strlen(dllPath) + 1, 0);
        }

        HANDLE hThread = CreateRemoteThread(hProc, 0, 0, (LPTHREAD_START_ROUTINE) LoadLibraryA, loc, 0, 0);

        if (hThread) {
            std::cout << "Successfully injected!" << std::endl;
            CloseHandle(hThread);
        } else {
            std::cout << "Failed to inject." << std::endl;
        }
    }
    if (hProc) {
        CloseHandle(hProc);
    }
    getchar();
    return 0;
}