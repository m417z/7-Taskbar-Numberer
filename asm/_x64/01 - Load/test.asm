format PE64 GUI 5.0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Code

LoadDll:
.LoadLibrary:		      dq 0xdeeeeeeeeeadbeef
.GetProcAddress:	      dq 0xdeeeeeeeeeadbeef
.FreeLibrary:		      dq 0xdeeeeeeeeeadbeef

    ; Stack reserve and align
    push rbx
    sub rsp, 8*4

    ; LoadLibrary(dll_str);
    lea rcx, [dll_str]
    call qword [.LoadLibrary]

    test rax, rax
    mov qword [.LoadLibrary], rax
    je .end

    mov rbx, rax

    ; GetProcAddress(rax, exit_str);
    mov rcx, rax
    lea rdx, [exit_str]
    call qword [.GetProcAddress]

    test rax, rax
    je .fail_and_free

    xchg qword [.GetProcAddress], rax ; note the xchg!

    ; GetProcAddress(rbx, init_str);
    mov rcx, rbx
    lea rdx, [init_str]
    call rax

    test rax, rax
    je .fail_and_free

    ; Init(inject_init_struct);
    lea rcx, [dll_str]
@@:
    add rcx, 2		; we don't know the length
    cmp word [rcx-2], 0 ; of our dll_str, so
    jnz @b		; we gonna skip it manually!
    call rax

    test rax, rax
    je .fail_and_free

    ; return;
.end:
    add rsp, 8*4
    pop rbx
    xor eax, eax
    retn

.fail_and_free:
    mov qword [.LoadLibrary], 0

    ; FreeLibrary(rbx);
    mov rcx, rbx
    call qword [.FreeLibrary]
    jmp .end

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Data
init_str	   db 'Init',0
exit_str	   db 'Exit',0
dll_str 	   db 'C:\test.dll',0
inject_init_struct db 'struct_goes_here',0
