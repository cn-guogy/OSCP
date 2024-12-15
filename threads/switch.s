/* switch.s 
 *   	与机器相关的上下文切换例程。请勿修改这些！
 *
 *	上下文切换本质上是与机器相关的，因为
 *	要保存的寄存器、如何设置初始
 *	调用帧等，都是特定于处理器架构的。
 *
 * 	此文件当前支持以下架构：
 *	    DEC MIPS
 *	    SUN SPARC
 *	    HP PA-RISC
 *	    Intel 386
 *	    DEC ALPHA
 *
 * 我们为每个架构定义两个例程：
 *
 * ThreadRoot(InitialPC, InitialArg, WhenDonePC, StartupPC)
 *	InitialPC  - 要在此线程中运行的过程的程序计数器。
 *      InitialArg - 线程的单个参数。
 *	WhenDonePC - 线程返回时要调用的例程。
 *	StartupPC  - 启动线程时要调用的例程。
 *
 *	ThreadRoot 从 SWITCH() 例程调用以首次启动
 *	一个线程。
 *
 * SWITCH(oldThread, newThread)
 * 	oldThread  - 当前正在运行的线程，CPU寄存器状态
 *		将被保存到此处。
 * 	newThread  - 要运行的新线程，CPU寄存器状态
 *		将从此处加载。
 */

/*
 版权所有 (c) 1992-1993 加州大学校董会。
 保留所有权利。请参阅 以获取版权声明和责任限制
 和免责声明条款。
 */

#if defined(HOST_i386) && defined(HOST_LINUX) && defined(HOST_ELF)
#define _ThreadRoot ThreadRoot
#define _SWITCH SWITCH
#endif


#include "switch.h"

#ifdef HOST_MIPS

/* 符号寄存器名称 */
#define z       $0      /* 零寄存器 */
#define a0      $4      /* 参数寄存器 */
#define a1      $5
#define s0      $16     /* 被调用者保存 */
#define s1      $17
#define s2      $18
#define s3      $19
#define s4      $20
#define s5      $21
#define s6      $22
#define s7      $23
#define sp      $29     /* 堆栈指针 */
#define fp      $30     /* 帧指针 */
#define ra      $31     /* 返回地址 */

        .text   
        .align  2

	.globl ThreadRoot
	.ent	ThreadRoot,0
ThreadRoot:
	or	fp,z,z		# 在这里清除帧指针
				# 使 gdb 的线程堆栈回溯
				# 在这里结束（我希望如此！）

	jal	StartupPC	# 调用启动过程
	move	a0, InitialArg
	jal	InitialPC	# 调用主过程
	jal 	WhenDonePC	# 当我们完成时，调用清理过程

	# 从未到达
	.end ThreadRoot

	# a0 -- 指向旧线程的指针
	# a1 -- 指向新线程的指针
	.globl SWITCH
	.ent	SWITCH,0
SWITCH:
	sw	sp, SP(a0)		# 保存新堆栈指针
	sw	s0, S0(a0)		# 保存所有被调用者保存的寄存器
	sw	s1, S1(a0)
	sw	s2, S2(a0)
	sw	s3, S3(a0)
	sw	s4, S4(a0)
	sw	s5, S5(a0)
	sw	s6, S6(a0)
	sw	s7, S7(a0)
	sw	fp, FP(a0)		# 保存帧指针
	sw	ra, PC(a0)		# 保存返回地址
	
	lw	sp, SP(a1)		# 加载新堆栈指针
	lw	s0, S0(a1)		# 加载被调用者保存的寄存器
	lw	s1, S1(a1)
	lw	s2, S2(a1)
	lw	s3, S3(a1)
	lw	s4, S4(a1)
	lw	s5, S5(a1)
	lw	s6, S6(a1)
	lw	s7, S7(a1)
	lw	fp, FP(a1)
	lw	ra, PC(a1)		# 加载返回地址	

	j	ra
	.end SWITCH
#endif HOST_MIPS

#ifdef HOST_SPARC

/* 注意！这些文件似乎在 Solaris 上不存在 --
 * 你需要找到（SPARC 特定的）MINFRAME、ST_FLUSH_WINDOWS 等等
 * 的定义。（我没有 Solaris 机器，所以我无法告诉。）
 */
#include <sun4/trap.h>
#include <sun4/asm_linkage.h>
.seg    "text"

/* 特殊的 SPARC：
 *	ThreadRoot 的前两个指令被跳过，因为
 *	ThreadRoot 的地址被设置为 SWITCH() 的返回地址
 *	通过例程 Thread::StackAllocate。SWITCH() 在这里跳转到
 *	“ret”指令，实际上是在“jmp %o7+8”。这 8 跳过了
 *	例程开头的两个 nops。
 */

