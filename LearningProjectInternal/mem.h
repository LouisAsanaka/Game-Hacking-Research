#pragma once

#include <windows.h>
#include <iostream>
#include <TlHelp32.h>
#include <vector>

namespace mem {

	void PatchEx(BYTE* dst, BYTE* src, unsigned int size, HANDLE hProcess);
	void NopEx(BYTE* dst, unsigned int size, HANDLE hProcess);

	void Patch(BYTE* dst, BYTE* src, unsigned int size);
	void Nop(BYTE* dst, unsigned int size);

	uintptr_t FindDMAAddy(uintptr_t ptr, std::vector<unsigned int> offsets);
};