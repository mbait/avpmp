#include "3dc.h"

void ADD_LL(LONGLONGCH *a, LONGLONGCH *b, LONGLONGCH *c);
void ADD_LL_PP(LONGLONGCH *c, LONGLONGCH *a);
void SUB_LL(LONGLONGCH *a, LONGLONGCH *b, LONGLONGCH *c);
void SUB_LL_MM(LONGLONGCH *c, LONGLONGCH *a);
void MUL_I_WIDE(int a, int b, LONGLONGCH *c);
int CMP_LL(LONGLONGCH *a, LONGLONGCH *b);
void EQUALS_LL(LONGLONGCH *a, LONGLONGCH *b);
void NEG_LL(LONGLONGCH *a);
void ASR_LL(LONGLONGCH *a, int shift);
void IntToLL(LONGLONGCH *a, int *b);
int MUL_FIXED(int a, int b);
int DIV_FIXED(int a, int b);

#define DIV_INT(a, b) ((a) / (b))

int NarrowDivide(LONGLONGCH *a, int b);
int WideMulNarrowDiv(int a, int b, int c);
void RotateVector_ASM(VECTORCH *v, MATRIXCH *m);
void RotateAndCopyVector_ASM(VECTORCH *v1, VECTORCH *v2, MATRIXCH *m);

#if 0
int FloatToInt(float);
#define f2i(a, b) { a = FloatToInt(b); }
#endif

void ADD_LL(LONGLONGCH *a, LONGLONGCH *b, LONGLONGCH *c)
{
/*
	_asm
	{
		mov esi,a
		mov edi,b
		mov ebx,c
		mov	eax,[esi]
		mov	edx,[esi+4]
		add	eax,[edi]
		adc	edx,[edi+4]
		mov	[ebx],eax
		mov	[ebx+4],edx
	}
*/

__asm__("movl	0(%%esi), %%eax		\n\t"
	"movl	4(%%esi), %%edx		\n\t"
	"addl	0(%%edi), %%eax		\n\t"
	"adcl	4(%%edi), %%edx		\n\t"
	"movl	%%eax, 0(%%ebx)		\n\t"
	"movl	%%edx, 4(%%ebx)		\n\t"
	: 
	: "S" (a), "D" (b), "b" (c)
	: "%eax", "%edx", "memory", "cc"
	);

/*
__asm__("movl	0(%%esi), %%eax		\n\t"
	"movl	4(%%esi), %%edx		\n\t"
	"addl	0(%%edi), %%eax		\n\t"
	"adcl	4(%%edi), %%edx		\n\t"
	: "=a" (c->lo32), "=d" (c->hi32)
	: "S" (a), "D" (b)
	);
*/
}

/* ADD ++ */

void ADD_LL_PP(LONGLONGCH *c, LONGLONGCH *a)
{
/*
	_asm
	{
		mov edi,c
		mov esi,a
		mov	eax,[esi]
		mov	edx,[esi+4]
		add	[edi],eax
		adc	[edi+4],edx
	}
*/
__asm__("movl	0(%%esi), %%eax		\n\t"
	"movl	4(%%esi), %%edx		\n\t"
	"addl	%%eax, 0(%%edi)		\n\t"
	"adcl	%%edx, 4(%%edi)		\n\t"
	:
	: "D" (c), "S" (a)
	: "%eax", "%edx", "memory", "cc"
	);
}

/* SUB */

void SUB_LL(LONGLONGCH *a, LONGLONGCH *b, LONGLONGCH *c)
{
/*
	_asm
	{
		mov esi,a
		mov edi,b
		mov ebx,c
		mov	eax,[esi]
		mov	edx,[esi+4]
		sub	eax,[edi]
		sbb	edx,[edi+4]
		mov	[ebx],eax
		mov	[ebx+4],edx
	}
*/
__asm__("movl	0(%%esi), %%eax		\n\t"
	"movl	4(%%esi), %%edx		\n\t"
	"subl	0(%%edi), %%eax		\n\t"
	"sbbl	4(%%edi), %%edx		\n\t"
	"movl	%%eax, 0(%%ebx)		\n\t"
	"movl	%%edx, 4(%%ebx)		\n\t"
	:
	: "S" (a), "D" (b), "b" (c)
	: "%eax", "%edx", "memory", "cc"
	);
}

