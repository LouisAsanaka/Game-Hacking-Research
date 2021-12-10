#pragma once

#include <windows.h>
#include <iostream>
#include <TlHelp32.h>
#include <vector>

DWORD GetProcID(const wchar_t* procName);

uintptr_t GetModuleBaseAddress(DWORD procId, const wchar_t* moduleName);