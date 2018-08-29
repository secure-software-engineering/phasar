	.section	__TEXT,__text,regular,pure_instructions
	.macosx_version_min 10, 13
	.globl	_pqueue_init
	.p2align	4, 0x90
_pqueue_init:                           ## @pqueue_init
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp0:
	.cfi_def_cfa_offset 16
Ltmp1:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp2:
	.cfi_def_cfa_register %rbp
	subq	$64, %rsp
	movl	$72, %eax
	movl	%eax, %r10d
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movq	%rdx, -32(%rbp)
	movq	%rcx, -40(%rbp)
	movq	%r8, -48(%rbp)
	movq	%r9, -56(%rbp)
	movq	%r10, %rdi
	callq	_malloc
	movq	%rax, -64(%rbp)
	cmpq	$0, %rax
	jne	LBB0_2
## BB#1:
	movq	$0, -8(%rbp)
	jmp	LBB0_5
LBB0_2:
	movq	-16(%rbp), %rax
	addq	$1, %rax
	shlq	$3, %rax
	movq	%rax, %rdi
	callq	_malloc
	movq	-64(%rbp), %rdi
	movq	%rax, 64(%rdi)
	cmpq	$0, %rax
	jne	LBB0_4
## BB#3:
	movq	-64(%rbp), %rax
	movq	%rax, %rdi
	callq	_free
	movq	$0, -8(%rbp)
	jmp	LBB0_5
LBB0_4:
	movq	-64(%rbp), %rax
	movq	$1, (%rax)
	movq	-16(%rbp), %rax
	addq	$1, %rax
	movq	-64(%rbp), %rcx
	movq	%rax, 16(%rcx)
	movq	-64(%rbp), %rcx
	movq	%rax, 8(%rcx)
	movq	-24(%rbp), %rax
	movq	-64(%rbp), %rcx
	movq	%rax, 24(%rcx)
	movq	-40(%rbp), %rax
	movq	-64(%rbp), %rcx
	movq	%rax, 40(%rcx)
	movq	-32(%rbp), %rax
	movq	-64(%rbp), %rcx
	movq	%rax, 32(%rcx)
	movq	-48(%rbp), %rax
	movq	-64(%rbp), %rcx
	movq	%rax, 48(%rcx)
	movq	-56(%rbp), %rax
	movq	-64(%rbp), %rcx
	movq	%rax, 56(%rcx)
	movq	-64(%rbp), %rax
	movq	%rax, -8(%rbp)
LBB0_5:
	movq	-8(%rbp), %rax
	addq	$64, %rsp
	popq	%rbp
	retq
	.cfi_endproc

	.globl	_pqueue_free
	.p2align	4, 0x90
_pqueue_free:                           ## @pqueue_free
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp3:
	.cfi_def_cfa_offset 16
Ltmp4:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp5:
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rdi
	movq	64(%rdi), %rdi
	callq	_free
	movq	-8(%rbp), %rdi
	callq	_free
	addq	$16, %rsp
	popq	%rbp
	retq
	.cfi_endproc

	.globl	_pqueue_size
	.p2align	4, 0x90
_pqueue_size:                           ## @pqueue_size
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp6:
	.cfi_def_cfa_offset 16
Ltmp7:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp8:
	.cfi_def_cfa_register %rbp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rdi
	movq	(%rdi), %rdi
	subq	$1, %rdi
	movq	%rdi, %rax
	popq	%rbp
	retq
	.cfi_endproc

	.globl	_pqueue_insert
	.p2align	4, 0x90
_pqueue_insert:                         ## @pqueue_insert
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp9:
	.cfi_def_cfa_offset 16
Ltmp10:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp11:
	.cfi_def_cfa_register %rbp
	subq	$48, %rsp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	cmpq	$0, -16(%rbp)
	jne	LBB3_2
## BB#1:
	movl	$1, -4(%rbp)
	jmp	LBB3_7
LBB3_2:
	movq	-16(%rbp), %rax
	movq	(%rax), %rax
	movq	-16(%rbp), %rcx
	cmpq	8(%rcx), %rax
	jb	LBB3_6
