#include "inject.h"

bool ManualMap(HANDLE hProcess, const wchar_t* szDllFile) {
	BYTE* pSrcData = nullptr;
	IMAGE_NT_HEADERS* pOldNtHeader = nullptr;
	IMAGE_OPTIONAL_HEADER* pOldOptHeader = nullptr;
	IMAGE_FILE_HEADER* pOldFileHeader = nullptr;
	BYTE* pTargetBase = nullptr; // Allocated memory for the DLL images in the target process

	if (GetFileAttributes(szDllFile) == INVALID_FILE_ATTRIBUTES) { // DLL file does not exist
		std::wcout << "- DLL file '" << szDllFile << "' does not exist." << std::endl;
		return false;
	}
	std::ifstream file{ szDllFile, std::ios::binary | std::ios::ate };
	if (file.fail()) {
		std::cout << "- Failed to open DLL file!" << std::endl;
		return false;
	}
	const auto file_size = file.tellg();
	if (file_size < 0x1000) {
		std::cout << "- File size is invalid!" << std::endl;
		file.close();
		return false;
	}

	// Allocate memory to store DLL file data
	pSrcData = new BYTE[static_cast<uintptr_t>(file_size)];
	if (!pSrcData) {
		std::cout << "- Failed to allocate memory for source data buffer." << std::endl;
		file.close();
		return false;
	}

	file.seekg(0, std::ios::beg); // Seek to the start of the file
	file.read(reinterpret_cast<char*>(pSrcData), file_size); // Read into the buffer (cast to char*)
	file.close();

	if (reinterpret_cast<IMAGE_DOS_HEADER*>(pSrcData)->e_magic != 0x5A4D) { // "MZ", magic byte for PE executables
		std::cout << "- Invalid magic bytes in DOS header!" << std::endl;
		delete[] pSrcData;
		return false;
	}

	pOldNtHeader = reinterpret_cast<IMAGE_NT_HEADERS*>(pSrcData + reinterpret_cast<IMAGE_DOS_HEADER*>(pSrcData)->e_lfanew);
	pOldOptHeader = &pOldNtHeader->OptionalHeader;
	pOldFileHeader = &pOldNtHeader->FileHeader;

#ifdef _WIN64
	if (pOldFileHeader->Machine != IMAGE_FILE_MACHINE_AMD64) {
#else 
	if (pOldFileHeader->Machine != IMAGE_FILE_MACHINE_I386) {
#endif
		std::cout << "- DLL is compiled for the wrong platform!" << std::endl;
		delete[] pSrcData;
		return false;
	}

	// Allocate memory region in the target process
	void* preferred_base = reinterpret_cast<void*>(pOldOptHeader->ImageBase);
	pTargetBase = reinterpret_cast<BYTE*>(VirtualAllocEx(hProcess,
		preferred_base, pOldOptHeader->SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
	std::cout << "> Attempting to allocate memory for DLL at the preferred base address " << preferred_base << "..." << std::endl;

	if (!pTargetBase) { // If we couldn't allocate memory at the preferred base, do it somewhere else
		std::cout << "> Could not allocate memory for DLL at the preferred base address. Using a random address..." << std::endl;
		SetLastError(0x0);

		// Will be relocated in shellcode later anyways
		pTargetBase = reinterpret_cast<BYTE*>(VirtualAllocEx(hProcess,
			nullptr, pOldOptHeader->SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
		
		if (!pTargetBase) { // Could not allocate any memory in the process at all
			std::cout << "- Failed to allocate memory for DLL in target process!" << std::endl;
			delete[] pSrcData;
			return false;
		}
		std::cout << "> Successfully allocated memory for DLL at " << static_cast<void*>(pTargetBase) << std::endl;
	}

	// PE file format:
	// https://upload.wikimedia.org/wikipedia/commons/1/1b/Portable_Executable_32_bit_Structure_in_SVG_fixed.svg
	MANUAL_MAPPING_DATA data{0};
	data.pLoadLibraryA = reinterpret_cast<f_LoadLibraryA>(LoadLibraryA);
	data.pGetProcAddress = reinterpret_cast<f_GetProcAddress>(GetProcAddress);
	data.hModule = nullptr;

	std::cout << "> Copying DLL sections (" << pOldFileHeader->NumberOfSections << " total) into memory..." << std::endl;

	// Iterate through all sections and copy them into the target process
	auto* pSectionHeader = IMAGE_FIRST_SECTION(pOldNtHeader);
	for (size_t i = 0; i < pOldFileHeader->NumberOfSections; i++, pSectionHeader++) {
		if (pSectionHeader->SizeOfRawData) { // Only copy data sections, not uninitialized ones
			bool result = WriteProcessMemory(hProcess, pTargetBase + pSectionHeader->VirtualAddress,
				pSrcData + pSectionHeader->PointerToRawData, pSectionHeader->SizeOfRawData, nullptr);
			if (!result) {
				std::cout << "- Failed to copy section into " << static_cast<void*>(pTargetBase + pSectionHeader->VirtualAddress) << std::endl;
				delete[] pSrcData;
				VirtualFreeEx(hProcess, pTargetBase, 0, MEM_RELEASE);
				return false;
			}
		}
	}
	std::cout << "> Successfully copied DLL sections into allocated memory region" << std::endl;

	std::cout << "> Writing PE header into process..." << std::endl;

	// Copy the PE headers into the target process
	memcpy(pSrcData, &data, sizeof(data)); // Overwrites the first few bytes of the PE header, but they aren't used anyways
	WriteProcessMemory(hProcess, pTargetBase, pSrcData, 0x1000, nullptr); // Writing the header into the target process

	delete[] pSrcData;

	// Allocate memory page for shellcode
	void* pShellcode = VirtualAllocEx(hProcess, nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!pShellcode) {
		std::cout << "- Failed to allocate memory in target process for shellcode" << std::endl;
		VirtualFreeEx(hProcess, pTargetBase, 0, MEM_RELEASE);
		return false;
	}

	std::cout << "> Successfully allocated memory at " << pShellcode << " for shellcode" << std::endl;
	std::cout << "> Writing shellcode into memory region..." << std::endl;

	WriteProcessMemory(hProcess, pShellcode, Shellcode, 0x1000, nullptr);

	std::cout << "> Executing shellcode..." << std::endl;

	// Execute the shellcode
	HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(pShellcode), pTargetBase, 0, nullptr);
	if (!hThread) {
		std::cout << "- Failed to create thread to run shellcode!" << std::endl;
		VirtualFreeEx(hProcess, pTargetBase, 0, MEM_RELEASE);
		VirtualFreeEx(hProcess, pShellcode, 0, MEM_RELEASE);
		return false;
	}

	// Wait till the thread finishes execution
	DWORD dwExitCode;
	GetExitCodeThread(hThread, &dwExitCode);
	while (dwExitCode == 259) {
		GetExitCodeThread(hThread, &dwExitCode);
		Sleep(20);
	}
	std::cout << "> Shellcode exited with code " << dwExitCode << std::endl;

	CloseHandle(hThread);

	std::cout << "> Confirming DLL module address..." << std::endl;

	// Make sure the shellcode successfully ran
	MANUAL_MAPPING_DATA loaded_data;
	ReadProcessMemory(hProcess, pTargetBase, &loaded_data, sizeof(loaded_data), nullptr);
	HMODULE hDllModule = loaded_data.hModule;
	std::cout << "> DLL module loaded at base address " << static_cast<void*>(hDllModule) << std::endl;
	VirtualFreeEx(hProcess, pShellcode, 0, MEM_RELEASE);

	return true;
}

#define RELOC_FLAG32(RelInfo) ((RelInfo >> 0x0c) == IMAGE_REL_BASED_HIGHLOW)
#define RELOC_FLAG64(RelInfo) ((RelInfo >> 0x0c) == IMAGE_REL_BASED_DIR64)

#ifdef _WIN64
#define RELOC_FLAG RELOC_FLAG64
#else
#define RELOC_FLAG RELOC_FLAG32
#endif

DWORD __stdcall Shellcode(MANUAL_MAPPING_DATA* pData) {
	if (!pData) {
		return 1;
	}

	BYTE* pBase = reinterpret_cast<BYTE*>(pData);
	auto* pOpt = &reinterpret_cast<IMAGE_NT_HEADERS*>(
		pBase + reinterpret_cast<IMAGE_DOS_HEADER*>(pData)->e_lfanew)->OptionalHeader;

	auto _LoadLibraryA = pData->pLoadLibraryA;
	auto _GetProcAddress = pData->pGetProcAddress;
	auto _DllMain = reinterpret_cast<f_DLL_ENTRY_POINT>(pBase + pOpt->AddressOfEntryPoint);

	BYTE* LocationDelta = pBase - pOpt->ImageBase; // Calculate delta for section relocation
	if (LocationDelta) { // Relocation needed
		if (!pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size) { // Some executables can't be relocated
			return 2;
		}
		auto* pRelocData = reinterpret_cast<IMAGE_BASE_RELOCATION*>(
			pBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
		while (pRelocData->VirtualAddress) {
			/*
			 * typedef struct _IMAGE_BASE_RELOCATION {
			 *     DWORD   VirtualAddress;
			 *	   DWORD   SizeOfBlock;
			 *     // WORD    TypeOffset[1];
			 * } IMAGE_BASE_RELOCATION;
			 *
			 * We want to find the length of the TypeOffset array given the size of the structure
			 * including the TypeOffset array. SizeOfBlock - SizeOfStructure gives the size of
			 * the TypeOffset array. Divide the size of the array by the size of the data type
			 * to get array length.
			 */
			auto number_of_entries = (pRelocData->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
			WORD* pRelativeInfo = reinterpret_cast<WORD*>(pRelocData + 1);
			for (size_t i = 0; i < number_of_entries; i++, pRelativeInfo++) {
				if (RELOC_FLAG(*pRelativeInfo)) { // If we need to relocate
					UINT_PTR* pPatch = reinterpret_cast<UINT_PTR*>(
						pBase + pRelocData->VirtualAddress + ((*pRelativeInfo) & 0xFFF)); // lower 12 bits store address
					*pPatch += reinterpret_cast<UINT_PTR>(LocationDelta);
				}
			}
			pRelocData = reinterpret_cast<IMAGE_BASE_RELOCATION*>(
				reinterpret_cast<BYTE*>(pRelocData) + pRelocData->SizeOfBlock);
		}
	}

	// Fix imports
	if (pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size) {
		auto* pImportDescr = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(
			pBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
		while (pImportDescr->Name) {
			char* szMod = reinterpret_cast<char*>(pBase + pImportDescr->Name);
			HINSTANCE hDll = _LoadLibraryA(szMod);
			ULONG_PTR* pThunkRef = reinterpret_cast<ULONG_PTR*>(pBase + pImportDescr->OriginalFirstThunk);
			ULONG_PTR* pFuncRef = reinterpret_cast<ULONG_PTR*>(pBase + pImportDescr->FirstThunk);
			if (!pThunkRef) {
				pThunkRef = pFuncRef;
			}

			for (; *pThunkRef; pThunkRef++, pFuncRef++) {
				// Functions can be imported by name or ordinal
				if (IMAGE_SNAP_BY_ORDINAL(*pThunkRef)) {
					// Take lower 2 bytes (16 bits) to get the ordinal of the function
					*pFuncRef = _GetProcAddress(hDll, reinterpret_cast<char*>(*pThunkRef & 0xFFFF));
				} else {
					// Resolve the pointer to a structure, and use the name of the function
					auto* pImport = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(pBase + (*pThunkRef));
					*pFuncRef = _GetProcAddress(hDll, pImport->Name);
				}
			}
			pImportDescr++;
		}
	}

	// Execute TLS (thread-local storage) callbacks
	if (pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size) {
		auto* pTLS = reinterpret_cast<IMAGE_TLS_DIRECTORY*>(
			pBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);
		auto* pCallback = reinterpret_cast<PIMAGE_TLS_CALLBACK*>(pTLS->AddressOfCallBacks);
		for (; pCallback && *pCallback; pCallback++) {
			(*pCallback)(pBase, DLL_PROCESS_ATTACH, nullptr);
		}
	}
	HINSTANCE hDllModule = reinterpret_cast<HINSTANCE>(pBase);
	_DllMain(hDllModule, DLL_PROCESS_ATTACH, nullptr);
	pData->hModule = hDllModule;
	return 0;
}