.globl	_ThreadRoot
_ThreadRoot:
	nop  ; nop         /* 这两个 nops 被跳过，因为我们被调用
			    * 使用 jmp+8 指令。 */
	clr	%fp        /* 清除帧指针使 gdb 的回溯
	                    * 线程堆栈在这里结束。 */
			   /* 当前参数在输出寄存器中，我们
			    * 将它们保存到本地寄存器中，以便在调用时不会被
			    * 破坏。 */
	mov	InitialPC, %l0  
	mov	InitialArg, %l1
	mov	WhenDonePC, %l2
			   /* 执行代码：
			   *	call StartupPC();
			   *	call InitialPC(InitialArg);
			   *	call WhenDonePC();
			   */
	call	StartupPC,0
	nop
	call	%l0, 1	
	mov	%l1, %o0   /* 使用延迟槽设置 InitialPC 的参数 */
	call	%l2, 0
	nop
			   /* WhenDonePC 调用永远不应返回。如果返回
			    * 我们将执行一个陷阱进入调试器。 */
	ta	ST_BREAKPOINT


.globl	_SWITCH
_SWITCH:
	save	%sp, -SA(MINFRAME), %sp
	st	%fp, [%i0]
	st	%i0, [%i0+I0]
	st	%i1, [%i0+I1]
	st	%i2, [%i0+I2]
	st	%i3, [%i0+I3]
	st	%i4, [%i0+I4]
	st	%i5, [%i0+I5]
	st	%i7, [%i0+I7]
	ta	ST_FLUSH_WINDOWS
	nop
	mov	%i1, %l0
	ld	[%l0+I0], %i0
	ld	[%l0+I1], %i1
	ld	[%l0+I2], %i2
	ld	[%l0+I3], %i3
	ld	[%l0+I4], %i4
	ld	[%l0+I5], %i5
	ld	[%l0+I7], %i7
	ld	[%l0], %i6
	ret
	restore

#endif HOST_SPARC

#ifdef HOST_SNAKE

    ;rp = r2,   sp = r30
    ;arg0 = r26,  arg1 = r25,  arg2 = r24,  arg3 = r23

	.SPACE  $TEXT$
	.SUBSPA $CODE$
ThreadRoot
	.PROC
	.CALLINFO CALLER,FRAME=0
	.ENTRY

	.CALL
	ble  0(%r6)		;调用 StartupPC
	or   %r31, 0, %rp	;将返回地址放入正确的寄存器
	or   %r4, 0, %arg0	;加载 InitialArg
	.CALL	;in=26
	ble  0(%r3)		;调用 InitialPC
	or   %r31, 0, %rp	;将返回地址放入正确的寄存器
	.CALL
	ble  0(%r5)		;调用 WhenDonePC
	.EXIT
	or   %r31, 0, %rp	;实际上不应该返回 - 不返回

	.PROCEND


SWITCH
	.PROC
	.CALLINFO CALLER,FRAME=0
	.ENTRY

    ; 保存旧线程的进程状态
	stw  %sp, SP(%arg0)	;保存堆栈指针
	stw  %r3, S0(%arg0)	;保存被调用者保存的寄存器
	stw  %r4, S1(%arg0)
	stw  %r5, S2(%arg0)
	stw  %r6, S3(%arg0)
	stw  %r7, S4(%arg0)
	stw  %r8, S5(%arg0)
	stw  %r9, S6(%arg0)
	stw  %r10, S7(%arg0)
	stw  %r11, S8(%arg0)
	stw  %r12, S9(%arg0)
	stw  %r13, S10(%arg0)
	stw  %r14, S11(%arg0)
	stw  %r15, S12(%arg0)
	stw  %r16, S13(%arg0)
	stw  %r17, S14(%arg0)
	stw  %r18, S15(%arg0)
	stw  %rp, PC(%arg0)	;保存程序计数器

    ; 恢复下一个线程的进程状态
	ldw  SP(%arg1), %sp	;恢复堆栈指针
	ldw  S0(%arg1), %r3	;恢复被调用者保存的寄存器
	ldw  S1(%arg1), %r4
	ldw  S2(%arg1), %r5
	ldw  S3(%arg1), %r6
	ldw  S4(%arg1), %r7
	ldw  S5(%arg1), %r8
	ldw  S6(%arg1), %r9
	ldw  S7(%arg1), %r10
	ldw  S8(%arg1), %r11
	ldw  S9(%arg1), %r12
	ldw  S10(%arg1), %r13
	ldw  S11(%arg1), %r14
	ldw  S12(%arg1), %r15
	ldw  S13(%arg1), %r16
	ldw  S14(%arg1), %r17
	ldw  PC(%arg1), %rp	;保存程序计数器
	bv   0(%rp)
	.EXIT
	ldw  S15(%arg1), %r18

	.PROCEND

	.EXPORT SWITCH,ENTRY,PRIV_LEV=3,RTNVAL=GR
	.EXPORT ThreadRoot,ENTRY,PRIV_LEV=3,RTNVAL=GR

#endif

#ifdef HOST_i386

        .text
        .align  2

        .globl  _ThreadRoot

