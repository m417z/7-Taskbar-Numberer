#include "com_func_hook.h"
#include <uxtheme.h>
#include <commoncontrols.h>
#include "MinHook/MinHook.h"
#include "functions.h"
#include "explorer_vars.h"
#include "keybd_hook.h"
#include "pointer_redirection.h"

static BOOL HookTaskBtnGroupFunctions();
static void UnhookTaskBtnGroupFunctions();
static LONG_PTR __stdcall RenderHook(LONG_PTR *button_group, LONG_PTR var2, LONG_PTR var3, LONG_PTR var4, LONG_PTR var5);
static LONG_PTR __stdcall RenderHook2(LONG_PTR *button_group, LONG_PTR var2, LONG_PTR var3, LONG_PTR var4, LONG_PTR var5, LONG_PTR var6);
static BOOL RenderHookPreOperation(LONG_PTR *button_group);
static void RenderHookPostOperation();
static LONG_PTR __stdcall TaskbarImageListDrawHook(LONG_PTR var1, IMAGELISTDRAWPARAMS *pimldp);
static int ButtonGroupFirstButtonIndex(LONG_PTR lpMMTaskListLongPtr, LONG_PTR *button_group);
static int ButtonGroupIndex(LONG_PTR lpMMTaskListLongPtr, LONG_PTR *button_group);
static void TaskbarImageListDrawText(HTHEME hTheme, IMAGELISTDRAWPARAMS *pimldp, WCHAR *pText, BOOL bSmallIcons);
static LONG_PTR __stdcall TrayToolbarImageListDrawHook(IImageList *this_ptr, IMAGELISTDRAWPARAMS *pimldp);
static int TrayToolbarHitTest(HWND hWnd, POINT *ppt);
static void TrayToolbarImageListDrawText(HTHEME hTheme, IMAGELISTDRAWPARAMS *pimldp, WCHAR *pText);

// superglobals
extern int nViewOption;
extern BOOL bWinKeyDown;
extern DWORD dwTaskbarThreadId;
extern HWND hTaskbarWnd, hTaskSwWnd, hTaskListWnd, hTrayNotifyWnd, hTrayToolbarWnd, hTrayTemporaryToolbarWnd;
extern LONG_PTR lpTaskbarLongPtr, lpTaskSwLongPtr, lpTaskListLongPtr, lpTrayNotifyLongPtr, lpTrayTemporaryToolbarLongPtr;

// hooks
static void **ppRender;
static void *pRender;
POINTER_REDIRECTION_VAR(static POINTER_REDIRECTION prRender);
static void **ppTaskbarImageListDraw;
static void *pTaskbarImageListDraw;
POINTER_REDIRECTION_VAR(static POINTER_REDIRECTION prTaskbarImageListDraw);
static void **ppTrayToolbarImageListDraw;
static void *pTrayToolbarImageListDraw;
POINTER_REDIRECTION_VAR(static POINTER_REDIRECTION prTrayToolbarImageListDraw);
static BOOL bTaskBtnGroupFunctionsHooked;
volatile static int nTrayToolbarPaintDepth;
volatile static int hook_proc_call_counter;

static HTHEME hTaskListTheme;
static int nButtonNumber;
static BOOL bButtonIsSmall;
static HWND hPaintingToolbar;

// Hooks
static BOOL CreateEnableHook(void** ppTarget, void* const pDetour, void** ppOriginal, POINTER_REDIRECTION *ppr);
static BOOL DisableHook(void** ppTarget, POINTER_REDIRECTION *ppr);

BOOL ComFuncHook_Init()
{
	// TaskListWnd functions hook
	if(HookTaskBtnGroupFunctions())
		bTaskBtnGroupFunctionsHooked = TRUE;

	return TRUE;
}

void ComFuncHook_Exit()
{
	if(bTaskBtnGroupFunctionsHooked)
		UnhookTaskBtnGroupFunctions();
}

void ComFuncHook_WaitTillDone()
{
	while(hook_proc_call_counter > 0 || nTrayToolbarPaintDepth > 0)
		Sleep(10);
}

