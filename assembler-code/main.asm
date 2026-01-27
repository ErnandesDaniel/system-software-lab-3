bits 64
default rel
section .text

global main
extern time
extern srand
extern rand
extern putchar
extern putchar
extern getchar
extern putchar
extern putchar
extern puts
extern printf
extern putchar
extern printf
extern printf

main:
    push rbp
    mov rbp, rsp
    sub rsp, 216
BB_0:
    mov eax, 1
    mov [rbp + -8], eax
    mov eax, [rbp + -8]
    mov [rbp + -12], eax
    mov eax, 5
    mov [rbp + -24], eax
    mov eax, [rbp + -12]
    mov ebx, [rbp + -24]
    cmp eax, ebx
    setl al
    movzx eax, al
    mov [rbp + -28], eax
    jmp BB_1
BB_1:
    mov eax, 5
    mov [rbp + -36], eax
    mov eax, [rbp + -12]
    mov ebx, [rbp + -36]
    cmp eax, ebx
    setl al
    movzx eax, al
    mov [rbp + -40], eax
    mov eax, [rbp + -40]
    cmp eax, 0
    jne BB_2
    jmp BB_3
BB_2:
    mov eax, 1
    mov [rbp + -48], eax
    mov eax, [rbp + -12]
    mov ebx, [rbp + -48]
    add eax, ebx
    mov [rbp + -52], eax
    mov eax, [rbp + -52]
    mov [rbp + -12], eax
    jmp BB_1
BB_3:
    mov eax, 0
    mov [rbp + -60], eax
    mov ecx, [rbp + -60]
    sub rsp, 32
    call time
    add rsp, 32
    mov [rbp + -64], eax
    mov eax, [rbp + -64]
    mov [rbp + -68], eax
    mov ecx, [rbp + -68]
    sub rsp, 32
    call srand
    add rsp, 32
    mov [rbp + -80], eax
    sub rsp, 32
    call rand
    add rsp, 32
    mov [rbp + -84], eax
    mov eax, [rbp + -84]
    mov [rbp + -88], eax
    mov eax, 65
    mov [rbp + -96], eax
    mov ecx, [rbp + -96]
    sub rsp, 32
    call putchar
    add rsp, 32
    mov [rbp + -100], eax
    mov eax, 10
    mov [rbp + -104], eax
    mov ecx, [rbp + -104]
    sub rsp, 32
    call putchar
    add rsp, 32
    mov [rbp + -108], eax
    sub rsp, 32
    call getchar
    add rsp, 32
    mov [rbp + -112], eax
    mov eax, [rbp + -112]
    mov [rbp + -116], eax
    mov ecx, [rbp + -116]
    sub rsp, 32
    call putchar
    add rsp, 32
    mov [rbp + -128], eax
    mov eax, 10
    mov [rbp + -132], eax
    mov ecx, [rbp + -132]
    sub rsp, 32
    call putchar
    add rsp, 32
    mov [rbp + -136], eax
    lea rax, [str_0]
    mov [rbp + -140], rax
    mov rax, [rbp + -140]
    mov [rbp + -144], rax
    mov rcx, [rbp + -144]
    sub rsp, 32
    call puts
    add rsp, 32
    mov [rbp + -156], eax
    lea rax, [str_1]
    mov [rbp + -160], rax
    mov eax, 10
    mov [rbp + -164], eax
    mov rcx, [rbp + -160]
    mov edx, [rbp + -164]
    sub rsp, 32
    call printf
    add rsp, 32
    mov [rbp + -168], eax
    mov eax, 10
    mov [rbp + -172], eax
    mov ecx, [rbp + -172]
    sub rsp, 32
    call putchar
    add rsp, 32
    mov [rbp + -176], eax
    lea rax, [str_2]
    mov [rbp + -180], rax
    mov eax, 65
    mov [rbp + -184], eax
    mov rcx, [rbp + -180]
    mov edx, [rbp + -184]
    sub rsp, 32
    call printf
    add rsp, 32
    mov [rbp + -188], eax
    lea rax, [str_3]
    mov [rbp + -192], rax
    mov eax, 65
    mov [rbp + -196], eax
    mov rcx, [rbp + -192]
    mov edx, [rbp + -196]
    sub rsp, 32
    call printf
    add rsp, 32
    mov [rbp + -200], eax
    mov eax, [rbp + -88]
; Очистка стека и возврат
    leave       ; эквивалент: mov rsp, rbp; pop rbp
    ret         ; возвращаем eax как результат

section .data
str_0 db 72, 101, 108, 108, 111, 44, 32, 87, 111, 114, 108, 100, 33, 0
str_1 db 37, 100, 0
str_2 db 37, 99, 10, 0
str_3 db 37, 100, 0
