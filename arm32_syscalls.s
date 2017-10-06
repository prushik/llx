;arm32_syscalls v1
;
;This is assembly glue to allow C programs to call syscalls without
;needing a C library to help
;
;v1 initial version

.text
	.global __syscall, syscall0, syscall1, syscall2, syscall3, syscall4, syscall5, syscall6, syscall_list

	syscall_list:
		mov			rax,rdi			@syscall number
		mov			rdi,[rsi]		@arg1
		mov			rdx,[rsi+16]	@arg3
		mov			r10,[rsi+24]	@arg4
		mov			r8,[rsi+32]		@arg5
		mov			r9,[rsi+40]		@arg6
		mov			rsi,[rsi+8]		@arg2
		syscall						@
		ret							@

	@
	syscall0:
	syscall1:
	syscall2:
		swi			$0				@
		ret							@

	syscall3:
		mov			rax,rdi			@syscall number
		mov			rdi,rsi			@arg1
		mov			rsi,rdx			@arg2
		mov			rdx,rcx			@arg3
		syscall						@
		ret							@

	syscall4:
		mov			rax,rdi			@syscall number
		mov			rdi,rsi			@arg1
		mov			rsi,rdx			@arg2
		mov			rdx,rcx			@arg3
		mov			r10,r8			@arg4
		syscall						@
		ret							@

	syscall5:
	__syscall:
		mov			rax,rdi			@syscall number
		mov			rdi,rsi			@arg1
		mov			rsi,rdx			@arg2
		mov			rdx,rcx			@arg3
		mov			r10,r8			@arg4
		mov			r8,r9			@arg5
		syscall						@
		ret							@

	syscall6:
		mov			rax,rdi			@syscall number
		mov			rdi,rsi			@arg1
		mov			rsi,rdx			@arg2
		mov			rdx,rcx			@arg3
		mov			r10,r8			@arg4
		mov			r8,r9			@arg5
		mov			r9,[rsp+8]		@arg6
		syscall						@
		ret							@
.data
	text: .ascii	"ARM \n"

.text
	.globl _start

	_start:
		mov %r0, $1		/*; stdout */
		ldr %r1, =text	/*; data */
		mov %r2, $5		/*; length */
		mov %r7, $4		/*; syscall number 4 (write) */
		swi $0			/*; syscall */

		mov	%r7, $1		/*; syscall number 1 (exit) */
		swi $0			/*; syscall */