static BOOL HookTaskBtnGroupFunctions()
{
	LONG_PTR *plp;
	SECONDARY_TASK_LIST_GET secondary_task_list_get;
	LONG_PTR lpSecondaryTaskListLongPtr;

	plp = (LONG_PTR *)*EV_MM_TASKLIST_BUTTON_GROUPS_HDPA(lpTaskListLongPtr);
	if(!plp || (int)plp[0] == 0)
	{
		lpSecondaryTaskListLongPtr = SecondaryTaskListGetFirstLongPtr(&secondary_task_list_get);
		while(lpSecondaryTaskListLongPtr)
		{
			plp = (LONG_PTR *)*EV_MM_TASKLIST_BUTTON_GROUPS_HDPA(lpSecondaryTaskListLongPtr);
			if(!plp || (int)plp[0] == 0)
				lpSecondaryTaskListLongPtr = SecondaryTaskListGetNextLongPtr(&secondary_task_list_get);
			else
				break;
		}

		if(!lpSecondaryTaskListLongPtr)
			return FALSE;
	}

	plp = (LONG_PTR *)plp[1]; // CTaskBtnGroup DPA array
	plp = (LONG_PTR *)plp[0]; // First array item
	plp = (LONG_PTR *)plp[0]; // COM functions list

	void *pRenderHook;
	if(nWinVersion >= WIN_VERSION_10_R4)
		pRenderHook = RenderHook2;
	else
		pRenderHook = RenderHook;

	ppRender = (void **)&plp[DO5(18, 20, , , 21)];
	if(!CreateEnableHook(ppRender, pRenderHook, &pRender, &prRender))
	{
		UnhookTaskBtnGroupFunctions();
		return FALSE;
	}

	bTaskBtnGroupFunctionsHooked = TRUE;
	return TRUE;
}

static void UnhookTaskBtnGroupFunctions()
{
	DisableHook(ppRender, &prRender);
}

static LONG_PTR __stdcall RenderHook(LONG_PTR *button_group, LONG_PTR var2, LONG_PTR var3, LONG_PTR var4, LONG_PTR var5)
{
	BOOL bProcessed;
	LONG_PTR lpRet;

	InterlockedIncrement((long *)&hook_proc_call_counter);

	bProcessed = RenderHookPreOperation(button_group);

	lpRet = ((LONG_PTR(__stdcall *)(LONG_PTR *, LONG_PTR, LONG_PTR, LONG_PTR, LONG_PTR))pRender)
		(button_group, var2, var3, var4, var5);

	if(bProcessed)
		RenderHookPostOperation();

	InterlockedDecrement((long *)&hook_proc_call_counter);

	return lpRet;
}

static LONG_PTR __stdcall RenderHook2(LONG_PTR *button_group, LONG_PTR var2, LONG_PTR var3, LONG_PTR var4, LONG_PTR var5, LONG_PTR var6)
{
	BOOL bProcessed;
	LONG_PTR lpRet;

	InterlockedIncrement((long *)&hook_proc_call_counter);

	bProcessed = RenderHookPreOperation(button_group);

	lpRet = ((LONG_PTR(__stdcall *)(LONG_PTR *, LONG_PTR, LONG_PTR, LONG_PTR, LONG_PTR, LONG_PTR))pRender)
		(button_group, var2, var3, var4, var5, var6);

	if(bProcessed)
		RenderHookPostOperation();

	InterlockedDecrement((long *)&hook_proc_call_counter);

	return lpRet;
}

