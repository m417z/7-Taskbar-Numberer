#include <windows.h>
#include "wnd_proc.h"
#include "functions.h"

// superglobals
HINSTANCE hDllInst;
int nWinVersion;
WORD nExplorerQFE;
UINT uPrivateMsg;
int nViewOption;
BOOL bWinKeyDown;
DWORD dwTaskbarThreadId;
HWND hTaskbarWnd, hTaskSwWnd, hTaskListWnd, hTrayNotifyWnd, hTrayToolbarWnd, hTrayTemporaryToolbarWnd;
LONG_PTR lpTaskbarLongPtr, lpTaskSwLongPtr, lpTaskListLongPtr, lpTrayNotifyLongPtr, lpTrayTemporaryToolbarLongPtr;

BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch(fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		hDllInst = hinstDLL;
		break;

	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}

// Exported

BOOL __stdcall Init(void *param)
{
	static volatile BOOL bInitCalled = FALSE;

	// Make sure this function is called only once
	if(InterlockedExchange((long *)&bInitCalled, TRUE))
		return FALSE;

	// Check OS version
	nWinVersion = WIN_VERSION_UNSUPPORTED;

	VS_FIXEDFILEINFO *pFixedFileInfo = GetModuleVersionInfo(NULL, NULL);
	if(pFixedFileInfo)
	{
		// HIWORD(pFixedFileInfo->dwFileVersionMS); // Major version
		// LOWORD(pFixedFileInfo->dwFileVersionMS); // Minor version
		// HIWORD(pFixedFileInfo->dwFileVersionLS); // Build number
		// LOWORD(pFixedFileInfo->dwFileVersionLS); // QFE

		switch(HIWORD(pFixedFileInfo->dwFileVersionMS)) // Major version
		{
		case 6:
			switch(LOWORD(pFixedFileInfo->dwFileVersionMS)) // Minor version
			{
			case 1:
				nWinVersion = WIN_VERSION_7;
				break;

			case 2:
				nWinVersion = WIN_VERSION_8;
				break;

			case 3:
				if(LOWORD(pFixedFileInfo->dwFileVersionLS) < 17000) // QFE
					nWinVersion = WIN_VERSION_81;
				else
					nWinVersion = WIN_VERSION_811;
				break;

			case 4:
				nWinVersion = WIN_VERSION_10_T1;
				break;
			}
			break;

		case 10:
			if(HIWORD(pFixedFileInfo->dwFileVersionLS) <= 10240) // Build number
				nWinVersion = WIN_VERSION_10_T1;
			else if(HIWORD(pFixedFileInfo->dwFileVersionLS) <= 10586)
				nWinVersion = WIN_VERSION_10_T2;
			else if(HIWORD(pFixedFileInfo->dwFileVersionLS) <= 14393)
				nWinVersion = WIN_VERSION_10_R1;
			else if(HIWORD(pFixedFileInfo->dwFileVersionLS) <= 15063)
				nWinVersion = WIN_VERSION_10_R2;
			else if(HIWORD(pFixedFileInfo->dwFileVersionLS) <= 16299)
				nWinVersion = WIN_VERSION_10_R3;
			else if(HIWORD(pFixedFileInfo->dwFileVersionLS) <= 17134)
				nWinVersion = WIN_VERSION_10_R4;
			else if(HIWORD(pFixedFileInfo->dwFileVersionLS) <= 17763)
				nWinVersion = WIN_VERSION_10_R5;
			else
				nWinVersion = WIN_VERSION_10_19H1;
			break;
		}
	}

	if(nWinVersion == WIN_VERSION_UNSUPPORTED)
		return FALSE;

	nExplorerQFE = LOWORD(pFixedFileInfo->dwFileVersionLS);

	nViewOption = *(int *)param;

	return WndProcInit();
}

void __stdcall Exit()
{
	WndProcExit();
	WndProcWaitTillDone();
}
