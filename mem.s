;mem.s v1
;
;This is some assembly for memory operations. Mainly intended for when fast memory
;copying is needed or when the optimizer wants to add dependency on these symbols
;
;This is the first version
;Todo: more functions

section .text
	global memcpy

	;fill the dest memory normally, but forget about returning the start of memory
	memfcpy:
		mov			rax,[rsi]		;store data from src in rax
		mov			[rdi],rax		;move from rax to destination
		add			rsi,4			;point rsi to next address
		add			rdi,4			;point rdi to next address
		dec			rdx				;decrement count
		jnz			memfcpy			;if count is not zero, continue
		ret							;return end of src memory cause thats whats there

	;this one fills the memory backwards, allowing rax to contain the destination at the end
	membcpy:
		add			rsi,rdx			;point rsi to end of src
		add			rdi,rdx			;point rdi to end of dest
		mov			rax,[rsi]		;store data from src in rax
		mov			[rdi],rax		;move from rax to destination
		sub			rsi,4			;point rsi to next address
		sub			rdi,4			;point rdi to next address
		dec			rdx				;decrement count
		jnz			membcpy			;if count is not zero, continue
		mov			rax,[rdi]		;put dest into rax for returning
		ret							;return end of src memory cause thats whats there

	;this one fills the memory backwards, allowing rax to contain the destination at the end
	memcpy:
		add			rsi,rdx			;point rsi to end of src
		add			rdi,rdx			;point rdi to end of dest
;		xor			rax,rax			;zero out rax
		prefetcht0	[rsi]			;prefetch source data
	memcpy_loop:
		mov			rax,[rsi]		;store data from src in rax
		mov			[rdi],rax		;move from rax to destination
		sub			rsi,1			;point rsi to next address
		sub			rdi,1			;point rdi to next address
		dec			rdx				;decrement count
		jnz			memcpy_loop		;if count is not zero, continue
		mov			rax,[rdi]		;put dest into rax for returning
		ret							;return end of src memory cause thats whats there