static BOOL RenderHookPreOperation(LONG_PTR *button_group)
{
	LONG_PTR lpMMTaskListLongPtr;
	LONG_PTR lp;
	int nNumber;
	HIMAGELIST hImageList;
	IImageList *imageList;
	HRESULT hr;

	// multimonitor environment
	if(nWinVersion >= WIN_VERSION_8)
		lpMMTaskListLongPtr = button_group[3];
	else
		lpMMTaskListLongPtr = lpTaskListLongPtr;

	nNumber = -1;

	if(nViewOption == 0)
	{
		int nIndex = ButtonGroupFirstButtonIndex(lpMMTaskListLongPtr, button_group);
		if(nIndex >= 0)
			nNumber = 1 + nIndex;
	}
	else if(nViewOption == 1 || ((nViewOption == 2 || nViewOption == 4) && bWinKeyDown))
	{
		if(lpMMTaskListLongPtr == lpTaskListLongPtr) // main monitor
		{
			int nIndex = ButtonGroupIndex(lpMMTaskListLongPtr, button_group);
			if(nIndex >= 0 && nIndex < 10)
				nNumber = 1 + nIndex;
		}
	}
	else if(nViewOption == 3 || nViewOption == 4)
	{
		int button_group_type = (int)button_group[DO2(6, 8)];
		if(button_group_type == 3)
		{
			LONG_PTR *plp = (LONG_PTR *)button_group[DO2(5, 7)];

			int buttons_count = (int)plp[0];
			//LONG_PTR **buttons = (LONG_PTR **)plp[1];

			if(buttons_count > 1)
				nNumber = buttons_count;
		}
	}

	if(nNumber >= 0)
	{
		nButtonNumber = nNumber;
		bButtonIsSmall = ((TaskbarGetPreference(lpMMTaskListLongPtr) & 0x10) != 0);

		// DEF3264: CTaskListWnd::_HandlePaint -or- CTaskListWnd::_HandleThemeChanged
		hTaskListTheme = *(HTHEME *)(lpTaskListLongPtr + DO8_3264(0x3C, 0x78, 0x40, 0x80, ,, ,, ,, ,, 0x34, 0x68, 0x38, 0x70));
		if(hTaskListTheme)
		{
			lp = *EV_MM_TASKLIST_TASK_BAND_REF(lpTaskListLongPtr);

			if(nWinVersion <= WIN_VERSION_811)
			{
				// DEF3264: CTaskBand::GetImageList
				hImageList = *(HIMAGELIST *)(lp + DO4_3264(0x40, 0x60, 0x34, 0x50, 0x38, 0x58, 0x44, 0x68));
			}
			else // if(nWinVersion >= WIN_VERSION_10)
			{
				// DEF3264: CTaskBand::GetImageList
				lp = *(LONG_PTR *)(lp + DO11_3264(0, 0, ,, ,, ,,
					0x50 - 0x04, 0x80 - 0x08,
					0x54 - 0x04, 0x88 - 0x08,
					0x5C - 0x04, 0xA0 - 0x08,
					0x60 - 0x04, 0xA0 - 0x08,
					,,
					,,
					0x58 - 0x04, 0x98 - 0x08));

				// DEF3264: IconContainer::GetReadOnlyImageListForDPI
				hImageList = **(HIMAGELIST **)(lp + DEF3264(0x20, 0x38));
			}

			// CImageList::Draw
			hr = HIMAGELIST_QueryInterface(hImageList, &IID_IImageList, (void **)&imageList);
			if(SUCCEEDED(hr))
			{
				ppTaskbarImageListDraw = (void **)&imageList->lpVtbl->Draw;

				PointerRedirectionAdd(ppTaskbarImageListDraw, TaskbarImageListDrawHook, &prTaskbarImageListDraw);

				imageList->lpVtbl->Release(imageList);

				return TRUE;
			}
		}
	}

	return FALSE;
}

static void RenderHookPostOperation()
{
	PointerRedirectionRemove(ppTaskbarImageListDraw, &prTaskbarImageListDraw);
}

static LONG_PTR __stdcall TaskbarImageListDrawHook(LONG_PTR var1, IMAGELISTDRAWPARAMS *pimldp)
{
	WCHAR szText[sizeof("4294967295")];
	LONG_PTR lpRet;

	InterlockedIncrement((long *)&hook_proc_call_counter);

	lpRet = ((LONG_PTR(__stdcall *)(LONG_PTR, IMAGELISTDRAWPARAMS *))prTaskbarImageListDraw.pOriginalAddress)(var1, pimldp);

	if(GetCurrentThreadId() == dwTaskbarThreadId)
	{
		if(pimldp->himl)
		{
			if(nViewOption == 0)
			{
				wsprintf(szText, L"%d", nButtonNumber);
				nButtonNumber++;
			}
			else if(nViewOption == 1 || ((nViewOption == 2 || nViewOption == 4) && bWinKeyDown))
			{
				wsprintf(szText, L"%d", nButtonNumber % 10);
			}
			else if(nViewOption == 3 || nViewOption == 4)
			{
				wsprintf(szText, L"%d", nButtonNumber);
			}

			TaskbarImageListDrawText(hTaskListTheme, pimldp, szText, bButtonIsSmall);
		}
	}

	InterlockedDecrement((long *)&hook_proc_call_counter);

	return lpRet;
}

