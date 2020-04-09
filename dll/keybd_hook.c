#include "keybd_hook.h"

static DWORD WINAPI KeybdHookThread(void *pParameter);
static LRESULT CALLBACK LowLevelKeybdProc(int nCode, WPARAM wParam, LPARAM lParam);

// superglobals
extern HINSTANCE hDllInst;
extern BOOL bWinKeyDown;
extern HWND hTaskListWnd;

static volatile HANDLE hKeybdHookThread;
static DWORD dwKeybdHookThreadId;
static HHOOK hLowLevelKeybdHook;

BOOL KeybdHook_Init()
{
	HANDLE hThreadReadyEvent;
	BOOL bSuccess;

	if(hKeybdHookThread)
		return TRUE;

	bSuccess = FALSE;

	hThreadReadyEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if(hThreadReadyEvent)
	{
		hKeybdHookThread = CreateThread(NULL, 0, KeybdHookThread, (void *)hThreadReadyEvent, CREATE_SUSPENDED, &dwKeybdHookThreadId);
		if(hKeybdHookThread)
		{
			SetThreadPriority(hKeybdHookThread, THREAD_PRIORITY_TIME_CRITICAL);
			ResumeThread(hKeybdHookThread);

			WaitForSingleObject(hThreadReadyEvent, INFINITE);

			bSuccess = TRUE;
		}

		CloseHandle(hThreadReadyEvent);
	}

	return bSuccess;
}

void KeybdHook_Exit()
{
	HANDLE hThread;

	hThread = InterlockedExchangePointer(&hKeybdHookThread, NULL);
	if(hThread)
	{
		PostThreadMessage(dwKeybdHookThreadId, WM_APP, 0, 0);
		WaitForSingleObject(hThread, INFINITE);
		CloseHandle(hThread);

		bWinKeyDown = FALSE;
	}
}

static DWORD WINAPI KeybdHookThread(void *pParameter)
{
	HANDLE hThreadReadyEvent;
	MSG msg;
	BOOL bRet;
	HANDLE hThread;

	hThreadReadyEvent = (HANDLE)pParameter;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
	SetEvent(hThreadReadyEvent);

	hLowLevelKeybdHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeybdProc, GetModuleHandle(NULL), 0);
	if(hLowLevelKeybdHook)
	{
		while((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
		{
			if(bRet == -1)
			{
				msg.wParam = 0;
				break;
			}

			if(msg.hwnd == NULL && msg.message == WM_APP)
			{
				PostQuitMessage(0);
				continue;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		UnhookWindowsHookEx(hLowLevelKeybdHook);
	}
	else
		msg.wParam = 0;

	hThread = InterlockedExchangePointer(&hKeybdHookThread, NULL);
	if(hThread)
		CloseHandle(hThread);

	return (DWORD)msg.wParam;
}

static LRESULT CALLBACK LowLevelKeybdProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	KBDLLHOOKSTRUCT *kbdllHookStruct;

	if(nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_KEYUP))
	{
		kbdllHookStruct = (KBDLLHOOKSTRUCT *)lParam;

		if(kbdllHookStruct->vkCode == VK_LWIN || kbdllHookStruct->vkCode == VK_RWIN)
		{
			if((wParam == WM_KEYDOWN) != bWinKeyDown)
			{
				bWinKeyDown = (wParam == WM_KEYDOWN);
				InvalidateRect(hTaskListWnd, NULL, FALSE);
			}
		}
	}

	return CallNextHookEx(hLowLevelKeybdHook, nCode, wParam, lParam);
}
