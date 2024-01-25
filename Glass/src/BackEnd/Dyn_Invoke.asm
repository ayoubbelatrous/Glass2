.code
dynamic_invoke PROC
	; rcx pointer, rdx arg_count, r8 args_pointer, r9 args_types_pointer

	push rbp
	mov rbp, rsp

	; save non volatile registers

	sub rsp, 32
	sub rsp, 48
	mov qword ptr [rbp - 8], rdi
	mov qword ptr [rbp - 16], rsi
	mov qword ptr [rbp - 24], r12
	mov qword ptr [rbp - 32], r13
	mov qword ptr [rbp - 40], r14
	mov qword ptr [rbp - 48], r15
	add rsp, 32
	add rsp, 48

	mov r14,  	rcx 	; pointer
	mov r13, 	rdx     ; arg_count
	mov r12, 	r8 	    ; arg_pointer
	mov r11, 	r9 	    ; args_types_pointer

	; compute stack needed for arguments (overestimated)
	mov r15, rdx
	imul r15, 8
	add r15, 32
	add r15, 48

	mov rax, r15
    ; calculate the remainder when dividing by 16
    mov rdx, 0  ; clear rdx for division
    mov rcx, 16
    div rcx
    ; Check if remainder is non-zero (number is not already aligned)

	test rdx, rdx
    jz aligned  ; if zero, already aligned

    ; if not aligned, calculate the aligned value
    sub rcx, rdx  ; subtract remainder from 16
    add r15, rcx  ; add the difference to align the number
aligned:

	sub rsp, r15

	; store allocated stack size in r15
	mov r15, rbp
	sub r15, rsp

	mov rdi, 32			; stack_top_offset
	mov rsi, 0

_loop:
	mov   r10, qword ptr [r11 + rsi * 8]

	; arg 0
	cmp rsi, 0
	jne skip_0
	mov   rcx, qword ptr [r12 + rsi * 8]
	cmp   r10, 0
	jne	  skip_f_0
	movss xmm0, dword ptr [r12 + rsi * 8]
skip_f_0:
	cmp   r10, 1
	jne	  skip_d_0
	movsd xmm0, qword ptr [r12 + rsi * 8]
skip_d_0:
skip_0:

	; arg 1
	cmp rsi, 1
	jne skip_1
	mov rdx, qword ptr [r12 + rsi * 8]
	cmp   r10, 0
	jne	  skip_f_1
	movss xmm1, dword ptr [r12 + rsi * 8]
skip_f_1:
	cmp   r10, 1
	jne	  skip_d_1
	movsd xmm1, qword ptr [r12 + rsi * 8]
skip_d_1:
skip_1:

	; arg 2
	cmp rsi, 2
	jne skip_2
	mov r8, qword ptr [r12 + rsi * 8]
	cmp   r10, 0
	jne	  skip_f_2
	movss xmm2, dword ptr [r12 + rsi * 8]
skip_f_2:
	cmp   r10, 1
	jne	  skip_d_2
	movsd xmm2, qword ptr [r12 + rsi * 8]
skip_d_2:
skip_2:

	; arg 3
	cmp rsi, 3
	jne skip_3
	mov r9, qword ptr [r12 + rsi * 8]
 	; float
	cmp   r10, 0
	jne	  skip_f_3
	movss xmm3, dword ptr [r12 + rsi * 8]
skip_f_3:
	; double
	cmp   r10, 1
	jne	  skip_d_3
	movsd xmm3, qword ptr [r12 + rsi * 8]
skip_d_3:
skip_3:

; stack args
	cmp rsi, 3
	jle skip_rest
	mov r10, qword ptr [r12 + rsi * 8]
	mov qword ptr [rsp + rdi], r10
	add	rdi, 8

skip_rest:
	inc rsi
	cmp rsi, r13
	jl _loop

	call r14

	add rsp, r15
	; restore scratch
	mov rdi, qword ptr [rbp - 8]
	mov rsi, qword ptr [rbp - 16]
	mov r12, qword ptr [rbp - 24]
	mov r13, qword ptr [rbp - 32]
	mov r14, qword ptr [rbp - 40]
	mov r15, qword ptr [rbp - 48]
	pop rbp
	ret
dynamic_invoke ENDP

END