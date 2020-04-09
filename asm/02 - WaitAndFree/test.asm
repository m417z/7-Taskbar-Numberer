format PE GUI 4.0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Macros for position-independence

macro _set_as_index reg
{
    index_reg equ reg

    call $+5
    index_reg_value = $
    pop index_reg
}

macro _set_other_as_index reg
{
    mov reg, index_reg

    restore index_reg
    index_reg equ reg
}

macro _label_cmd label, [cmd]
{
common
    if label-index_reg_value <> 0
	add index_reg, label-index_reg_value
	index_reg_value = label
    end if

    _the_label equ index_reg
    cmd
    restore _the_label
}

macro _laptr_cmd label, [cmd]
{
common
    _the_label equ index_reg+label-index_reg_value
    cmd
    restore _the_label
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Code

WaitAndFreeDll:
.hHandle1:		      dd 0xdeadbeef
.hHandle2:		      dd 0xdeadbeef
.WaitForMultipleObjects:      dd 0xdeadbeef
.CloseHandle:		      dd 0xdeadbeef
.pExitProc:		      dd 0xdeadbeef
.hModule:		      dd 0xdeadbeef
.FreeLibrary:		      dd 0xdeadbeef
.VirtualFree:		      dd 0xdeadbeef

    ; we use edi as our index!
    push edi
    _set_as_index edi

    ; WaitForMultipleObjects(2, lpHandles, FALSE, INFINITE);
    push -1
    push 0
    _label_cmd .hHandle1,     push _the_label
    push 2
    _laptr_cmd .WaitForMultipleObjects, call dword [_the_label]

    ; CloseHanele(hHandle1);
    _laptr_cmd .hHandle1,     push dword [_the_label]
    _laptr_cmd .CloseHandle,  call dword [_the_label]

    ; CloseHanele(hHandle2);
    _laptr_cmd .hHandle2,     push dword [_the_label]
    _laptr_cmd .CloseHandle,  call dword [_the_label]

    ; pExitProc();
    _laptr_cmd .pExitProc,    call dword [_the_label]

    ; FreeLibrary(hModule);
    _laptr_cmd .hModule,      push dword [_the_label]
    _laptr_cmd .FreeLibrary,  call dword [_the_label]

    ; VirtualFree(start, NULL, MEM_RELEASE);
    _set_other_as_index eax
    pop edi

    pop ecx ; Return address
    pop edx ; lParam
    push 0x8000
    push 0
    _label_cmd WaitAndFreeDll, push _the_label
    push ecx
    _laptr_cmd .VirtualFree,  jmp dword [_the_label]