static int ButtonGroupFirstButtonIndex(LONG_PTR lpMMTaskListLongPtr, LONG_PTR *button_group)
{
	LONG_PTR *plp;
	int button_groups_count;
	LONG_PTR **button_groups;
	int button_group_type;
	int buttons_count;
	LONG_PTR **buttons;
	int index;
	int i;

	if(button_group)
	{
		plp = (LONG_PTR *)*EV_MM_TASKLIST_BUTTON_GROUPS_HDPA(lpMMTaskListLongPtr);
		if(plp)
		{
			index = 0;

			button_groups_count = (int)plp[0];
			button_groups = (LONG_PTR **)plp[1];

			for(i = 0; i < button_groups_count; i++)
			{
				if(button_group == button_groups[i])
					return index;

				button_group_type = (int)button_groups[i][DO2(6, 8)];
				if(button_group_type == 1)
				{
					plp = (LONG_PTR *)button_groups[i][DO2(5, 7)];

					buttons_count = (int)plp[0];
					buttons = (LONG_PTR **)plp[1];

					index += buttons_count;
				}
				else
					index++;
			}
		}
	}

	return -1;
}

static int ButtonGroupIndex(LONG_PTR lpMMTaskListLongPtr, LONG_PTR *button_group)
{
	LONG_PTR *plp;
	int button_groups_count;
	LONG_PTR **button_groups;
	int index;
	int i;

	if(button_group)
	{
		plp = (LONG_PTR *)*EV_MM_TASKLIST_BUTTON_GROUPS_HDPA(lpMMTaskListLongPtr);
		if(plp)
		{
			index = 0;

			button_groups_count = (int)plp[0];
			button_groups = (LONG_PTR **)plp[1];

			for(i = 0; i < button_groups_count; i++)
			{
				if(button_group == button_groups[i])
					return index;

				index++;
			}
		}
	}

	return -1;
}

static void TaskbarImageListDrawText(HTHEME hTheme, IMAGELISTDRAWPARAMS *pimldp, WCHAR *pText, BOOL bSmallIcons)
{
	int cx, cy;
	RECT rc;
	DTTOPTS dtt_options;

	// Rect init
	ImageList_GetIconSize(pimldp->himl, &cx, &cy);

	rc.left = pimldp->x;
	rc.top = pimldp->y;
	rc.right = pimldp->x + cx;
	rc.bottom = pimldp->y + cy;

	// Rect calc
	ZeroMemory(&dtt_options, sizeof(DTTOPTS));
	dtt_options.dwSize = sizeof(DTTOPTS);
	dtt_options.dwFlags = DTT_CALCRECT | DTT_COMPOSITED;

	DrawThemeTextEx(hTheme, pimldp->hdcDst, 0, 1, pText, -1, DT_SINGLELINE | DT_CALCRECT | DT_NOPREFIX, &rc, &dtt_options);

	// Frame draw
	if(bSmallIcons)
	{
		rc.top -= 4;
		rc.bottom -= 6;
		rc.left -= 4;
		rc.right -= 4;
	}

	rc.right += 4;
	FrameRect(pimldp->hdcDst, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));
	rc.right -= 4;

	rc.left++;
	rc.right++;

	// Rect draw
	rc.right += 2;
	rc.top++;
	rc.bottom--;
	//FillRect(pimldp->hdcDst, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
	DrawThemeParentBackground(hTaskListWnd, pimldp->hdcDst, &rc);
	rc.right -= 2;
	rc.top--;
	rc.bottom++;

	rc.left++;
	rc.right++;

	// Text draw
	if(bSmallIcons)
		rc.top--;

	dtt_options.dwFlags &= ~DTT_CALCRECT;

	DrawThemeTextEx(hTheme, pimldp->hdcDst, 0, 1, pText, -1, DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS, &rc, &dtt_options);
}