## BB#3:
	movq	-16(%rbp), %rax
	movq	(%rax), %rax
	movq	-16(%rbp), %rcx
	addq	16(%rcx), %rax
	movq	%rax, -48(%rbp)
	movq	-16(%rbp), %rax
	movq	64(%rax), %rax
	movq	-48(%rbp), %rcx
	shlq	$3, %rcx
	movq	%rax, %rdi
	movq	%rcx, %rsi
	callq	_realloc
	movq	%rax, -32(%rbp)
	cmpq	$0, %rax
	jne	LBB3_5
## BB#4:
	movl	$1, -4(%rbp)
	jmp	LBB3_7
LBB3_5:
	movq	-32(%rbp), %rax
	movq	-16(%rbp), %rcx
	movq	%rax, 64(%rcx)
	movq	-48(%rbp), %rax
	movq	-16(%rbp), %rcx
	movq	%rax, 8(%rcx)
LBB3_6:
	movq	-16(%rbp), %rax
	movq	(%rax), %rcx
	movq	%rcx, %rdx
	addq	$1, %rdx
	movq	%rdx, (%rax)
	movq	%rcx, -40(%rbp)
	movq	-24(%rbp), %rax
	movq	-40(%rbp), %rcx
	movq	-16(%rbp), %rdx
	movq	64(%rdx), %rdx
	movq	%rax, (%rdx,%rcx,8)
	movq	-16(%rbp), %rdi
	movq	-40(%rbp), %rsi
	callq	_bubble_up
	movl	$0, -4(%rbp)
LBB3_7:
	movl	-4(%rbp), %eax
	addq	$48, %rsp
	popq	%rbp
	retq
	.cfi_endproc

	.p2align	4, 0x90
_bubble_up:                             ## @bubble_up
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp12:
	.cfi_def_cfa_offset 16
Ltmp13:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp14:
	.cfi_def_cfa_register %rbp
	subq	$64, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-16(%rbp), %rsi
	movq	-8(%rbp), %rdi
	movq	64(%rdi), %rdi
	movq	(%rdi,%rsi,8), %rsi
	movq	%rsi, -32(%rbp)
	movq	-8(%rbp), %rsi
	movq	32(%rsi), %rsi
	movq	-32(%rbp), %rdi
	callq	*%rsi
	movq	%rax, -40(%rbp)
	movq	-16(%rbp), %rax
	shrq	$1, %rax
	movq	%rax, -24(%rbp)
LBB4_1:                                 ## =>This Inner Loop Header: Depth=1
	xorl	%eax, %eax
	movb	%al, %cl
	cmpq	$1, -16(%rbp)
	movb	%cl, -41(%rbp)          ## 1-byte Spill
	jbe	LBB4_3
## BB#2:                                ##   in Loop: Header=BB4_1 Depth=1
	movq	-8(%rbp), %rax
	movq	24(%rax), %rax
	movq	-8(%rbp), %rcx
	movq	32(%rcx), %rcx
	movq	-24(%rbp), %rdx
	movq	-8(%rbp), %rsi
	movq	64(%rsi), %rsi
	movq	(%rsi,%rdx,8), %rdi
	movq	%rax, -56(%rbp)         ## 8-byte Spill
	callq	*%rcx
	movq	-40(%rbp), %rsi
	movq	%rax, %rdi
	movq	-56(%rbp), %rax         ## 8-byte Reload
	callq	*%rax
	cmpl	$0, %eax
	setne	%r8b
	movb	%r8b, -41(%rbp)         ## 1-byte Spill
LBB4_3:                                 ##   in Loop: Header=BB4_1 Depth=1
	movb	-41(%rbp), %al          ## 1-byte Reload
	testb	$1, %al
	jne	LBB4_4
	jmp	LBB4_6
LBB4_4:                                 ##   in Loop: Header=BB4_1 Depth=1
	movq	-24(%rbp), %rax
	movq	-8(%rbp), %rcx
	movq	64(%rcx), %rcx
	movq	(%rcx,%rax,8), %rax
	movq	-16(%rbp), %rcx
	movq	-8(%rbp), %rdx
	movq	64(%rdx), %rdx
	movq	%rax, (%rdx,%rcx,8)
	movq	-8(%rbp), %rax
	movq	56(%rax), %rax
	movq	-16(%rbp), %rcx
	movq	-8(%rbp), %rdx
	movq	64(%rdx), %rdx
	movq	(%rdx,%rcx,8), %rdi
	movq	-16(%rbp), %rsi
	callq	*%rax
