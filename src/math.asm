BITS 32
SECTION .text

GLOBAL TranslatePoint_Asm

; float *source, float *dest, float *matrix
; "S" (source), "b" (dest), "d" (matrix)
TranslatePoint_Asm:
	fld	dword [esi]
	fmul	dword [edi]
	fld	dword [esi+4]
	fmul	dword [edi+4]
	fld	dword [esi+8]
	fmul	dword [edi+8]
	fxch	st1
	faddp	st2, st0
	fld	dword [esi]
	fmul	dword [edi+16]
	fxch	st1
	faddp	st2, st0
	fld	dword [esi+4]
	fmul	dword [edi+20]
	fld	dword [esi+8]
	fmul	dword [edi+24]
	fxch	st1
	faddp	st2, st0
	fld	dword [esi]
	fmul	dword [edi+32]
	fxch	st1
	faddp	st2, st0
	fld	dword [esi+4]
	fmul	dword [edi+36]
	fld	dword [esi+8]
	fmul	dword [edi+40]
	fxch	st1
	faddp	st2, st0
	fxch	st3
	fadd	dword [edi+12]
	fxch	st1
	faddp	st3, st0
	fxch	st1
	fadd	dword [edi+28]
	fxch	st2
	fadd	dword [edi+44]
	fxch	st1
	fstp	dword [ebx]
	fxch	st1
	fstp	dword [ebx+4]
	fstp	dword [ebx+8]
	
	ret
	