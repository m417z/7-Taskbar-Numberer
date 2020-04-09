#include <windows.h>
#include "explorer_inject.h"
#include "version.h"
#include "resource.h"

#define UWM_NOTIFYICON        (WM_APP)

#define MSG_EXE_SHOWWINDOW    0
#define MSG_DLL_SETVIEWOPT    (-1)

typedef struct _dlg_param {
	UINT uTaskbarCreatedMsg, uPrivateMsg;
	HICON hSmallIcon, hLargeIcon;
	NOTIFYICONDATA nid;

	// Injection error
	BOOL bInjectFailed;

	// Settings
	BOOL bHideWnd;
	BOOL bHideTray;
	
	// Options
	int nViewOption;
} DLG_PARAM;

BOOL CompareWindowsVersion(DWORD dwMajorVersion, DWORD dwMinorVersion);
BOOL RegisterDialogClass(LPCTSTR lpszClassName, HINSTANCE hInstance);
LRESULT CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void InitNotifyIconData(HWND hWnd, DLG_PARAM *p_dlg_param);
void UpdateDisabledControls(HWND hWnd);
int FindCmdLineSwitch(const WCHAR *pSwitch);
LRESULT SendViewOptionMessage(HWND hWnd, UINT uTweakerMsg, int nViewOption);

int argc;
WCHAR **argv;

int main()
{
	// Check OS version
	if(
		!CompareWindowsVersion(6, 1) &&
		!CompareWindowsVersion(6, 2) &&
		!CompareWindowsVersion(6, 3) &&
		!CompareWindowsVersion(6, 4) &&
		!CompareWindowsVersion(10, 0)
	)
	{
		MessageBox(NULL, L"IDS_ERROR_WINXONLY", NULL, MB_ICONHAND);
		ExitProcess(0);
	}

	// Using the 32-bit version on a 64-bit OS?
#ifndef _WIN64
	BOOL bWow64Process;
	if(IsWow64Process(GetCurrentProcess(), &bWow64Process) && bWow64Process)
	{
		MessageBox(NULL, L"IDS_ERROR_64OS", NULL, MB_ICONHAND);
		ExitProcess(0);
	}
#endif

	// Run!
	HANDLE hDesktop = GetThreadDesktop(GetCurrentThreadId());
	if(!hDesktop)
	{
		MessageBox(NULL, L"GetThreadDesktop() failed", NULL, MB_ICONHAND);
		ExitProcess(0);
	}

	WCHAR szMutexName[MAX_PATH];
	lstrcpy(szMutexName, L"7TN_");
	if(!GetUserObjectInformation(hDesktop, UOI_NAME, szMutexName + 4, sizeof(szMutexName)-(4 * sizeof(WCHAR)), NULL))
	{
		MessageBox(NULL, L"GetUserObjectInformation() failed", NULL, MB_ICONHAND);
		ExitProcess(0);
	}

	HANDLE hMutex = CreateMutex(NULL, TRUE, szMutexName);
	if(!hMutex)
	{
		MessageBox(NULL, L"CreateMutex() failed", NULL, MB_ICONHAND);
		ExitProcess(0);
	}

	if(GetLastError() == ERROR_ALREADY_EXISTS)
	{
		HWND hNumbererWnd = FindWindow(L"7+ Taskbar Numberer", NULL);
		if(hNumbererWnd)
		{
			DWORD dwProcessId;

			GetWindowThreadProcessId(hNumbererWnd, &dwProcessId);
			AllowSetForegroundWindow(dwProcessId);

			PostMessage(hNumbererWnd, RegisterWindowMessage(L"7 Taskbar Numberer"), 0, MSG_EXE_SHOWWINDOW);
		}
	}
	else
	{
		DLG_PARAM DlgParam;

		argv = CommandLineToArgvW(GetCommandLine(), &argc);
		if(argv)
		{
			RegisterDialogClass(L"7+ Taskbar Numberer", GetModuleHandle(NULL));

			DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_MAIN), NULL, (DLGPROC)DlgProc, (LPARAM)&DlgParam);

			UnregisterClass(L"7+ Taskbar Numberer", GetModuleHandle(NULL));

			LocalFree(argv);
		}
		else
		{
			MessageBox(NULL, L"CommandLineToArgvW() failed", NULL, MB_ICONHAND);
		}

		ReleaseMutex(hMutex);
	}

	CloseHandle(hMutex);
	ExitProcess(0);
}