## BB#5:                                ##   in Loop: Header=BB4_1 Depth=1
	movq	-24(%rbp), %rax
	movq	%rax, -16(%rbp)
	movq	-16(%rbp), %rax
	shrq	$1, %rax
	movq	%rax, -24(%rbp)
	jmp	LBB4_1
LBB4_6:
	movq	-32(%rbp), %rax
	movq	-16(%rbp), %rcx
	movq	-8(%rbp), %rdx
	movq	64(%rdx), %rdx
	movq	%rax, (%rdx,%rcx,8)
	movq	-8(%rbp), %rax
	movq	56(%rax), %rax
	movq	-32(%rbp), %rdi
	movq	-16(%rbp), %rsi
	callq	*%rax
	addq	$64, %rsp
	popq	%rbp
	retq
	.cfi_endproc

	.globl	_pqueue_change_priority
	.p2align	4, 0x90
_pqueue_change_priority:                ## @pqueue_change_priority
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp15:
	.cfi_def_cfa_offset 16
Ltmp16:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp17:
	.cfi_def_cfa_register %rbp
	subq	$48, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movq	-8(%rbp), %rdx
	movq	32(%rdx), %rdx
	movq	-24(%rbp), %rdi
	callq	*%rdx
	movq	%rax, -40(%rbp)
	movq	-8(%rbp), %rax
	movq	40(%rax), %rax
	movq	-24(%rbp), %rdi
	movq	-16(%rbp), %rsi
	callq	*%rax
	movq	-8(%rbp), %rax
	movq	48(%rax), %rax
	movq	-24(%rbp), %rdi
	callq	*%rax
	movq	%rax, -32(%rbp)
	movq	-8(%rbp), %rax
	movq	24(%rax), %rax
	movq	-40(%rbp), %rdi
	movq	-16(%rbp), %rsi
	callq	*%rax
	cmpl	$0, %eax
	je	LBB5_2
## BB#1:
	movq	-8(%rbp), %rdi
	movq	-32(%rbp), %rsi
	callq	_bubble_up
	jmp	LBB5_3
LBB5_2:
	movq	-8(%rbp), %rdi
	movq	-32(%rbp), %rsi
	callq	_percolate_down
LBB5_3:
	addq	$48, %rsp
	popq	%rbp
	retq
	.cfi_endproc

	.p2align	4, 0x90
_percolate_down:                        ## @percolate_down
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp18:
	.cfi_def_cfa_offset 16
Ltmp19:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp20:
	.cfi_def_cfa_register %rbp
	subq	$64, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-16(%rbp), %rsi
	movq	-8(%rbp), %rdi
	movq	64(%rdi), %rdi
	movq	(%rdi,%rsi,8), %rsi
	movq	%rsi, -32(%rbp)
	movq	-8(%rbp), %rsi
	movq	32(%rsi), %rsi
	movq	-32(%rbp), %rdi
	callq	*%rsi
	movq	%rax, -40(%rbp)
LBB6_1:                                 ## =>This Inner Loop Header: Depth=1
	movq	-8(%rbp), %rdi
	movq	-16(%rbp), %rsi
	callq	_maxchild
	xorl	%ecx, %ecx
	movb	%cl, %dl
	movq	%rax, -24(%rbp)
	cmpq	$0, %rax
	movb	%dl, -41(%rbp)          ## 1-byte Spill
	je	LBB6_3
## BB#2:                                ##   in Loop: Header=BB6_1 Depth=1
	movq	-8(%rbp), %rax
	movq	24(%rax), %rax
	movq	-40(%rbp), %rdi
	movq	-8(%rbp), %rcx
	movq	32(%rcx), %rcx
	movq	-24(%rbp), %rdx
	movq	-8(%rbp), %rsi
	movq	64(%rsi), %rsi
	movq	(%rsi,%rdx,8), %rdx
	movq	%rdi, -56(%rbp)         ## 8-byte Spill
	movq	%rdx, %rdi
	movq	%rax, -64(%rbp)         ## 8-byte Spill
	callq	*%rcx
	movq	-56(%rbp), %rdi         ## 8-byte Reload
	movq	%rax, %rsi
	movq	-64(%rbp), %rax         ## 8-byte Reload
	callq	*%rax
	cmpl	$0, %eax
	setne	%r8b
	movb	%r8b, -41(%rbp)         ## 1-byte Spill
