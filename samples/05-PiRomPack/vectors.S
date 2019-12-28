
;@ ------------------------------------------------------------------
;@ ------------------------------------------------------------------
.equ	SCTLR_ENABLE_DATA_CACHE,        0x4
.equ	SCTLR_ENABLE_BRANCH_PREDICTION, 0x800
.equ	SCTLR_ENABLE_INSTRUCTION_CACHE, 0x1000
.equ	SCTLR_ENABLE_MMU,				0x1

// See ARM section A2.5 (Program status registers)
.equ    CPSR_IRQ_INHIBIT,       0x80
.equ    CPSR_FIQ_INHIBIT,       0x40
.equ    CPSR_THUMB,             0x20

.equ    CPSR_MODE_USER,         0x10
.equ    CPSR_MODE_FIQ,          0x11
.equ    CPSR_MODE_IRQ,          0x12
.equ    CPSR_MODE_SVR,          0x13
.equ    CPSR_MODE_ABORT,        0x17
.equ    CPSR_MODE_UNDEFINED,    0x1B
.equ    CPSR_MODE_SYSTEM,       0x1F

	.macro safe_svcmode_maskall reg:req
	
	mrs	\reg , cpsr
	eor	\reg, \reg, #0x1A		/* test for HYP mode */
	tst	\reg, #0x1F
	bic	\reg , \reg , #0x1F		/* clear mode bits */
	orr	\reg , \reg , #0xC0 | 0x13	/* mask IRQ/FIQ bits and set SVC mode */
	bne	1f				/* branch if not HYP mode */
	orr	\reg, \reg, #0x100		/* mask Abort bit */
	adr	lr, 2f
	msr	spsr_cxsf, \reg
	.word	0xE12EF30E			/* msr ELR_hyp, lr */
	.word	0xE160006E			/* eret */
1:	msr	cpsr_c, \reg
2:

	.endm

.globl start_l1cache
start_l1cache:
    mov r0, #0
    mcr p15, 0, r0, c7, c7, 0 ;@ invalidate caches
    mcr p15, 0, r0, c8, c7, 0 ;@ invalidate tlb
    mcr p15, 0, r2, c7,c10, 4 ;@ DSB ??
    mrc p15, 0, r0, c1, c0, 0
    orr r0,r0,#0x1000 ;@ instruction
    orr r0,r0,#0x0004 ;@ data
	orr r0,r0,#0x800  ;@ branch prediction
    mcr p15, 0, r0, c1, c0, 0
    bx lr	

.global _exit
_exit:
	bx lr
		