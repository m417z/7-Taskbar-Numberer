#include "explorer_inject.h"

static volatile BOOL bInjected = FALSE;
static HWND hTaskbarWnd;
static HANDLE hExplorerProcess;
static HANDLE hCleanEvent;
static HANDLE hRemoteWaitThread;
static volatile HANDLE hWaitThread;

WCHAR *ExplorerInject(int nViewOption)
{
	WCHAR szDllFileName[MAX_PATH];
	HANDLE hExplorerIsShellMutex;
	DWORD dwProcessId;
	HMODULE hLoadedDll;
	HANDLE hRemoteEvent, hRemoteCurrentProcess;
	void *pExitProc;
	WCHAR *pError;
	int i;

	// Get DLL path
	i = GetModuleFileName(NULL, szDllFileName, MAX_PATH);
	while(i-- && szDllFileName[i] != L'\\');
	lstrcpy(&szDllFileName[i+1], L"inject.dll");

	if(GetFileAttributes(szDllFileName) == INVALID_FILE_ATTRIBUTES)
		return L"IDS_INJERROR_NODLL";

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

	// Find and open explorer process
	hTaskbarWnd = FindWindow(L"Shell_TrayWnd", NULL);
	if(!hTaskbarWnd)
		return L"IDS_INJERROR_NOTBAR";

	GetWindowThreadProcessId(hTaskbarWnd, &dwProcessId);

	hExplorerProcess = OpenProcess(
		PROCESS_CREATE_THREAD|PROCESS_VM_OPERATION|PROCESS_VM_READ|PROCESS_VM_WRITE|
		PROCESS_DUP_HANDLE|PROCESS_QUERY_INFORMATION|SYNCHRONIZE, FALSE, dwProcessId
	);
	if(!hExplorerProcess)
		return L"IDS_INJERROR_EXPROC";

	// Wait for explorer to initialize in case it didn't
	WaitForInputIdle(hExplorerProcess, INFINITE);

	// Create an event (that we are going to signal on exit)
	hCleanEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// Duplicate some handles
	DuplicateHandle(GetCurrentProcess(), hCleanEvent, hExplorerProcess, &hRemoteEvent, EVENT_MODIFY_STATE|SYNCHRONIZE, FALSE, 0);
	DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), hExplorerProcess, &hRemoteCurrentProcess, SYNCHRONIZE, FALSE, 0);

	// The real thing, load the dll in explorer
	hLoadedDll = LoadLibraryInExplorer(hExplorerProcess, szDllFileName, hRemoteEvent, nViewOption, &pExitProc);
	if(hLoadedDll)
	{
		// Now inject the code that will clean up when we signal or crash
		hRemoteWaitThread = InjectCleanCodeInExplorer(hExplorerProcess, hLoadedDll, hRemoteEvent, hRemoteCurrentProcess, pExitProc);
		if(hRemoteWaitThread)
		{
			// Create our thread that will clean stuff up
			hWaitThread = CreateThread(NULL, 0, WaitThread, NULL, CREATE_SUSPENDED, NULL);
			if(hWaitThread)
			{
				ResumeThread(hWaitThread);
				bInjected = TRUE;
				return 0;
			}
			else
				pError = L"IDS_INJERROR_X3";

			SetEvent(hCleanEvent);

			WaitForSingleObject(hRemoteWaitThread, INFINITE);
			CloseHandle(hRemoteWaitThread);
		}
		else
			pError = L"IDS_INJERROR_X3";
	}
	else
		pError = L"IDS_INJERROR_LOADDLL";

	CloseHandle(hCleanEvent);
	DuplicateHandle(hExplorerProcess, hRemoteEvent, NULL, NULL, 0, FALSE, DUPLICATE_CLOSE_SOURCE);
	DuplicateHandle(hExplorerProcess, hRemoteCurrentProcess, NULL, NULL, 0, FALSE, DUPLICATE_CLOSE_SOURCE);
	CloseHandle(hExplorerProcess);

	return pError;
}

