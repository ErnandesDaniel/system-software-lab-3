bits 64
default rel
section .text

global square

square:
    push rbp
    mov rbp, rsp
    sub rsp, 40
BB_0:
    mov eax, ecx
    mov ebx, ecx
    imul eax, ebx
    mov [rbp + -20], eax
    mov eax, [rbp + -20]
; Очистка стека и возврат
    leave       ; эквивалент: mov rsp, rbp; pop rbp
    ret         ; возвращаем eax как результат
