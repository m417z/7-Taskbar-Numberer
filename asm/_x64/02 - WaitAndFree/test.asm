format PE64 GUI 5.0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Code

WaitAndFreeDll:
.hHandle1:		      dq 0xdeeeeeeeeeadbeef
.hHandle2:		      dq 0xdeeeeeeeeeadbeef
.WaitForMultipleObjects:      dq 0xdeeeeeeeeeadbeef
.CloseHandle:		      dq 0xdeeeeeeeeeadbeef
.pExitProc:		      dq 0xdeeeeeeeeeadbeef
.hModule:		      dq 0xdeeeeeeeeeadbeef
.FreeLibrary:		      dq 0xdeeeeeeeeeadbeef
.VirtualFree:		      dq 0xdeeeeeeeeeadbeef

    ; Stack reserve and align
    sub rsp, 8*5

    ; WaitForMultipleObjects(2, lpHandles, FALSE, INFINITE);
    or r9d, 0xFFFFFFFF
    xor r8d, r8d
    lea rdx, [.hHandle1]
    mov ecx, 2
    call qword [.WaitForMultipleObjects]

    ; CloseHanele(hHandle1);
    mov rcx, [.hHandle1]
    call qword [.CloseHandle]

    ; CloseHanele(hHandle2);
    mov rcx, [.hHandle2]
    call qword [.CloseHandle]

    ; pExitProc();
    call qword [.pExitProc]

    ; FreeLibrary(hModule);
    mov rcx, [.hModule]
    call qword [.FreeLibrary]

    ; Stack restore
    add rsp, 8*5

    ; VirtualFree(start, NULL, MEM_RELEASE);
    mov r8d, 0x8000
    xor edx, edx
    lea rcx, [WaitAndFreeDll]
    jmp qword [.VirtualFree]