LBB6_3:                                 ##   in Loop: Header=BB6_1 Depth=1
	movb	-41(%rbp), %al          ## 1-byte Reload
	testb	$1, %al
	jne	LBB6_4
	jmp	LBB6_5
LBB6_4:                                 ##   in Loop: Header=BB6_1 Depth=1
	movq	-24(%rbp), %rax
	movq	-8(%rbp), %rcx
	movq	64(%rcx), %rcx
	movq	(%rcx,%rax,8), %rax
	movq	-16(%rbp), %rcx
	movq	-8(%rbp), %rdx
	movq	64(%rdx), %rdx
	movq	%rax, (%rdx,%rcx,8)
	movq	-8(%rbp), %rax
	movq	56(%rax), %rax
	movq	-16(%rbp), %rcx
	movq	-8(%rbp), %rdx
	movq	64(%rdx), %rdx
	movq	(%rdx,%rcx,8), %rdi
	movq	-16(%rbp), %rsi
	callq	*%rax
	movq	-24(%rbp), %rax
	movq	%rax, -16(%rbp)
	jmp	LBB6_1
LBB6_5:
	movq	-32(%rbp), %rax
	movq	-16(%rbp), %rcx
	movq	-8(%rbp), %rdx
	movq	64(%rdx), %rdx
	movq	%rax, (%rdx,%rcx,8)
	movq	-8(%rbp), %rax
	movq	56(%rax), %rax
	movq	-32(%rbp), %rdi
	movq	-16(%rbp), %rsi
	callq	*%rax
	addq	$64, %rsp
	popq	%rbp
	retq
	.cfi_endproc

	.globl	_pqueue_remove
	.p2align	4, 0x90
_pqueue_remove:                         ## @pqueue_remove
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp21:
	.cfi_def_cfa_offset 16
Ltmp22:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp23:
	.cfi_def_cfa_register %rbp
	subq	$48, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-8(%rbp), %rsi
	movq	48(%rsi), %rsi
	movq	-16(%rbp), %rdi
	callq	*%rsi
	movq	%rax, -24(%rbp)
	movq	-8(%rbp), %rax
	movq	(%rax), %rsi
	movq	%rsi, %rdi
	addq	$-1, %rdi
	movq	%rdi, (%rax)
	movq	-8(%rbp), %rax
	movq	64(%rax), %rax
	movq	-8(%rax,%rsi,8), %rax
	movq	-24(%rbp), %rsi
	movq	-8(%rbp), %rdi
	movq	64(%rdi), %rdi
	movq	%rax, (%rdi,%rsi,8)
	movq	-8(%rbp), %rax
	movq	24(%rax), %rax
	movq	-8(%rbp), %rsi
	movq	32(%rsi), %rsi
	movq	-16(%rbp), %rdi
	movq	%rax, -32(%rbp)         ## 8-byte Spill
	callq	*%rsi
	movq	-8(%rbp), %rsi
	movq	32(%rsi), %rsi
	movq	-24(%rbp), %rdi
	movq	-8(%rbp), %rcx
	movq	64(%rcx), %rcx
	movq	(%rcx,%rdi,8), %rdi
	movq	%rax, -40(%rbp)         ## 8-byte Spill
	callq	*%rsi
	movq	-40(%rbp), %rdi         ## 8-byte Reload
	movq	%rax, %rsi
	movq	-32(%rbp), %rax         ## 8-byte Reload
	callq	*%rax
	cmpl	$0, %eax
	je	LBB7_2
## BB#1:
	movq	-8(%rbp), %rdi
	movq	-24(%rbp), %rsi
	callq	_bubble_up
	jmp	LBB7_3
