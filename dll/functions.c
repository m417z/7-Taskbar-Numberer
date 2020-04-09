#include "functions.h"
#include "explorer_vars.h"

// superglobals
extern HWND hTaskSwWnd;
extern HWND hTaskbarWnd, hTaskSwWnd, hTaskListWnd, hThumbnailWnd;
extern LONG_PTR lpTaskbarLongPtr, lpTaskSwLongPtr, lpTaskListLongPtr, lpThumbnailLongPtr;

// General functions

VS_FIXEDFILEINFO *GetModuleVersionInfo(HMODULE hModule, UINT *puPtrLen)
{
	HRSRC hResource;
	HGLOBAL hGlobal;
	void *pData;
	void *pFixedFileInfo;
	UINT uPtrLen;

	pFixedFileInfo = NULL;
	uPtrLen = 0;

	hResource = FindResource(hModule, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);
	if(hResource != NULL)
	{
		hGlobal = LoadResource(hModule, hResource);
		if(hGlobal != NULL)
		{
			pData = LockResource(hGlobal);
			if(pData != NULL)
			{
				if(!VerQueryValue(pData, L"\\", &pFixedFileInfo, &uPtrLen) || uPtrLen == 0)
				{
					pFixedFileInfo = NULL;
					uPtrLen = 0;
				}
			}
		}
	}

	if(puPtrLen)
		*puPtrLen = uPtrLen;

	return (VS_FIXEDFILEINFO *)pFixedFileInfo;
}

void **FindImportPtr(HMODULE hFindInModule, char *pModuleName, char *pImportName)
{
	IMAGE_DOS_HEADER *pDosHeader;
	IMAGE_NT_HEADERS *pNtHeader;
	ULONG_PTR ImageBase;
	IMAGE_IMPORT_DESCRIPTOR *pImportDescriptor;
	ULONG_PTR *pOriginalFirstThunk;
	ULONG_PTR *pFirstThunk;
	ULONG_PTR ImageImportByName;

	// Init
	pDosHeader = (IMAGE_DOS_HEADER *)hFindInModule;
	pNtHeader = (IMAGE_NT_HEADERS *)((char *)pDosHeader + pDosHeader->e_lfanew);

	if(!pNtHeader->OptionalHeader.DataDirectory[1].VirtualAddress)
		return NULL;

	ImageBase = (ULONG_PTR)hFindInModule;
	pImportDescriptor = (IMAGE_IMPORT_DESCRIPTOR *)(ImageBase + pNtHeader->OptionalHeader.DataDirectory[1].VirtualAddress);

	// Search!
	while(pImportDescriptor->OriginalFirstThunk)
	{
		if(lstrcmpiA((char *)(ImageBase + pImportDescriptor->Name), pModuleName) == 0)
		{
			pOriginalFirstThunk = (ULONG_PTR *)(ImageBase + pImportDescriptor->OriginalFirstThunk);
			ImageImportByName = *pOriginalFirstThunk;

			pFirstThunk = (ULONG_PTR *)(ImageBase + pImportDescriptor->FirstThunk);

			while(ImageImportByName)
			{
				if(!(ImageImportByName & IMAGE_ORDINAL_FLAG))
				{
					if((ULONG_PTR)pImportName & ~0xFFFF)
					{
						ImageImportByName += sizeof(WORD);

						if(lstrcmpA((char *)(ImageBase + ImageImportByName), pImportName) == 0)
							return (void **)pFirstThunk;
					}
				}
				else
				{
					if(((ULONG_PTR)pImportName & ~0xFFFF) == 0)
						if((ImageImportByName & 0xFFFF) == (ULONG_PTR)pImportName)
							return (void **)pFirstThunk;
				}

				pOriginalFirstThunk++;
				ImageImportByName = *pOriginalFirstThunk;

				pFirstThunk++;
			}
		}

		pImportDescriptor++;
	}

	return NULL;
}

