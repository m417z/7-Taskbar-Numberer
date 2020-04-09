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

LoadDll:
.LoadLibrary:		      dd 0xdeadbeef
.GetProcAddress:	      dd 0xdeadbeef
.FreeLibrary:		      dd 0xdeadbeef

    ; we use edi as our index!
    push edi
    push esi
    push ebp
    push ebx
    _set_as_index edi

    _laptr_cmd .LoadLibrary,  lea ebx, dword [_the_label] ; save .LoadLibrary for
							  ; fast access and short code

    ; LoadLibrary(dll_str);
    _label_cmd dll_str,       push _the_label
    call dword [ebx]

    test eax, eax
    mov dword [ebx], eax
    je .end

    mov esi, eax ; save ret value
    _laptr_cmd .FreeLibrary,  mov ebp, dword [_the_label] ; save [.FreeLibrary] for
							  ; fast access and short code

    ; GetProcAddress(eax, exit_str);
    _label_cmd exit_str,      push _the_label
    push eax
    _laptr_cmd .GetProcAddress, call dword [_the_label]

    test eax, eax
    je .fail_and_free

    _laptr_cmd .GetProcAddress, xchg dword [_the_label], eax ; note the xchg!

    ; GetProcAddress(esi, init_str);
    _label_cmd init_str,      push _the_label
    push esi
    call eax

    test eax, eax
    je .fail_and_free

    ; Init(inject_init_struct);
    _label_cmd dll_str,       mov ecx, _the_label
@@:
    add ecx, 2		; we don't know the length
    cmp word [ecx-2], 0 ; of our dll_str, so
    jnz @b		; we gonna skip it manually!

    push ecx
    call eax

    test eax, eax
    je .fail_and_free

    ; return 0;
.end:
    pop ebx
    pop ebp
    pop esi
    pop edi
    xor eax, eax
    retn 4

.fail_and_free:
    mov dword [ebx], 0

    ; FreeLibrary(esi);
    push esi
    call ebp
    jmp .end

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Data
init_str	   db 'Init',0
exit_str	   db 'Exit',0
dll_str 	   db 'C:\test.dll',0
inject_init_struct db 'struct_goes_here',0