LBB7_2:
	movq	-8(%rbp), %rdi
	movq	-24(%rbp), %rsi
	callq	_percolate_down
LBB7_3:
	xorl	%eax, %eax
	addq	$48, %rsp
	popq	%rbp
	retq
	.cfi_endproc

	.globl	_pqueue_pop
	.p2align	4, 0x90
_pqueue_pop:                            ## @pqueue_pop
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp24:
	.cfi_def_cfa_offset 16
Ltmp25:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp26:
	.cfi_def_cfa_register %rbp
	subq	$32, %rsp
	movq	%rdi, -16(%rbp)
	cmpq	$0, -16(%rbp)
	je	LBB8_2
## BB#1:
	movq	-16(%rbp), %rax
	cmpq	$1, (%rax)
	jne	LBB8_3
LBB8_2:
	movq	$0, -8(%rbp)
	jmp	LBB8_4
LBB8_3:
	movl	$1, %eax
	movl	%eax, %esi
	movq	-16(%rbp), %rcx
	movq	64(%rcx), %rcx
	movq	8(%rcx), %rcx
	movq	%rcx, -24(%rbp)
	movq	-16(%rbp), %rcx
	movq	(%rcx), %rdx
	movq	%rdx, %rdi
	addq	$-1, %rdi
	movq	%rdi, (%rcx)
	movq	-16(%rbp), %rcx
	movq	64(%rcx), %rcx
	movq	-8(%rcx,%rdx,8), %rcx
	movq	-16(%rbp), %rdx
	movq	64(%rdx), %rdx
	movq	%rcx, 8(%rdx)
	movq	-16(%rbp), %rdi
	callq	_percolate_down
	movq	-24(%rbp), %rcx
	movq	%rcx, -8(%rbp)
LBB8_4:
	movq	-8(%rbp), %rax
	addq	$32, %rsp
	popq	%rbp
	retq
	.cfi_endproc

	.globl	_pqueue_peek
	.p2align	4, 0x90
_pqueue_peek:                           ## @pqueue_peek
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp27:
	.cfi_def_cfa_offset 16
Ltmp28:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp29:
	.cfi_def_cfa_register %rbp
	movq	%rdi, -16(%rbp)
	cmpq	$0, -16(%rbp)
	je	LBB9_2
## BB#1:
	movq	-16(%rbp), %rax
	cmpq	$1, (%rax)
	jne	LBB9_3
LBB9_2:
	movq	$0, -8(%rbp)
	jmp	LBB9_4
LBB9_3:
	movq	-16(%rbp), %rax
	movq	64(%rax), %rax
	movq	8(%rax), %rax
	movq	%rax, -24(%rbp)
	movq	-24(%rbp), %rax
	movq	%rax, -8(%rbp)
LBB9_4:
	movq	-8(%rbp), %rax
	popq	%rbp
	retq
	.cfi_endproc

	.globl	_pqueue_dump
	.p2align	4, 0x90
_pqueue_dump:                           ## @pqueue_dump
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp30:
	.cfi_def_cfa_offset 16
Ltmp31:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp32:
	.cfi_def_cfa_register %rbp
	pushq	%rbx
	subq	$88, %rsp
Ltmp33:
	.cfi_offset %rbx, -24
	leaq	L_.str(%rip), %rax
	movq	___stdoutp@GOTPCREL(%rip), %rcx
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movq	%rdx, -32(%rbp)
	movq	(%rcx), %rdi
	movq	%rax, %rsi
	movb	$0, %al
	callq	_fprintf
	movl	$1, -36(%rbp)
	movl	%eax, -40(%rbp)         ## 4-byte Spill
LBB10_1:                                ## =>This Inner Loop Header: Depth=1
	movslq	-36(%rbp), %rax
	movq	-16(%rbp), %rcx
	cmpq	(%rcx), %rax
	jae	LBB10_4
