[bits 64]

global syscall_entry
extern syscall_handler

syscall_entry:
    ; We need a kernel stack! 
    ; For a quick hack, we can use the current thread's stack.
    ; But we are in ring 0 now, and RSP still points to the user stack!
    ; We must use swapgs if we set up MSR_KERNEL_GS_BASE.
    ; Since this is a simple OS, let's just use a static kernel stack for syscalls for now.
    ; Warning: This prevents nested syscalls or preempting inside a syscall, but it works for a hobby OS demo.
    
    mov [user_rsp], rsp
    lea rsp, [syscall_stack + 4096]

    push rcx ; User RIP
    push r11 ; User RFLAGS
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15

    ; rdi, rsi, rdx, r10, r8, r9 already contain arguments for SysV ABI.
    ; Linux syscall ABI uses r10 instead of rcx for 4th arg.
    mov rcx, r10

    call syscall_handler

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    pop r11
    pop rcx
    
    mov rsp, [user_rsp]
    
    o64 sysret

section .bss
user_rsp: resq 1
syscall_stack: resb 4096
