;math.s v1
;
;Very simple math library. Only a few math functions are implemented
;and most use the inefficient FPU co-processor
;sin, cos, and atan2 are implemented because they were needed
;sincos is implemented because some GCC optimizations assume it exists
;sgn is implemented because its real simple
;
;This is the initial version
;Todo: implement more and better

section .text
	global sin, cos, sincos, atan2, sgn

	sin:
		sub	rsp,16					; Make room for the value
		movsd	QWORD [rsp],xmm0	;
		fld	QWORD [rsp]				; ST0
		fsin						;
		fstp	QWORD [rsp]			; ST0
		movsd	xmm0,QWORD [rsp]	;
		add	rsp,16					;
		ret							;

	cos:
		sub	rsp,16					;
		movsd	QWORD [rsp],xmm0	;
		fld	QWORD [rsp]				;
		fcos						;
		fstp	QWORD [rsp]			;
		movsd	xmm0,QWORD [rsp]	;
		add	rsp,16					;
		ret

	sincos:
		sub	rsp,16					;
		movsd	QWORD [rsp],xmm0	;
		fld	QWORD [rsp]				;
		fsincos						;
		fstp	QWORD [rsi]			;
		fstp	QWORD [rdi]			;
		add	rsp,16					;
		ret							;

	sgn:
		cmp	edi,0					;
		jg	.GT						;
		jl	.LT						;
		mov	rax,0					;
		ret							;
	.GT:	mov	rax,1				;
		ret							;
	.LT:	mov	rax,-1				;
		ret							;

	atan2:
		sub	rsp,16					;
		movsd	QWORD [rsp],xmm0	;
		fld	QWORD [rsp]				;
		movsd	QWORD [rsp],xmm1	;
		fld	QWORD [rsp]				;
		fpatan						;
		fstp	QWORD [rsp]			;
		movsd	xmm0,QWORD [rsp]	;
		add	rsp,16					;
		ret							;