/* SUB -- */

void SUB_LL_MM(LONGLONGCH *c, LONGLONGCH *a)
{
/*
	_asm
	{
		mov edi,c
		mov esi,a
		mov	eax,[esi]
		mov	edx,[esi+4]
		sub	[edi],eax
		sbb	[edi+4],edx
	}
*/
__asm__("movl	0(%%esi), %%eax		\n\t"
	"movl	4(%%esi), %%edx		\n\t"
	"subl	%%eax, 0(%%edi)		\n\t"
	"sbbl	%%edx, 4(%%edi)		\n\t"
	:
	: "D" (c), "S" (a)
	: "%eax", "%edx", "memory", "cc"
	);
}

/*

 MUL

 This is the multiply we use, the 32 x 32 = 64 widening version

*/

void MUL_I_WIDE(int a, int b, LONGLONGCH *c)
{
/*
	_asm
	{
		mov eax,a
		mov ebx,c
		imul b
		mov	[ebx],eax
		mov	[ebx+4],edx
	}
*/
__asm__("imull	%%edx			\n\t"
	"movl	%%eax, 0(%%ebx)		\n\t"
	"movl	%%edx, 4(%%ebx)		\n\t"
	:
	: "a" (a), "b" (c), "d" (b)
	: "memory", "cc"
	);
}

/*

 CMP

 This substitutes for ==, >, <, >=, <=

*/

int CMP_LL(LONGLONGCH *a, LONGLONGCH *b)
{
/*
	int retval;
	_asm
	{
		mov ebx,a
		mov ecx,b
		mov	eax,[ebx]
		mov	edx,[ebx+4]
		sub	eax,[ecx]
		sbb	edx,[ecx+4]
		and	edx,edx
		jne	llnz
		and	eax,eax
		je	llgs
		llnz:
		mov	retval,1
		and	edx,edx
		jge	llgs
		neg	retval
		llgs:
	}
*/
#if 0
int retval;
/* TODO */
__asm__("movl	0(%%ebx), %%eax		\n\t"
	"movl	4(%%ebx), %%edx		\n\t"
	"subl	0(%%ecx), %%eax		\n\t"
	"sbbl	4(%%ecx), %%edx		\n\t"
	"xorl	%%ebx, %%ebx            \n\t"
	"andl	%%edx, %%edx		\n\t"
	"jne	0			\n\t" /* llnz */
	"andl	%%eax, %%eax		\n\t"
	"je	1			\n"   /* llgs */
"0:					\n\t" /* llnz */
	"movl	$1, %%ebx		\n\t"
	"andl	%%edx, %%edx		\n\t"
	"jge	1			\n\t" /* llgs */
	"negl	%%ebx			\n"
"1:					\n\t" /* llgs */
	: "=b" (retval)
	: "b" (a), "c" (b)
	: "%eax", "%edx", "memory", "cc"
	);
	
	return retval;
#else
	if (a->hi32 > b->hi32)
		return 1;
	else if (a->hi32 < b->hi32)
		return -1;
	else if (a->lo32 > b->lo32)
		return 1;
	else if (a->lo32 < b->lo32)
		return -1;
	else
		return 0;
#endif		
}

/* EQUALS */

void EQUALS_LL(LONGLONGCH *a, LONGLONGCH *b)
{
/*
	_asm
	{
		mov edi,a
		mov esi,b
		mov	eax,[esi]
		mov	edx,[esi+4]
		mov	[edi],eax
		mov	[edi+4],edx
	}
*/
#if 0
__asm__("movl	0(%%esi), %%eax		\n\t"
	"movl	4(%%esi), %%edx		\n\t"
	"movl	%%eax, 0(%%edi)		\n\t"
	"movl	%%edx, 4(%%edi)		\n\t"
	:
	: "D" (a), "S" (b)
	: "%eax", "%edx", "memory"
	);
#else
	*a = *b;
#endif
}

