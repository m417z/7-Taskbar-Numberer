#include "wnd_proc.h"
#include <shlobj.h>
#include "MinHook/MinHook.h"
#include "functions.h"
#include "explorer_vars.h"
#include "keybd_hook.h"
#include "pointer_redirection.h"
#include "com_func_hook.h"

// External
#define MSG_DLL_SETVIEWOPT     (-1)

// Internal
#define MSG_DLL_INIT           (-2)
#define MSG_DLL_UNSUBCLASS     (-3)
#define MSG_DLL_CALLFUNC       (-4)

static LRESULT THISCALL_C InitTaskbarProc(THISCALL_C_THIS_ARG(LONG_PTR *this_ptr), HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static void SubclassExplorerWindows(void);
static void UnsubclassExplorerWindows(void);
static BOOL InitFromExplorerThread(void);
static void UnintializeTweakerComponents(void);
static LONG_PTR ExitFromExplorerThread(void);
static LRESULT CALLBACK NewTaskbarProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
static LRESULT CALLBACK NewTaskSwProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
static LRESULT CALLBACK NewTrayToolbarProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
//static LRESULT CALLBACK NewTrayOverflowToolbarProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
static LRESULT CALLBACK NewTrayTemporaryToolbarProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
static LRESULT ProcessTrayToolbarMsg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// superglobals
extern UINT uPrivateMsg;
extern int nViewOption;
extern BOOL bWinKeyDown;
extern DWORD dwTaskbarThreadId;
extern HWND hTaskbarWnd, hTaskSwWnd, hTaskListWnd, hTrayNotifyWnd, hTrayToolbarWnd, hTrayTemporaryToolbarWnd;
extern LONG_PTR lpTaskbarLongPtr, lpTaskSwLongPtr, lpTaskListLongPtr, lpTrayNotifyLongPtr, lpTrayTemporaryToolbarLongPtr;

// subclasses
static WNDPROC pOldTaskbarProc;
static volatile int wnd_proc_call_counter;

// hooks
static void **ppTaskbarSubWndProc;
POINTER_REDIRECTION_VAR(static POINTER_REDIRECTION prTaskbarSubWndProc);

static BOOL bUnintializeStarted;

BOOL WndProcInit(void)
{
	HANDLE hExplorerIsShellMutex;
	DWORD dwError;

	// Set some globals
	uPrivateMsg = RegisterWindowMessage(L"7 Taskbar Numberer");

	// Wait until explorer shell is created
	hExplorerIsShellMutex = OpenMutex(SYNCHRONIZE, FALSE, L"Local\\ExplorerIsShellMutex");
	if(hExplorerIsShellMutex)
	{
		switch(WaitForSingleObject(hExplorerIsShellMutex, INFINITE))
		{
		case WAIT_OBJECT_0:
		case WAIT_ABANDONED:
			ReleaseMutex(hExplorerIsShellMutex);
			break;
		}

		CloseHandle(hExplorerIsShellMutex);
	}

	// Find our windows, and get their LongPtr's
	hTaskbarWnd = FindWindow(L"Shell_TrayWnd", NULL);
	if(!hTaskbarWnd)
		return FALSE;

	lpTaskbarLongPtr = GetWindowLongPtr(hTaskbarWnd, 0);
	hTaskSwWnd = (HWND)GetProp(hTaskbarWnd, L"TaskbandHWND");
	if(!hTaskSwWnd)
		return FALSE;

	lpTaskSwLongPtr = GetWindowLongPtr(hTaskSwWnd, 0);

	hTrayNotifyWnd = *EV_TASKBAR_TRAY_NOTIFY_WND;
	if(!hTrayNotifyWnd)
		return FALSE;

	lpTrayNotifyLongPtr = GetWindowLongPtr(hTrayNotifyWnd, 0);

	hTaskListWnd = FindWindowEx(hTaskSwWnd, NULL, L"MSTaskListWClass", NULL);
	if(!hTaskListWnd)
		return FALSE;

	lpTaskListLongPtr = GetWindowLongPtr(hTaskListWnd, 0);
/*
	hTrayOverflowToolbarWnd = *EV_TRAY_NOTIFY_OVERFLOW_TOOLBAR_WND(lpTrayNotifyLongPtr);
	if(!hTrayOverflowToolbarWnd)
		return FALSE;
*/

	hTrayTemporaryToolbarWnd = *EV_TRAY_NOTIFY_TEMPORARY_TOOLBAR_WND(lpTrayNotifyLongPtr);
	if(!hTrayTemporaryToolbarWnd)
		return FALSE;

	hTrayToolbarWnd = *EV_TRAY_NOTIFY_TOOLBAR_WND(lpTrayNotifyLongPtr);
	if(!hTrayToolbarWnd)
		return FALSE;

	// Init other stuff
	ppTaskbarSubWndProc = &((*(void ***)lpTaskbarLongPtr)[2]);
	PointerRedirectionAdd(ppTaskbarSubWndProc, InitTaskbarProc, &prTaskbarSubWndProc);

	dwError = (DWORD)SendMessage(hTaskbarWnd, uPrivateMsg, 0, MSG_DLL_INIT);
	if(dwError == 0)
	{
		PointerRedirectionRemove(ppTaskbarSubWndProc, &prTaskbarSubWndProc);
		return FALSE;
	}

	dwError -= 1;
	if(dwError)
	{
		SendMessage(hTaskbarWnd, uPrivateMsg, 0, MSG_DLL_UNSUBCLASS);
		while(wnd_proc_call_counter > 0)
			Sleep(10);

		return FALSE;
	}

	return TRUE;
}

static LRESULT THISCALL_C InitTaskbarProc(THISCALL_C_THIS_ARG(LONG_PTR *this_ptr), HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	DWORD dwError;

	if(uMsg == uPrivateMsg && lParam == MSG_DLL_INIT)
	{
		PointerRedirectionRemove(ppTaskbarSubWndProc, &prTaskbarSubWndProc);

		SubclassExplorerWindows();

		dwError = 0;

		if(!InitFromExplorerThread())
			dwError = 1;

		return 1 + dwError;
	}

	return ((LRESULT(THISCALL_C *)(THISCALL_C_THIS_TYPE(LONG_PTR *), HWND, UINT, WPARAM, LPARAM))prTaskbarSubWndProc.pOriginalAddress)(THISCALL_C_THIS_VAL(this_ptr), hWnd, uMsg, wParam, lParam);
}

static void SubclassExplorerWindows(void)
{
	SetWindowSubclass(hTaskbarWnd, NewTaskbarProc, 0, 0);
	SetWindowSubclass(hTaskSwWnd, NewTaskSwProc, 0, 0);
	SetWindowSubclass(hTrayToolbarWnd, NewTrayToolbarProc, 0, 0);
	//SetWindowSubclass(hTrayOverflowToolbarWnd, NewTrayOverflowToolbarProc, 0, 0);
	SetWindowSubclass(hTrayTemporaryToolbarWnd, NewTrayTemporaryToolbarProc, 0, 0);
}

static void UnsubclassExplorerWindows(void)
{
	RemoveWindowSubclass(hTaskbarWnd, NewTaskbarProc, 0);
	RemoveWindowSubclass(hTaskSwWnd, NewTaskSwProc, 0);
	RemoveWindowSubclass(hTrayToolbarWnd, NewTrayToolbarProc, 0);
	//RemoveWindowSubclass(hTrayOverflowToolbarWnd, NewTrayOverflowToolbarProc, 0);
	RemoveWindowSubclass(hTrayTemporaryToolbarWnd, NewTrayToolbarProc, 0);
}

static BOOL InitFromExplorerThread(void)
{
	dwTaskbarThreadId = GetCurrentThreadId();

	if(MH_Initialize() == MH_OK)
	{
		if(ComFuncHook_Init())
		{
			if(MH_ApplyQueued() == MH_OK)
			{
				if(nViewOption == 2 || nViewOption == 4)
					KeybdHook_Init();

				InvalidateRect(hTaskListWnd, NULL, FALSE);

				if(nViewOption == 0 || nViewOption == 3 || nViewOption == 4)
				{
					InvalidateSecondaryTaskListWndRect();
					if(nViewOption == 0)
					{
						InvalidateRect(hTrayToolbarWnd, NULL, FALSE);
						InvalidateRect(hTrayTemporaryToolbarWnd, NULL, FALSE);
					}
				}

				return TRUE;
			}
		}

		MH_Uninitialize();
	}

	return FALSE;
}

void WndProcExit(void)
{
	LRESULT lExited = SendMessage(hTaskbarWnd, uPrivateMsg, (WPARAM)ExitFromExplorerThread, MSG_DLL_CALLFUNC);
	if(lExited)
	{
		ComFuncHook_WaitTillDone();

		SendMessage(hTaskbarWnd, uPrivateMsg, 0, MSG_DLL_UNSUBCLASS);
	}
}

void WndProcWaitTillDone(void)
{
	while(wnd_proc_call_counter > 0)
		Sleep(10);
}

static LONG_PTR ExitFromExplorerThread(void)
{
	if(bUnintializeStarted)
		return 0;

	bUnintializeStarted = TRUE;

	int nOldOpt = nViewOption;
	nViewOption = 0;

	InvalidateRect(hTaskListWnd, NULL, FALSE);

	if(nOldOpt == 0 || nOldOpt == 3 || nOldOpt == 4)
	{
		InvalidateSecondaryTaskListWndRect();
		if(nOldOpt == 0)
		{
			InvalidateRect(hTrayToolbarWnd, NULL, FALSE);
			InvalidateRect(hTrayTemporaryToolbarWnd, NULL, FALSE);
		}
	}

	if(nOldOpt == 2 || nOldOpt == 4)
		KeybdHook_Exit();

	UnintializeTweakerComponents();

	return 1;
}

static void UnintializeTweakerComponents(void)
{
	ComFuncHook_Exit();
	MH_Uninitialize();
}

static LRESULT CALLBACK NewTaskbarProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	int nOldOpt, nNewOpt;
	LRESULT result;

	wnd_proc_call_counter++;

	switch(uMsg)
	{
	case WM_DESTROY:
		if(!bUnintializeStarted)
		{
			bUnintializeStarted = TRUE;

			result = DefSubclassProc(hWnd, uMsg, wParam, lParam);

			if(nViewOption == 2 || nViewOption == 4)
				KeybdHook_Exit();

			UnintializeTweakerComponents();
			UnsubclassExplorerWindows();
		}
		else
			result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
		break;

	case WM_WTSSESSION_CHANGE:
		if((nViewOption == 2 || nViewOption == 4) && lParam == WTSGetActiveConsoleSessionId())
		{
			switch(wParam)
			{
			case WTS_CONSOLE_CONNECT:
			case WTS_SESSION_UNLOCK:
				if(bWinKeyDown)
				{
					bWinKeyDown = FALSE;
					InvalidateRect(hTaskListWnd, NULL, FALSE);
				}
				break;
			}
		}

		result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
		break;

	default:
		if(uMsg == uPrivateMsg)
		{
			result = 0;

			switch(lParam)
			{
			case MSG_DLL_SETVIEWOPT:
				nOldOpt = nViewOption;
				nNewOpt = (int)wParam;

				if(nNewOpt != nOldOpt)
				{
					nViewOption = nNewOpt;

					if((nOldOpt == 2 || nOldOpt == 4) != (nNewOpt == 2 || nNewOpt == 4))
					{
						if(nOldOpt == 2 || nOldOpt == 4)
							KeybdHook_Exit();
						else // if(nNewOpt == 2 || nNewOpt == 4)
							KeybdHook_Init();
					}

					InvalidateRect(hTaskListWnd, NULL, FALSE);
					InvalidateSecondaryTaskListWndRect();

					if((nOldOpt == 0) != (nNewOpt == 0))
					{
						InvalidateRect(hTrayToolbarWnd, NULL, FALSE);
						InvalidateRect(hTrayTemporaryToolbarWnd, NULL, FALSE);
					}
				}
				break;

			case MSG_DLL_UNSUBCLASS:
				UnsubclassExplorerWindows();
				break;

			case MSG_DLL_CALLFUNC:
				result = ((LONG_PTR(*)())wParam)();
				break;
			}
		}
		else
			result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
		break;
	}

	wnd_proc_call_counter--;

	return result;
}