## BB#2:                                ##   in Loop: Header=BB10_1 Depth=1
	movq	___stdoutp@GOTPCREL(%rip), %rax
	movq	(%rax), %rdi
	movl	-36(%rbp), %edx
	movl	-36(%rbp), %ecx
	shll	$1, %ecx
	movl	-36(%rbp), %esi
	shll	$1, %esi
	addl	$1, %esi
	movl	-36(%rbp), %r8d
	sarl	$1, %r8d
	movq	-16(%rbp), %rax
	movslq	-36(%rbp), %r9
	movq	%rdi, -48(%rbp)         ## 8-byte Spill
	movq	%rax, %rdi
	movl	%esi, -52(%rbp)         ## 4-byte Spill
	movq	%r9, %rsi
	movl	%r8d, -56(%rbp)         ## 4-byte Spill
	movl	%edx, -60(%rbp)         ## 4-byte Spill
	movl	%ecx, -64(%rbp)         ## 4-byte Spill
	callq	_maxchild
	leaq	L_.str.1(%rip), %rsi
	movl	%eax, %ecx
	movq	-48(%rbp), %rdi         ## 8-byte Reload
	movl	-60(%rbp), %edx         ## 4-byte Reload
	movl	-64(%rbp), %r8d         ## 4-byte Reload
	movl	%ecx, -68(%rbp)         ## 4-byte Spill
	movl	%r8d, %ecx
	movl	-52(%rbp), %r8d         ## 4-byte Reload
	movl	-56(%rbp), %r9d         ## 4-byte Reload
	movl	-68(%rbp), %r10d        ## 4-byte Reload
	movl	%r10d, (%rsp)
	movb	$0, %al
	callq	_fprintf
	movq	-32(%rbp), %rsi
	movq	-24(%rbp), %rdi
	movslq	-36(%rbp), %r11
	movq	-16(%rbp), %rbx
	movq	64(%rbx), %rbx
	movq	(%rbx,%r11,8), %r11
	movq	%rsi, -80(%rbp)         ## 8-byte Spill
	movq	%r11, %rsi
	movq	-80(%rbp), %r11         ## 8-byte Reload
	movl	%eax, -84(%rbp)         ## 4-byte Spill
	callq	*%r11