/* void ThreadRoot( void )
**
** 期望以下寄存器被初始化：
**      eax     指向启动函数（中断使能）
**      edx     包含线程函数的初始参数
**      esi     指向线程函数
**      edi     指向 Thread::Finish()
*/
_ThreadRoot:
        pushl   %ebp
        movl    %esp,%ebp
        pushl   InitialArg
        call    StartupPC
        call    InitialPC
        call    WhenDonePC

        # 不会到达
        movl    %ebp,%esp
        popl    %ebp
        ret



/* void SWITCH( thread *t1, thread *t2 )
**
** 进入时，堆栈看起来像这样：
**      8(esp)  ->              thread *t2
**      4(esp)  ->              thread *t1
**       (esp)  ->              返回地址
**
** 我们将当前的 eax 推送到堆栈，以便我们可以将其用作
** 指向 t1 的指针，这会将 esp 减少 4，因此当我们使用它
** 来引用堆栈上的内容时，我们将 4 加到偏移量。
*/
        .comm   _eax_save,4

        .globl  _SWITCH
_SWITCH:
        movl    %eax,_eax_save          # 保存 eax 的值
        movl    4(%esp),%eax            # 将指向 t1 的指针移动到 eax
        movl    %ebx,_EBX(%eax)         # 保存寄存器
        movl    %ecx,_ECX(%eax)
        movl    %edx,_EDX(%eax)
        movl    %esi,_ESI(%eax)
        movl    %edi,_EDI(%eax)
        movl    %ebp,_EBP(%eax)
        movl    %esp,_ESP(%eax)         # 保存堆栈指针
        movl    _eax_save,%ebx          # 获取保存的 eax 值
        movl    %ebx,_EAX(%eax)         # 存储它
        movl    0(%esp),%ebx            # 从堆栈中获取返回地址到 ebx
        movl    %ebx,_PC(%eax)          # 将其保存到 pc 存储中

        movl    8(%esp),%eax            # 将指向 t2 的指针移动到 eax

        movl    _EAX(%eax),%ebx         # 获取新值到 eax 中
        movl    %ebx,_eax_save          # 保存它
        movl    _EBX(%eax),%ebx         # 恢复旧寄存器
        movl    _ECX(%eax),%ecx
        movl    _EDX(%eax),%edx
        movl    _ESI(%eax),%esi
        movl    _EDI(%eax),%edi
        movl    _EBP(%eax),%ebp
        movl    _ESP(%eax),%esp         # 恢复堆栈指针
        movl    _PC(%eax),%eax          # 将返回地址恢复到 eax
        movl    %eax,4(%esp)            # 将返回地址复制到堆栈上
        movl    _eax_save,%eax

        ret

#endif // HOST_i386

// Roberto Rossi (roberto@csr.unibo.it) - 1994
#ifdef HOST_ALPHA

	.set noreorder
	.set volatile
	.set noat

	.text
	.align 3

	.globl ThreadRoot
	.ent ThreadRoot
ThreadRoot:
	ldgp gp,0(pv)
	.frame sp,0,ra,0
	.prologue 1

	mov StartupPC,pv
	jsr ra,(pv),0		# 调用启动过程
	ldgp gp,0(ra)
	mov InitialArg,a0
	mov InitialPC,pv
	jsr ra,(pv),0		# 调用主过程
	ldgp gp,0(ra)
	mov WhenDonePC,pv
	jsr ra,(pv),0		# 当我们完成时，调用清理过程
	ldgp gp,0(ra)

	# 从未到达
	ret zero,(ra),1
	.end ThreadRoot

	# a0 -- 指向旧线程的指针
	# a1 -- 指向新线程的指针
	.globl SWITCH
	.ent SWITCH
SWITCH:
	ldgp gp,0(pv)
	.frame sp,0,ra,0
	.prologue 1

	stq sp,SP(a0)		# 保存新堆栈指针
	stq s0,S0(a0)		# 保存所有被调用者保存的寄存器
	stq s1,S1(a0)
	stq s2,S2(a0)
	stq s3,S3(a0)
	stq s4,S4(a0)
	stq s5,S5(a0)
	stq fp,FP(a0)		# 保存帧指针
	stq gp,GP(a0)		# 保存全局指针
	stq ra,PC(a0)		# 保存返回地址

	ldq sp,SP(a1)		# 加载新堆栈指针
	ldq s0,S0(a1)		# 加载被调用者保存的寄存器
	ldq s1,S1(a1)
	ldq s2,S2(a1)
	ldq s3,S3(a1)
	ldq s4,S4(a1)
	ldq s5,S5(a1)
	ldq fp,FP(a1)		# 加载帧指针
	ldq gp,GP(a1)		# 加载全局指针
	ldq ra,PC(a1)		# 加载返回地址

	jmp (ra)			# 执行上下文切换
	ldgp gp,0(ra)
	ret zero,(ra),1
	.end SWITCH

#endif // HOST_ALPHA