void ComFuncButtonCreatedOrPinnedItemChange()
{
	BOOL bApplyQueuedHooks = FALSE;

	if(!bTaskBtnGroupFunctionsHooked)
		if(HookTaskBtnGroupFunctions() && bTaskBtnGroupFunctionsHooked)
			bApplyQueuedHooks = TRUE;

	if(bApplyQueuedHooks)
		MH_ApplyQueued();
}

void ComFuncBeforeTrayToolbarPaint(HWND hWnd)
{
	HIMAGELIST hImageList;
	IImageList *imageList;
	HRESULT hr;

	if(nTrayToolbarPaintDepth == 0)
	{
		hImageList = (HIMAGELIST)SendMessage(hWnd, TB_GETIMAGELIST, 0, 0);
		if(hImageList)
		{
			hr = HIMAGELIST_QueryInterface(hImageList, &IID_IImageList, (void **)&imageList);
			if(SUCCEEDED(hr))
			{
				ppTrayToolbarImageListDraw = (void **)&imageList->lpVtbl->Draw;
				if(ppTrayToolbarImageListDraw)
					PointerRedirectionAdd(ppTrayToolbarImageListDraw, TrayToolbarImageListDrawHook, &prTrayToolbarImageListDraw);

				imageList->lpVtbl->Release(imageList);
			}
		}
	}

	hPaintingToolbar = hWnd;
	nTrayToolbarPaintDepth++;
}

void ComFuncAfterTrayToolbarPaint(HWND hWnd)
{
	nTrayToolbarPaintDepth--;

	if(nTrayToolbarPaintDepth == 0)
	{
		if(ppTrayToolbarImageListDraw)
		{
			PointerRedirectionRemove(ppTrayToolbarImageListDraw, &prTrayToolbarImageListDraw);
			ppTrayToolbarImageListDraw = NULL;
		}
	}
}

static LONG_PTR __stdcall TrayToolbarImageListDrawHook(IImageList *this_ptr, IMAGELISTDRAWPARAMS *pimldp)
{
	LONG_PTR lpRet;

	InterlockedIncrement((long *)&hook_proc_call_counter);

	lpRet = ((LONG_PTR(__stdcall *)(IImageList *, IMAGELISTDRAWPARAMS *))prTrayToolbarImageListDraw.pOriginalAddress)(this_ptr, pimldp);

	if(GetCurrentThreadId() == dwTaskbarThreadId)
	{
		POINT pt;
		pt.x = pimldp->x;
		pt.y = pimldp->y;

		int index = TrayToolbarHitTest(hPaintingToolbar, &pt);
		if(hPaintingToolbar == hTrayToolbarWnd)
			index += (int)SendMessage(hTrayTemporaryToolbarWnd, TB_BUTTONCOUNT, 0, 0);

		if(index >= 0)
		{
			// DEF3264: CTrayNotify::_OpenTheme
			HTHEME *phTheme = (HTHEME *)(lpTrayNotifyLongPtr + DO12_3264(0x34C, 0x420, 0x14C, 0x220, ,, ,, 0xF0, 0x1A0, ,, 0xF4, 0x1B0, 0x108, 0x1D0, 0x10C, 0x1D8, 0x110, 0x1D8, ,, 0x10C, 0x1D0));

			// DEF3264: CTrayNotify::_UpdateChevronState
			BOOL *pbChevronVisible = (BOOL *)(lpTrayNotifyLongPtr + DO8_3264(0x324, 0x3EC, 0x124, 0x1EC, ,, ,, 0xB8, 0x148, ,, 0xB8, 0x150, 0xC0, 0x158));

			if(nWinVersion == WIN_VERSION_811 && nExplorerQFE >= 17238)
			{
				// For Windows 8.1 Update 1:
				//
				// <= build 17039
				// original
				//
				// builds between the two
				// unknown
				//
				// >= build 17238
				// with fixup

				// DEF3264: CTrayNotify::_OpenTheme
				phTheme = (HTHEME *)(lpTrayNotifyLongPtr + DEF3264(0x150, 0x228));
			}

			if(*phTheme)
			{
				WCHAR szText[sizeof("4294967295")];

				// if there's an arrow for more items in the notification area
				if(*pbChevronVisible)
					wsprintf(szText, L"%d", index + 2);
				else
					wsprintf(szText, L"%d", index + 1);

				TrayToolbarImageListDrawText(*phTheme, pimldp, szText);
			}
		}
	}

	InterlockedDecrement((long *)&hook_proc_call_counter);

	return lpRet;
}