static HMODULE LoadLibraryInExplorer(HANDLE hProcess, WCHAR *pDllName, HANDLE hEvent, int nViewOption, void **ppExitProc)
{
	void *pCodeVars[] = {
		LoadLibrary, GetProcAddress, FreeLibrary
	};
	int nDllNameSize;
	void *pInjectAddr;
	HANDLE hThread;
	struct {
		HMODULE hModule;
		void *pExit;
	} thread_result;

	// Allocate some memory and copy our code
	nDllNameSize = (lstrlen(pDllName)+1)*sizeof(WCHAR);

	pInjectAddr = VirtualAllocEx(hProcess, NULL, sizeof(ASM_CODE_LOAD)-1+nDllNameSize+sizeof(int), 
		MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if(!pInjectAddr)
		return NULL;

	if(
		!MyWriteProcessMemory(hProcess, pInjectAddr, ASM_CODE_LOAD, sizeof(ASM_CODE_LOAD)-1) || 
		!MyWriteProcessMemory(hProcess, pInjectAddr, pCodeVars, sizeof(pCodeVars)) || 
		!MyWriteProcessMemory(hProcess, (char *)pInjectAddr+sizeof(ASM_CODE_LOAD)-1, pDllName, nDllNameSize) || 
		!MyWriteProcessMemory(hProcess, (char *)pInjectAddr+sizeof(ASM_CODE_LOAD)-1+nDllNameSize, &nViewOption, sizeof(int))
	)
	{
		VirtualFreeEx(hProcess, pInjectAddr, 0, MEM_RELEASE);
		return NULL;
	}

	// Create a thread to run the code
	hThread = CreateRemoteThread(hProcess, NULL, 0, 
		(LPTHREAD_START_ROUTINE)((char *)pInjectAddr+sizeof(pCodeVars)), NULL, 0, NULL);
	if(!hThread)
	{
		VirtualFreeEx(hProcess, pInjectAddr, 0, MEM_RELEASE);
		return NULL;
	}

	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);

	// Read the result
	MyReadProcessMemory(hProcess, pInjectAddr, &thread_result, 2*sizeof(void *));
	VirtualFreeEx(hProcess, pInjectAddr, 0, MEM_RELEASE);

	if(!thread_result.hModule)
		return NULL;

	*ppExitProc = thread_result.pExit;
	return thread_result.hModule;
}

static HANDLE InjectCleanCodeInExplorer(HANDLE hProcess, HMODULE hDllModule, HANDLE hEvent, HANDLE hCurrentProcess, void *pExitProc)
{
	void *pCodeVars[] = {
		hEvent, hCurrentProcess, WaitForMultipleObjects, CloseHandle, 
		pExitProc, hDllModule, FreeLibrary, VirtualFree
	};
	void *pInjectAddr;
	HANDLE hThread;

	// Allocate some memory and copy our code
	pInjectAddr = VirtualAllocEx(hProcess, NULL, sizeof(ASM_CODE_WAITANDFREE)-1, MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if(!pInjectAddr)
		return NULL;

	if(
		!MyWriteProcessMemory(hProcess, pInjectAddr, ASM_CODE_WAITANDFREE, sizeof(ASM_CODE_WAITANDFREE)-1) || 
		!MyWriteProcessMemory(hProcess, pInjectAddr, pCodeVars, sizeof(pCodeVars))
	)
	{
		VirtualFreeEx(hProcess, pInjectAddr, 0, MEM_RELEASE);
		return NULL;
	}

	// Create a thread to run the code
	hThread = CreateRemoteThread(hProcess, NULL, 0, 
		(LPTHREAD_START_ROUTINE)((char *)pInjectAddr+sizeof(pCodeVars)), NULL, 0, NULL);
	if(!hThread)
	{
		VirtualFreeEx(hProcess, pInjectAddr, 0, MEM_RELEASE);
		return NULL;
	}

	return hThread;
}

static DWORD WINAPI WaitThread(LPVOID lpParameter)
{
	HANDLE hHandles[2];
	HANDLE hThread;

	hHandles[0] = hCleanEvent;
	hHandles[1] = hExplorerProcess;

	if(WaitForMultipleObjects(2, hHandles, FALSE, INFINITE) == WAIT_OBJECT_0)
		WaitForSingleObject(hRemoteWaitThread, INFINITE);

	CloseHandle(hRemoteWaitThread);
	CloseHandle(hCleanEvent);
	CloseHandle(hExplorerProcess);

	hThread = InterlockedExchangePointer(&hWaitThread, NULL);
	if(hThread)
		CloseHandle(hThread);

	bInjected = FALSE;
	return 0;
}

static BOOL MyReadProcessMemory(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize)
{
	SIZE_T nNumberOfBytesRead;

	return ReadProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, &nNumberOfBytesRead) && 
		nNumberOfBytesRead == nSize;
}

static BOOL MyWriteProcessMemory(HANDLE hProcess, LPVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize)
{
	SIZE_T nNumberOfBytesWritten;

	return WriteProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, &nNumberOfBytesWritten) && 
		nNumberOfBytesWritten == nSize;
}

BOOL ExplorerIsInjected()
{
	return bInjected;
}

HWND ExplorerGetTaskbarWnd()
{
	return hTaskbarWnd;
}

void ExplorerCleanup()
{
	HANDLE hThread;

	SetEvent(hCleanEvent);

	hThread = InterlockedExchangePointer(&hWaitThread, NULL);
	if(hThread)
	{
		WaitForSingleObject(hThread, INFINITE);
		CloseHandle(hThread);
	}
}
