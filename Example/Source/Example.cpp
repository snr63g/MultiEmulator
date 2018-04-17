#include <Windows.h>

#include "..\..\MultiEmulator\Source\Emulators\RevEmu2013.h"

class ISteamUser;

using TInitiateGameConnection = int(__fastcall *)(ISteamUser* self, int, void* pData, int cbMaxData, long long steamID, int unIPServer, short usPortServer, int bSecure);
using TSteamUser = ISteamUser*(*)();

TInitiateGameConnection pfnInitiateGameConnection = nullptr;

int __fastcall hkInitiateGameConnection(ISteamUser* self, int, void* pData, int cbMaxData, long long steamID, int unIPServer, short usPortServer, int bSecure)
{
	return GenerateRevEmu2013(pData, 3333333);

	// return pfnInitiateGameConnection(self, 0, pData, cbMaxData, steamID, unIPServer, usPortServer, bSecure);
}

DWORD WINAPI Init(LPVOID lpThreadParameter)
{
	HMODULE hSteamAPI;

	/* Wait for steam_api.dll load. */
	while (true)
	{
		hSteamAPI = GetModuleHandle("steam_api.dll");

		if (hSteamAPI)
			break;

		Sleep(500);
	}

	auto pfnSteamUser = (TSteamUser)GetProcAddress(hSteamAPI, "SteamUser");
	if (!pfnSteamUser)
		return FALSE;

	auto pSteamUser = pfnSteamUser();
	if (!pSteamUser)
		return FALSE;

	/* Get pointer to InitiateGameConnection method address in virtual table of ISteamUser interface. */
	auto pfn = &((void **)(*(void **)pSteamUser))[3];

	/* Save original InitiateGameConnection method code address. */
	pfnInitiateGameConnection = (TInitiateGameConnection)*pfn;

	/* Write own InitiateGameConnection function address to virtual table. */
	DWORD oldProtect;
	VirtualProtect(pfn, sizeof(void*), PAGE_READWRITE, &oldProtect);
	*pfn = hkInitiateGameConnection;
	VirtualProtect(pfn, sizeof(void*), oldProtect, nullptr);

	return TRUE;
}

BOOL WINAPI DllMain(_In_ HINSTANCE hinstDLL, _In_ DWORD fdwReason, _In_ LPVOID lpvReserved)
{
	UNREFERENCED_PARAMETER(lpvReserved);

	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		/* Create mutex to prevent multiple code calling. */
		char sMutex[256];
		sprintf_s(sMutex, "MultiEmuExample%08X", (int)hinstDLL);
		if (CreateMutex(nullptr, false, sMutex) == (HANDLE)ERROR_ALREADY_EXISTS)
			return TRUE;

		CreateThread(nullptr, 0, Init, nullptr, 0, nullptr);
	}

	return TRUE;
}