BOOL CompareWindowsVersion(DWORD dwMajorVersion, DWORD dwMinorVersion)
{
	OSVERSIONINFOEX ver;
	DWORDLONG dwlConditionMask = 0;

	ZeroMemory(&ver, sizeof(OSVERSIONINFOEX));
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	ver.dwMajorVersion = dwMajorVersion;
	ver.dwMinorVersion = dwMinorVersion;

	VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_EQUAL);
	VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_EQUAL);

	return VerifyVersionInfo(&ver, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask);
}

BOOL RegisterDialogClass(LPCTSTR lpszClassName, HINSTANCE hInstance)
{
	WNDCLASS wndcls;
	GetClassInfo(hInstance, MAKEINTRESOURCE(32770), &wndcls);

	// Set our own class name
	wndcls.lpszClassName = lpszClassName;

	// Just register the class
	return RegisterClass(&wndcls);
}

LRESULT CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static const WCHAR *pszOptions[] = { L"-v1", L"-v2", L"-v3", L"-v4", L"-v5", NULL };
	DLG_PARAM *pDlgParam;
	MSGBOXPARAMS mbpMsgBoxParams;
	HWND hPopup;
	WCHAR *pError;

	if(uMsg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hWnd, DWLP_USER, lParam);
		pDlgParam = (DLG_PARAM *)lParam;
	}
	else
	{
		pDlgParam = (DLG_PARAM *)GetWindowLongPtr(hWnd, DWLP_USER);
		if(!pDlgParam)
			return FALSE;
	}

	switch(uMsg)
	{
	case WM_INITDIALOG:
		pDlgParam->uTaskbarCreatedMsg = RegisterWindowMessage(L"TaskbarCreated");
		pDlgParam->uPrivateMsg = RegisterWindowMessage(L"7 Taskbar Numberer");

		pDlgParam->hSmallIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, 16, 16, 0);
		if(pDlgParam->hSmallIcon)
			SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)pDlgParam->hSmallIcon);

		pDlgParam->hLargeIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, 32, 32, 0);
		if(pDlgParam->hLargeIcon)
			SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)pDlgParam->hLargeIcon);

		pDlgParam->bInjectFailed = FALSE;

		pDlgParam->bHideWnd = FindCmdLineSwitch(L"-hidewnd") != 0;
		pDlgParam->bHideTray = FindCmdLineSwitch(L"-hidetray") != 0;

		InitNotifyIconData(hWnd, pDlgParam);
		if(pDlgParam->bHideTray && pDlgParam->bHideWnd)
			pDlgParam->nid.dwState |= NIS_HIDDEN;

		Shell_NotifyIcon(NIM_ADD, &pDlgParam->nid);

		pDlgParam->nViewOption = 0;
		for(int i = 0; pszOptions[i] != NULL; i++)
		{
			if(FindCmdLineSwitch(pszOptions[i]) != 0)
			{
				pDlgParam->nViewOption = i;
				break;
			}
		}

		switch(pDlgParam->nViewOption)
		{
		case 0:
			CheckDlgButton(hWnd, IDC_VIEWOPT1, BST_CHECKED);
			break;

		case 2:
			CheckDlgButton(hWnd, IDC_VIEWOPT3, BST_CHECKED);
		case 1:
			CheckDlgButton(hWnd, IDC_VIEWOPT2, BST_CHECKED);
			break;

		case 4:
			CheckDlgButton(hWnd, IDC_VIEWOPT5, BST_CHECKED);
		case 3:
			CheckDlgButton(hWnd, IDC_VIEWOPT4, BST_CHECKED);
			break;
		}

		UpdateDisabledControls(hWnd);

		pError = ExplorerInject(pDlgParam->nViewOption);
		if(pError)
		{
			pDlgParam->bInjectFailed = TRUE;
			MessageBox(hWnd, pError, NULL, MB_ICONHAND);
		}
		break;

	case WM_WINDOWPOSCHANGING:
		if(pDlgParam->bHideWnd && ((WINDOWPOS *)lParam)->flags & SWP_SHOWWINDOW)
		{
			((WINDOWPOS *)lParam)->flags &= ~SWP_SHOWWINDOW;
			pDlgParam->bHideWnd = FALSE;
		}
		break;

	case WM_LBUTTONDOWN:
		SendMessage(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
		break;

	case UWM_NOTIFYICON:
		switch(LOWORD(lParam))
		{
		case WM_LBUTTONUP:
			hPopup = GetLastActivePopup(hWnd);

			if(!IsWindowVisible(hWnd))
			{
				ShowWindow(hWnd, SW_SHOWNORMAL);
				SetForegroundWindow(hPopup);
			}
			else if(hPopup == hWnd)
			{
				if(pDlgParam->bHideTray)
				{
					pDlgParam->nid.dwState |= NIS_HIDDEN;
					Shell_NotifyIcon(NIM_MODIFY, &pDlgParam->nid);
				}

				ShowWindow(hWnd, SW_HIDE);
			}
			else
				SetForegroundWindow(hPopup);
			break;

		case WM_RBUTTONUP:
			hPopup = GetLastActivePopup(hWnd);

			if(hPopup == hWnd)
			{
				EndDialog(hWnd, 0);
			}
			else
			{
				ShowWindow(hWnd, SW_SHOWNORMAL);
				SetForegroundWindow(hPopup);
			}
			break;
		}
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_VIEWOPT1:
		case IDC_VIEWOPT2:
		case IDC_VIEWOPT3:
		case IDC_VIEWOPT4:
		case IDC_VIEWOPT5:
			UpdateDisabledControls(hWnd);

			switch(LOWORD(wParam))
			{
			case IDC_VIEWOPT1:
				pDlgParam->nViewOption = 0;
				break;

			case IDC_VIEWOPT2:
			case IDC_VIEWOPT3:
				pDlgParam->nViewOption = (IsDlgButtonChecked(hWnd, IDC_VIEWOPT3) ? 2 : 1);
				break;

			case IDC_VIEWOPT4:
			case IDC_VIEWOPT5:
				pDlgParam->nViewOption = (IsDlgButtonChecked(hWnd, IDC_VIEWOPT5) ? 4 : 3);
				break;
			}

			if(ExplorerIsInjected())
				SendViewOptionMessage(ExplorerGetTaskbarWnd(), pDlgParam->uPrivateMsg, pDlgParam->nViewOption);
			break;

		case IDOK:
			break;

		case IDC_ABOUT:
			ZeroMemory(&mbpMsgBoxParams, sizeof(MSGBOXPARAMS));

			mbpMsgBoxParams.cbSize = sizeof(MSGBOXPARAMS);
			mbpMsgBoxParams.hwndOwner = hWnd;
			mbpMsgBoxParams.hInstance = GetModuleHandle(NULL);
			mbpMsgBoxParams.lpszText = 
				L"7+ Taskbar Numberer v" VER_FILE_VERSION_WSTR L"\n"
				L"By RaMMicHaeL\n"
				L"http://rammichael.com/\n"
				L"\n"
				L"Compiled on: " TEXT(__DATE__);
			mbpMsgBoxParams.lpszCaption = L"About";
			mbpMsgBoxParams.dwStyle = MB_USERICON;
			mbpMsgBoxParams.lpszIcon = MAKEINTRESOURCE(IDI_MAIN);

			MessageBoxIndirect(&mbpMsgBoxParams);
			break;

		case IDCANCEL:
			if(pDlgParam->bHideTray)
			{
				pDlgParam->nid.dwState |= NIS_HIDDEN;
				Shell_NotifyIcon(NIM_MODIFY, &pDlgParam->nid);
			}

			ShowWindow(hWnd, SW_HIDE);
			break;
		}
		break;

	case WM_DESTROY:
		if(ExplorerIsInjected())
			ExplorerCleanup();

		Shell_NotifyIcon(NIM_DELETE, &pDlgParam->nid);

		if(pDlgParam->hSmallIcon)
			DestroyIcon(pDlgParam->hSmallIcon);

		if(pDlgParam->hLargeIcon)
			DestroyIcon(pDlgParam->hLargeIcon);
		break;

	default:
		if(uMsg == pDlgParam->uTaskbarCreatedMsg)
		{
			Shell_NotifyIcon(NIM_ADD, &pDlgParam->nid);

			if(!ExplorerIsInjected() && !pDlgParam->bInjectFailed)
			{
				pError = ExplorerInject(pDlgParam->nViewOption);
				if(pError)
				{
					pDlgParam->bInjectFailed = TRUE; // Prevents infinite crashes and restarts of explorer
					MessageBox(hWnd, pError, NULL, MB_ICONHAND);
				}
			}
		}
		else if(uMsg == pDlgParam->uPrivateMsg)
		{
			switch(lParam)
			{
			case MSG_EXE_SHOWWINDOW:
				if(pDlgParam->bHideTray)
				{
					pDlgParam->nid.dwState &= ~NIS_HIDDEN;
					Shell_NotifyIcon(NIM_MODIFY, &pDlgParam->nid);
				}

				hPopup = GetLastActivePopup(hWnd);

				ShowWindow(hWnd, SW_SHOWNORMAL);
				SetForegroundWindow(hPopup);
				break;
			}
		}
		break;
	}

	return FALSE;
}

