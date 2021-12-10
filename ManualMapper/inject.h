#pragma once

#include <Windows.h>
#include <winnt.h>
#include <TlHelp32.h>
#include <iostream>
#include <fstream>

using f_LoadLibraryA = HMODULE(WINAPI*)(LPCSTR lpLibraryFilename);
using f_GetProcAddress = UINT_PTR(WINAPI*)(HMODULE hModule, LPCSTR lpProcessName);
using f_DLL_ENTRY_POINT = BOOL(WINAPI*)(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID* lpReserved);

struct MANUAL_MAPPING_DATA {
	f_LoadLibraryA pLoadLibraryA;
	f_GetProcAddress pGetProcAddress;
	HMODULE hModule;
};

bool ManualMap(HANDLE hProcess, const wchar_t* szDllFile);
DWORD __stdcall Shellcode(MANUAL_MAPPING_DATA* pMappingData);