/* NEGATE */

void NEG_LL(LONGLONGCH *a)
{
/*
	_asm
	{
		mov esi,a
		not	dword ptr[esi]
		not	dword ptr[esi+4]
		add	dword ptr[esi],1
		adc	dword ptr[esi+4],0
	}
*/
__asm__("notl	0(%%esi)		\n\t"
	"notl	4(%%esi)		\n\t"
	"addl	$1, 0(%%esi)		\n\t"
	"adcl	$0, 4(%%esi)		\n\t"
	:
	: "S" (a)
	: "memory", "cc"
	);
}

/* ASR */

void ASR_LL(LONGLONGCH *a, int shift)
{
/*
	_asm
	{
		mov esi,a
		mov eax,shift
		and	eax,eax
		jle	asrdn
		asrlp:
		sar	dword ptr[esi+4],1
		rcr	dword ptr[esi],1
		dec	eax
		jne	asrlp
		asrdn:
	}
*/
__asm__("andl	%%eax, %%eax		\n\t"
	"jle	0			\n" /* asrdn */
"1:					\n\t" /* asrlp */
	"sarl	$1, 4(%%esi)		\n\t"
	"rcrl	$1, 0(%%esi)		\n\t"
	"decl	%%eax			\n\t"
	"jne	1			\n"
"0:					\n\t"
	:
	: "S" (a), "a" (shift)
	: "memory", "cc"
	);
	
}

/* Convert int to LONGLONGCH */

void IntToLL(LONGLONGCH *a, int *b)
{
/*
	_asm
	{
		mov esi,b
		mov edi,a
		mov	eax,[esi]
		cdq
		mov	[edi],eax
		mov	[edi+4],edx
	}
*/
__asm__("movl	0(%%esi), %%eax		\n\t"
	"cdq				\n\t"
	"movl	%%eax, 0(%%edi)		\n\t"
	"movl	%%edx, 4(%%edi)		\n\t"
	:
	: "S" (b), "D" (a)
	: "%eax", "%edx", "memory", "cc"
	);

}

/*

 Fixed Point Multiply.


 16.16 * 16.16 -> 16.16
 or
 16.16 * 0.32 -> 0.32

 A proper version of this function ought to read
 16.16 * 16.16 -> 32.16
 but this would require a long long result

 Algorithm:

 Take the mid 32 bits of the 64 bit result

*/

/*
	These functions have been checked for suitability for 
	a Pentium and look as if they would work adequately.
	Might be worth a more detailed look at optimising
	them though.
*/

int MUL_FIXED(int a, int b)
{
	int retval;
/*
	_asm
	{
		mov eax,a
		imul b
		shrd eax,edx,16
		mov retval,eax
	}
*/
__asm__("imull	%%edx			\n\t"
	"shrdl	$16, %%edx, %%eax	\n\t"
	: "=a" (retval)
	: "a" (a), "d" (b)
	: "cc"
	);
	return retval;
}

/*

 Fixed Point Divide - returns a / b

*/

int DIV_FIXED(int a, int b)
{
	int retval;
	
	if (b == 0) return 0; /* TODO: debug this! (start with alien on ferarco) */
/*
	_asm
	{
		mov eax,a
		cdq
		rol eax,16
		mov dx,ax
		xor ax,ax
		idiv b
		mov retval,eax
	}
*/
__asm__("cdq				\n\t"
	"roll	$16, %%eax		\n\t"
	"mov	%%ax, %%dx		\n\t"
	"xor	%%ax, %%ax		\n\t"
	"idivl	%%ebx			\n\t"
	: "=a" (retval)
	: "a" (a), "b" (b)
	: "%edx", "cc"
	);
	return retval;
}

/*

 Multiply and Divide Functions.

*/


/*

 32/32 division

 This macro is a function on some other platforms

*/

#define DIV_INT(a, b) ((a) / (b))

