#ifndef _WND_PROC_H_
#define _WND_PROC_H_

#include <windows.h>

BOOL WndProcInit(void);
void WndProcExit(void);
void WndProcWaitTillDone(void);
void InvalidateSecondaryTaskListWndRect();

#endif // _WND_PROC_H_