## BB#3:                                ##   in Loop: Header=BB10_1 Depth=1
	movl	-36(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -36(%rbp)
	jmp	LBB10_1
LBB10_4:
	addq	$88, %rsp
	popq	%rbx
	popq	%rbp
	retq
	.cfi_endproc

	.p2align	4, 0x90
_maxchild:                              ## @maxchild
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp34:
	.cfi_def_cfa_offset 16
Ltmp35:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp36:
	.cfi_def_cfa_register %rbp
	subq	$48, %rsp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movq	-24(%rbp), %rsi
	shlq	$1, %rsi
	movq	%rsi, -32(%rbp)
	movq	-32(%rbp), %rsi
	movq	-16(%rbp), %rdi
	cmpq	(%rdi), %rsi
	jb	LBB11_2
## BB#1:
	movq	$0, -8(%rbp)
	jmp	LBB11_6
LBB11_2:
	movq	-32(%rbp), %rax
	addq	$1, %rax
	movq	-16(%rbp), %rcx
	cmpq	(%rcx), %rax
	jae	LBB11_5
## BB#3:
	movq	-16(%rbp), %rax
	movq	24(%rax), %rax
	movq	-16(%rbp), %rcx
	movq	32(%rcx), %rcx
	movq	-32(%rbp), %rdx
	movq	-16(%rbp), %rsi
	movq	64(%rsi), %rsi
	movq	(%rsi,%rdx,8), %rdi
	movq	%rax, -40(%rbp)         ## 8-byte Spill
	callq	*%rcx
	movq	-16(%rbp), %rcx
	movq	32(%rcx), %rcx
	movq	-32(%rbp), %rdx
	movq	-16(%rbp), %rsi
	movq	64(%rsi), %rsi
	movq	8(%rsi,%rdx,8), %rdi
	movq	%rax, -48(%rbp)         ## 8-byte Spill
	callq	*%rcx
	movq	-48(%rbp), %rdi         ## 8-byte Reload
	movq	%rax, %rsi
	movq	-40(%rbp), %rax         ## 8-byte Reload
	callq	*%rax
	cmpl	$0, %eax
	je	LBB11_5
## BB#4:
	movq	-32(%rbp), %rax
	addq	$1, %rax
	movq	%rax, -32(%rbp)
LBB11_5:
	movq	-32(%rbp), %rax
	movq	%rax, -8(%rbp)
LBB11_6:
	movq	-8(%rbp), %rax
	addq	$48, %rsp
	popq	%rbp
	retq
	.cfi_endproc

	.globl	_pqueue_print
	.p2align	4, 0x90
_pqueue_print:                          ## @pqueue_print
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp37:
	.cfi_def_cfa_offset 16
Ltmp38:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp39:
	.cfi_def_cfa_register %rbp
	subq	$64, %rsp
	leaq	_set_pri(%rip), %rcx
	leaq	_set_pos(%rip), %r9
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	%rdx, -24(%rbp)
	movq	-8(%rbp), %rdx
	movq	(%rdx), %rdi
	movq	-8(%rbp), %rdx
	movq	24(%rdx), %rsi
	movq	-8(%rbp), %rdx
	movq	32(%rdx), %rdx
	movq	-8(%rbp), %rax
	movq	48(%rax), %r8
	callq	_pqueue_init
	movq	$-1, %rcx
	movq	%rax, -32(%rbp)
	movq	-8(%rbp), %rax
	movq	(%rax), %rax
	movq	-32(%rbp), %rdx
	movq	%rax, (%rdx)
	movq	-8(%rbp), %rax
	movq	8(%rax), %rax
	movq	-32(%rbp), %rdx
	movq	%rax, 8(%rdx)
	movq	-8(%rbp), %rax
	movq	16(%rax), %rax
	movq	-32(%rbp), %rdx
	movq	%rax, 16(%rdx)
	movq	-32(%rbp), %rax
	movq	64(%rax), %rax
	movq	-8(%rbp), %rdx
	movq	64(%rdx), %rdx
	movq	-8(%rbp), %rsi
	movq	(%rsi), %rsi
	shlq	$3, %rsi
	movq	%rax, %rdi
	movq	%rsi, -48(%rbp)         ## 8-byte Spill
	movq	%rdx, %rsi
	movq	-48(%rbp), %rdx         ## 8-byte Reload
	callq	___memcpy_chk
	movq	%rax, -56(%rbp)         ## 8-byte Spill
LBB12_1:                                ## =>This Inner Loop Header: Depth=1
	movq	-32(%rbp), %rdi
	callq	_pqueue_pop
	movq	%rax, -40(%rbp)
	cmpq	$0, %rax
	je	LBB12_3
## BB#2:                                ##   in Loop: Header=BB12_1 Depth=1
	movq	-24(%rbp), %rax
	movq	-16(%rbp), %rdi
	movq	-40(%rbp), %rsi
	callq	*%rax
	jmp	LBB12_1
LBB12_3:
	movq	-32(%rbp), %rdi
	callq	_pqueue_free
	addq	$64, %rsp
	popq	%rbp
	retq
	.cfi_endproc

	.p2align	4, 0x90
_set_pri:                               ## @set_pri
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp40:
	.cfi_def_cfa_offset 16
Ltmp41:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp42:
	.cfi_def_cfa_register %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	popq	%rbp
	retq
	.cfi_endproc

	.p2align	4, 0x90
_set_pos:                               ## @set_pos
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp43:
	.cfi_def_cfa_offset 16
Ltmp44:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp45:
	.cfi_def_cfa_register %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	popq	%rbp
	retq
	.cfi_endproc

	.globl	_pqueue_is_valid
	.p2align	4, 0x90
_pqueue_is_valid:                       ## @pqueue_is_valid
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp46:
	.cfi_def_cfa_offset 16
Ltmp47:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp48:
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movl	$1, %esi
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rdi
	callq	_subtree_is_valid
	addq	$16, %rsp
	popq	%rbp
	retq
	.cfi_endproc

	.p2align	4, 0x90
_subtree_is_valid:                      ## @subtree_is_valid
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp49:
	.cfi_def_cfa_offset 16
Ltmp50:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp51:
	.cfi_def_cfa_register %rbp
	subq	$64, %rsp
	movq	%rdi, -16(%rbp)
	movl	%esi, -20(%rbp)
	movl	-20(%rbp), %esi
	shll	$1, %esi
	movslq	%esi, %rdi
	movq	-16(%rbp), %rax
	cmpq	(%rax), %rdi
	jae	LBB16_6
## BB#1:
	movq	-16(%rbp), %rax
	movq	24(%rax), %rax
	movq	-16(%rbp), %rcx
	movq	32(%rcx), %rcx
	movslq	-20(%rbp), %rdx
	movq	-16(%rbp), %rsi
	movq	64(%rsi), %rsi
	movq	(%rsi,%rdx,8), %rdi
	movq	%rax, -32(%rbp)         ## 8-byte Spill
	callq	*%rcx
	movq	-16(%rbp), %rcx
	movq	32(%rcx), %rcx
	movl	-20(%rbp), %r8d
	shll	$1, %r8d
	movslq	%r8d, %rdx
	movq	-16(%rbp), %rsi
	movq	64(%rsi), %rsi
	movq	(%rsi,%rdx,8), %rdi
	movq	%rax, -40(%rbp)         ## 8-byte Spill
	callq	*%rcx
	movq	-40(%rbp), %rdi         ## 8-byte Reload
	movq	%rax, %rsi
	movq	-32(%rbp), %rax         ## 8-byte Reload
	callq	*%rax
	cmpl	$0, %eax
	je	LBB16_3
## BB#2:
	movl	$0, -4(%rbp)
	jmp	LBB16_13
LBB16_3:
	movq	-16(%rbp), %rdi
	movl	-20(%rbp), %eax
	shll	$1, %eax
	movl	%eax, %esi
	callq	_subtree_is_valid
	cmpl	$0, %eax
	jne	LBB16_5
## BB#4:
	movl	$0, -4(%rbp)
	jmp	LBB16_13
LBB16_5:
	jmp	LBB16_6
LBB16_6:
	movl	-20(%rbp), %eax
	shll	$1, %eax
	addl	$1, %eax
	movslq	%eax, %rcx
	movq	-16(%rbp), %rdx
	cmpq	(%rdx), %rcx
	jae	LBB16_12
## BB#7:
	movq	-16(%rbp), %rax
	movq	24(%rax), %rax
	movq	-16(%rbp), %rcx
	movq	32(%rcx), %rcx
	movslq	-20(%rbp), %rdx
	movq	-16(%rbp), %rsi
	movq	64(%rsi), %rsi
	movq	(%rsi,%rdx,8), %rdi
	movq	%rax, -48(%rbp)         ## 8-byte Spill
	callq	*%rcx
	movq	-16(%rbp), %rcx
	movq	32(%rcx), %rcx
	movl	-20(%rbp), %r8d
	shll	$1, %r8d
	addl	$1, %r8d
	movslq	%r8d, %rdx
	movq	-16(%rbp), %rsi
	movq	64(%rsi), %rsi
	movq	(%rsi,%rdx,8), %rdi
	movq	%rax, -56(%rbp)         ## 8-byte Spill
	callq	*%rcx
	movq	-56(%rbp), %rdi         ## 8-byte Reload
	movq	%rax, %rsi
	movq	-48(%rbp), %rax         ## 8-byte Reload
	callq	*%rax
	cmpl	$0, %eax
	je	LBB16_9
## BB#8:
	movl	$0, -4(%rbp)
	jmp	LBB16_13
LBB16_9:
	movq	-16(%rbp), %rdi
	movl	-20(%rbp), %eax
	shll	$1, %eax
	addl	$1, %eax
	movl	%eax, %esi
	callq	_subtree_is_valid
	cmpl	$0, %eax
	jne	LBB16_11
## BB#10:
	movl	$0, -4(%rbp)
	jmp	LBB16_13
LBB16_11:
	jmp	LBB16_12
LBB16_12:
	movl	$1, -4(%rbp)
LBB16_13:
	movl	-4(%rbp), %eax
	addq	$64, %rsp
	popq	%rbp
	retq
	.cfi_endproc

	.section	__TEXT,__cstring,cstring_literals
L_.str:                                 ## @.str
	.asciz	"posn\tleft\tright\tparent\tmaxchild\t...\n"

L_.str.1:                               ## @.str.1
	.asciz	"%d\t%d\t%d\t%d\t%ul\t"


.subsections_via_symbols