/*

 A Narrowing 64/32 Division

*/

int NarrowDivide(LONGLONGCH *a, int b)
{
	int retval;
/*
	_asm
	{
		mov esi,a
		mov	eax,[esi]
		mov	edx,[esi+4]
		idiv	b
		mov retval,eax
	}
*/
__asm__("movl	0(%%esi), %%eax		\n\t"
	"movl	4(%%esi), %%edx		\n\t"
	"idivl	%%ebx			\n\t"
	: "=a" (retval)
	: "S" (a), "b" (b)
	: "%edx", "cc"
	);
	return retval;
}

/*

 This function performs a Widening Multiply followed by a Narrowing Divide.

 a = (a * b) / c

*/

int WideMulNarrowDiv(int a, int b, int c)
{
#if 0 /* TODO: broken? */
	int retval;
/*
	_asm
	{
		mov eax,a
		imul b
		idiv c
		mov retval,eax
	}
*/
/* TODO */
__asm__("imull	%2			\n\t"
	"idivl	%3			\n\t"
	: "=a" (retval)
	: "a" (a), "q" (b), "q" (c)
	: "cc"
	);	
	return retval;
#endif
	return (a * b) / c;	
}

/*

 Function to rotate a VECTORCH using a MATRIXCH

 This is the C function

	x =  MUL_FIXED(m->mat11, v->vx);
	x += MUL_FIXED(m->mat21, v->vy);
	x += MUL_FIXED(m->mat31, v->vz);

	y  = MUL_FIXED(m->mat12, v->vx);
	y += MUL_FIXED(m->mat22, v->vy);
	y += MUL_FIXED(m->mat32, v->vz);

	z  = MUL_FIXED(m->mat13, v->vx);
	z += MUL_FIXED(m->mat23, v->vy);
	z += MUL_FIXED(m->mat33, v->vz);

	v->vx = x;
	v->vy = y;
	v->vz = z;

 This is the MUL_FIXED inline assembler function

	imul edx
	shrd eax,edx,16


typedef struct matrixch {

	int mat11;	0
	int mat12;	4
	int mat13;	8

	int mat21;	12
	int mat22;	16
	int mat23;	20

	int mat31;	24
	int mat32;	28
	int mat33;	32

} MATRIXCH;

*/

/*

 Square Root

 Returns the Square Root of a 32-bit number

*/

extern int sqrt_temp1;
extern int sqrt_temp2;

int SqRoot32(int A)
{
#if 1
	sqrt_temp1 = A;
/*
	_asm
	{
		finit
		fild A
		fsqrt
		fistp temp2
		fwait
	}
*/

__asm__("finit				\n\t"
	"fildl	sqrt_temp1		\n\t"
	"fsqrt				\n\t"
	"fistpl	sqrt_temp2		\n\t"
	"fwait				\n\t"
	:
	:
	: "memory", "cc"
	);
	
	return sqrt_temp2;
#else
{ /* TODO: clean this please */
	double x = A;
	double retvald = sqrt(x);
	int retval = retvald;
	return retval;
}
#endif
}



/*

 This may look ugly (it is) but it is a MUCH faster way to convert "float" into "int" than
 the function call "CHP" used by the WATCOM compiler.

*/

extern float fti_fptmp;
extern int fti_itmp;

void FloatToInt()
{
#if 1
__asm__("fld	fti_fptmp		\n\t"
	"fistpl	fti_itmp		\n\t"
	:
	:
	: "memory", "cc"
	);
#else
	fti_itmp = (int)fti_fptmp;	
#endif
}

void TranslatePoint(float *source, float *dest, float *matrix)
{
//      fprintf(stderr, "TranslatePoint(%f, %f, %f)\n");

/* TODO - implement the inline assembly here? */
/* Moved it to a separate file because I can't figure out the damn syntax! */
/* This is currently not inlined for testing */
__asm__("call   TranslatePoint_Asm      \n\t"
        :
        : "S" (source), "b" (dest), "D" (matrix)
        );
}