void InitNotifyIconData(HWND hWnd, DLG_PARAM *p_dlg_param)
{
	NOTIFYICONDATA *p_nid;

	p_nid = &p_dlg_param->nid;

	ZeroMemory(p_nid, sizeof(NOTIFYICONDATA));

	p_nid->cbSize = sizeof(NOTIFYICONDATA);
	p_nid->hWnd = hWnd;
	p_nid->uID = 0;
	p_nid->uFlags = NIF_MESSAGE|NIF_ICON|NIF_TIP|NIF_STATE;
	p_nid->uCallbackMessage = UWM_NOTIFYICON;
	p_nid->hIcon = p_dlg_param->hSmallIcon;
	lstrcpy(p_nid->szTip, L"7+ Taskbar Numberer");
	p_nid->dwState = 0;
	p_nid->dwStateMask = NIS_HIDDEN;
	p_nid->uVersion = NOTIFYICON_VERSION_4;
}

void UpdateDisabledControls(HWND hWnd)
{
	if(IsDlgButtonChecked(hWnd, IDC_VIEWOPT1))
	{
		EnableWindow(GetDlgItem(hWnd, IDC_VIEWOPT3), FALSE);
		EnableWindow(GetDlgItem(hWnd, IDC_VIEWOPT5), FALSE);
	}
	else if(IsDlgButtonChecked(hWnd, IDC_VIEWOPT2))
	{
		EnableWindow(GetDlgItem(hWnd, IDC_VIEWOPT3), TRUE);
		EnableWindow(GetDlgItem(hWnd, IDC_VIEWOPT5), FALSE);
	}
	else // if(IsDlgButtonChecked(hWnd, IDC_VIEWOPT4))
	{
		EnableWindow(GetDlgItem(hWnd, IDC_VIEWOPT3), FALSE);
		EnableWindow(GetDlgItem(hWnd, IDC_VIEWOPT5), TRUE);
	}
}

int FindCmdLineSwitch(const WCHAR *pSwitch)
{
	int i;

	for(i=1; i<argc; i++)
		if(lstrcmpi(argv[i], pSwitch) == 0)
			return i;

	return 0;
}

LRESULT SendViewOptionMessage(HWND hWnd, UINT uTweakerMsg, int nViewOption)
{
	return SendMessage(hWnd, uTweakerMsg, nViewOption, MSG_DLL_SETVIEWOPT);
}