void PatchPtr(void **ppAddress, void *pPtr)
{
	DWORD dwOldProtect, dwOtherProtect;

	VirtualProtect(ppAddress, sizeof(void *), PAGE_EXECUTE_READWRITE, &dwOldProtect);
	*ppAddress = pPtr;
	VirtualProtect(ppAddress, sizeof(void *), dwOldProtect, &dwOtherProtect);
}

void PatchMemory(void *pDest, void *pSrc, size_t nSize)
{
	DWORD dwOldProtect, dwOtherProtect;

	VirtualProtect(pDest, nSize, PAGE_EXECUTE_READWRITE, &dwOldProtect);
	CopyMemory(pDest, pSrc, nSize);
	VirtualProtect(pDest, nSize, dwOldProtect, &dwOtherProtect);
}

BOOL StringBeginsWith(WCHAR *pString, WCHAR *pBeginStr)
{
	do
	{
		if(*pBeginStr == L'\0')
			return TRUE;
	}
	while(*pString++ == *pBeginStr++);

	return FALSE;
}

// Taskbar functions

DWORD TaskbarGetPreference(LONG_PTR lpMMTaskListLongPtr)
{
/*
	0x00: Never combine
	0x01: Combine when taskbar is full
	0x03: Always combine, hide labels
	0x08: Animate
	0x10: Small buttons

	Multiple displays, show taskbar buttons on:
	0x100: All taskbars
	0x200: Main taskbar and taskbar where window is open
	0x400: Taskbar where window is open
*/

	if(lpMMTaskListLongPtr == lpTaskListLongPtr)
	{
		return *EV_TASK_SW_PREFERENCES;
	}
	else
	{
		LONG_PTR lpSecondaryTaskBandLongPtr = EV_MM_TASKLIST_SECONDARY_TASK_BAND_LONG_PTR_VALUE(lpMMTaskListLongPtr);
		return *EV_SECONDARY_TASK_BAND_PREFERENCES(lpSecondaryTaskBandLongPtr);
	}
}

LONG_PTR SecondaryTaskListGetFirstLongPtr(SECONDARY_TASK_LIST_GET *p_secondary_task_list_get)
{
	LONG_PTR lp;
	int nSecondaryTaskbarsCount;
	LONG_PTR *dpa_ptr;
	LONG_PTR lpSecondaryTaskListLongPtr;

	if(nWinVersion >= WIN_VERSION_8)
	{
		lp = *EV_TASK_SW_MULTI_TASK_LIST_REF;

		// DEF3264: CTaskListWndMulti::ActivateTask
		nSecondaryTaskbarsCount = *(int *)(lp + DO8_3264(0x14, 0x28, ,, ,, ,, 0x1C, 0x38, ,, 0x14, 0x28, 0x18, 0x30));
		if(nSecondaryTaskbarsCount > 0)
		{
			dpa_ptr = (*(LONG_PTR ***)(lp + DO8_3264(0x10, 0x20, ,, ,, ,, ,, ,, ,, 0x14, 0x28)))[1];

			lpSecondaryTaskListLongPtr = *dpa_ptr;
			lpSecondaryTaskListLongPtr -= DEF3264(0x14, 0x28);

			p_secondary_task_list_get->count = nSecondaryTaskbarsCount - 1;
			p_secondary_task_list_get->dpa_ptr = dpa_ptr + 1;

			return lpSecondaryTaskListLongPtr;
		}
	}

	return 0;
}

LONG_PTR SecondaryTaskListGetNextLongPtr(SECONDARY_TASK_LIST_GET *p_secondary_task_list_get)
{
	LONG_PTR lpSecondaryTaskListLongPtr;

	if(p_secondary_task_list_get->count > 0)
	{
		lpSecondaryTaskListLongPtr = *p_secondary_task_list_get->dpa_ptr;
		lpSecondaryTaskListLongPtr -= DEF3264(0x14, 0x28);

		p_secondary_task_list_get->count--;
		p_secondary_task_list_get->dpa_ptr++;

		return lpSecondaryTaskListLongPtr;
	}

	return 0;
}