static LRESULT CALLBACK NewTaskSwProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	HANDLE hChange;
	PIDLIST_ABSOLUTE *ppidl;
	long lEvent;
	LRESULT result;

	wnd_proc_call_counter++;

	switch(uMsg)
	{
	case 0x044A: // Button create
		result = DefSubclassProc(hWnd, uMsg, wParam, lParam);

		ComFuncButtonCreatedOrPinnedItemChange();
		break;

	case 0x043A: // Registered with SHChangeNotifyRegister
		hChange = SHChangeNotification_Lock((HANDLE)wParam, (DWORD)lParam, &ppidl, &lEvent);
		if(hChange)
		{
			if(lEvent == SHCNE_EXTENDED_EVENT)
			{
				if(ppidl && ppidl[0] && *(DWORD *)&ppidl[0]->mkid.abID == 0x0D) // Pinned item change (add or remove)
					ComFuncButtonCreatedOrPinnedItemChange();
			}

			SHChangeNotification_Unlock(hChange);
		}

		result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
		break;

	default:
		result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
		break;
	}

	wnd_proc_call_counter--;

	return result;
}

static LRESULT CALLBACK NewTrayToolbarProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	LRESULT result;

	wnd_proc_call_counter++;

	if(nViewOption == 0)
		result = ProcessTrayToolbarMsg(hWnd, uMsg, wParam, lParam);
	else
		result = DefSubclassProc(hWnd, uMsg, wParam, lParam);

	wnd_proc_call_counter--;

	return result;
}
/*
static LRESULT CALLBACK NewTrayOverflowToolbarProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	LRESULT result;

	wnd_proc_call_counter++;

	if(nViewOption == 0)
		result = ProcessTrayToolbarMsg(hWnd, uMsg, wParam, lParam);
	else
		result = DefSubclassProc(hWnd, uMsg, wParam, lParam);

	wnd_proc_call_counter--;

	return result;
}
*/
static LRESULT CALLBACK NewTrayTemporaryToolbarProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	LRESULT result;

	wnd_proc_call_counter++;

	if(nViewOption == 0)
		result = ProcessTrayToolbarMsg(hWnd, uMsg, wParam, lParam);
	else
		result = DefSubclassProc(hWnd, uMsg, wParam, lParam);

	wnd_proc_call_counter--;

	return result;
}

static LRESULT ProcessTrayToolbarMsg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result;

	switch(uMsg)
	{
	case WM_PAINT:
		ComFuncBeforeTrayToolbarPaint(hWnd);

		result = DefSubclassProc(hWnd, uMsg, wParam, lParam);

		ComFuncAfterTrayToolbarPaint(hWnd);
		break;

	default:
		result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
		break;
	}

	return result;
}

void InvalidateSecondaryTaskListWndRect()
{
	SECONDARY_TASK_LIST_GET secondary_task_list_get;
	LONG_PTR lpSecondaryTaskListLongPtr;
	HWND hSecondaryTaskListWnd;

	lpSecondaryTaskListLongPtr = SecondaryTaskListGetFirstLongPtr(&secondary_task_list_get);
	while(lpSecondaryTaskListLongPtr)
	{
		hSecondaryTaskListWnd = *(HWND *)(lpSecondaryTaskListLongPtr + DEF3264(0x04, 0x08));
		InvalidateRect(hSecondaryTaskListWnd, NULL, FALSE);

		lpSecondaryTaskListLongPtr = SecondaryTaskListGetNextLongPtr(&secondary_task_list_get);
	}
}