static int TrayToolbarHitTest(HWND hWnd, POINT *ppt)
{
	int index;
	int i;
	TBBUTTON tb;

	index = (int)SendMessage(hTrayToolbarWnd, TB_HITTEST, 0, (LPARAM)ppt);
	if(index < 0)
		return -1;

	i = index;

	SendMessage(hTrayToolbarWnd, TB_GETBUTTON, i, (LPARAM)&tb);
	if(tb.fsState & TBSTATE_HIDDEN)
		return -1;

	while(i--)
	{
		SendMessage(hTrayToolbarWnd, TB_GETBUTTON, i, (LPARAM)&tb);
		if(tb.fsState & TBSTATE_HIDDEN)
			index--;
	}

	return index;
}

static void TrayToolbarImageListDrawText(HTHEME hTheme, IMAGELISTDRAWPARAMS *pimldp, WCHAR *pText)
{
	int cx, cy;
	RECT rc;
	DTTOPTS dtt_options;
	int n;

	// Rect init
	ImageList_GetIconSize(pimldp->himl, &cx, &cy);

	rc.left = pimldp->x;
	rc.top = pimldp->y;
	rc.right = pimldp->x + cx;
	rc.bottom = pimldp->y + cy;

	// Rect calc
	ZeroMemory(&dtt_options, sizeof(DTTOPTS));
	dtt_options.dwSize = sizeof(DTTOPTS);
	dtt_options.dwFlags = DTT_CALCRECT | DTT_COMPOSITED | DTT_TEXTCOLOR;
	dtt_options.crText = RGB(255, 255, 255);

	DrawThemeTextEx(hTheme, pimldp->hdcDst, 0, 1, pText, -1, DT_SINGLELINE | DT_CALCRECT | DT_NOPREFIX, &rc, &dtt_options);

	n = cx - (rc.right - rc.left);
	rc.left += n;
	rc.right += n;

	n = cy - (rc.bottom - rc.top);
	rc.top += n;
	rc.bottom += n;

	// Frame draw
	rc.top -= 4;
	rc.bottom -= 6;

	rc.right += 4;
	FrameRect(pimldp->hdcDst, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));
	rc.right -= 4;

	rc.left++;
	rc.right++;

	// Rect draw
	rc.right += 2;
	rc.top++;
	rc.bottom--;
	//FillRect(pimldp->hdcDst, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
	DrawThemeParentBackground(hTaskListWnd, pimldp->hdcDst, &rc);
	rc.right -= 2;
	rc.top--;
	rc.bottom++;

	rc.left++;
	rc.right++;

	// Text draw
	rc.top--;

	dtt_options.dwFlags &= ~DTT_CALCRECT;

	DrawThemeTextEx(hTheme, pimldp->hdcDst, 0, 1, pText, -1, DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS, &rc, &dtt_options);
}

// Hooks

static BOOL CreateEnableHook(void** ppTarget, void* const pDetour, void** ppOriginal, POINTER_REDIRECTION *ppr)
{
	void* pTarget;
	MH_STATUS status;

	pTarget = PointerRedirectionGetOriginalPtr(ppTarget);

	status = MH_CreateHook(pTarget, pDetour, ppOriginal);
	if(status == MH_OK)
	{
		status = MH_QueueEnableHook(pTarget);
		if(status == MH_OK)
		{
			PointerRedirectionAdd(ppTarget, pDetour, ppr);
			return TRUE;
		}

		MH_RemoveHook(pTarget);
	}

	*ppOriginal = NULL;
	return FALSE;
}

static BOOL DisableHook(void** ppTarget, POINTER_REDIRECTION *ppr)
{
	/*void* pTarget;
	MH_STATUS status;*/

	if(ppr->pOriginalAddress)
	{
		PointerRedirectionRemove(ppTarget, ppr);

		// Note: no need to cleanup MinHook hooks, they will be removed upon uninitialization

		/*pTarget = PointerRedirectionGetOriginalPtr(ppTarget);

		status = MH_QueueDisableHook(pTarget);
		if(status != MH_OK)
			return FALSE;*/
	}

	return TRUE;
}
