#include "hook.h"
#include "mem.h"

bool Detour32(BYTE* src, BYTE* dst, const uintptr_t len) {
	if (len < 5) {
		return false;
	}
	DWORD currProtect;
	VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &currProtect);

	uintptr_t relativeAddress = dst - src - 5; // length of jmp instruction is 5 bytes
	*src = 0xE9; // jmp instruction
	*(uintptr_t*)(src + 1) = relativeAddress;

	VirtualProtect(src, len, currProtect, &currProtect);
	return true;
}

BYTE* TrampHook32(BYTE* src, BYTE* dst, const uintptr_t len) {
	if (len < 5) {
		return nullptr;
	}

	// Create the gateway
	BYTE* gateway = (BYTE*)VirtualAlloc(0, len + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

	// Write the stolen bytes to the gateway
	memcpy_s(gateway, len, src, len);

	// Get the gateway to the destination address
	uintptr_t gatewayRelativeAddr = src - gateway - 5;

	// Add jmp opcode to the end of the gateway
	*(gateway + len) = 0xE9;

	// Write the address of the gateway to the jmp
	*(uintptr_t*)((uintptr_t)gateway + len + 1) = gatewayRelativeAddr;

	// Perform the detour
	Detour32(src, dst, len);

	return gateway;
}

Hook::Hook(BYTE* src, BYTE* dst, BYTE* ptrToGatewayPtr, uintptr_t len) : 
	src{ src }, dst{ dst }, ptrToGatewayFnPtr{ ptrToGatewayPtr }, len{len} {}

Hook::Hook(const char* exportName, const char* modName, BYTE* dst, BYTE* ptrToGatewayPtr, uintptr_t len) {
	HMODULE hModule = GetModuleHandleA(modName);
	this->src = (BYTE*)GetProcAddress(hModule, exportName);
	this->dst = dst;
	this->ptrToGatewayFnPtr = ptrToGatewayPtr;
	this->len = len;
}

void Hook::Enable() {
	memcpy(originalBytes, src, len);
	*(uintptr_t*)ptrToGatewayFnPtr = (uintptr_t)TrampHook32(src, dst, len);
	bStatus = true;
}

void Hook::Disable() {
	mem::Patch(src, originalBytes, len);
	VirtualFree((void*)*(uintptr_t*) ptrToGatewayFnPtr, 0, MEM_RELEASE);
	bStatus = false;
}

void Hook::Toggle() {
	if (!bStatus) {
		Enable();
	} else {
		Disable();
	}
}
