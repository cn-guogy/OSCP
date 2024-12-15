/* Start.s 
 *	汇编语言辅助用户程序在Nachos上运行。
 *
 *	由于我们不想引入整个C库，我们在这里定义
 *	用户程序所需的内容，即Start和系统调用。
 */

#define IN_ASM
#include "syscall.h"

        .text   
        .align  2

/* -------------------------------------------------------------
 * __start
 *	通过调用“main”初始化运行C程序。 
 *
 * 	注意：这必须是第一个，以便它被加载到位置0。
 *	Nachos内核总是通过跳转到位置0来启动程序。
 * -------------------------------------------------------------
 */

	.globl __start
	.ent	__start
__start:
	jal	main
	move	$4,$0		
	jal	Exit	 /* 如果我们从main返回，退出(0) */
	.end __start

/* -------------------------------------------------------------
 * 系统调用存根：
 *	汇编语言辅助以进行系统调用到Nachos内核。
 *	每个系统调用都有一个存根，将系统调用的代码放入寄存器r2中，
 *	并保持系统调用的参数不变（换句话说，arg1在r4中，arg2在
 *	r5中，arg3在r6中，arg4在r7中）
 *
 * 	返回值在r2中。这遵循MIPS上的标准C调用约定。
 * -------------------------------------------------------------
 */

	.globl Halt
	.ent	Halt
Halt:
	addiu $2,$0,SC_Halt
	syscall
	j	$31
	.end Halt

	.globl Exit
	.ent	Exit
Exit:
	addiu $2,$0,SC_Exit
	syscall
	j	$31
	.end Exit

	.globl Exec
	.ent	Exec
Exec:
	addiu $2,$0,SC_Exec
	syscall
	j	$31
	.end Exec

	.globl Join
	.ent	Join
Join:
	addiu $2,$0,SC_Join
	syscall
	j	$31
	.end Join

	.globl Create
	.ent	Create
Create:
	addiu $2,$0,SC_Create
	syscall
	j	$31
	.end Create

	.globl Open
	.ent	Open
Open:
	addiu $2,$0,SC_Open
	syscall
	j	$31
	.end Open

	.globl Read
	.ent	Read
Read:
	addiu $2,$0,SC_Read
	syscall
	j	$31
	.end Read

	.globl Write
	.ent	Write
Write:
	addiu $2,$0,SC_Write
	syscall
	j	$31
	.end Write

	.globl Close
	.ent	Close
Close:
	addiu $2,$0,SC_Close
	syscall
	j	$31
	.end Close

	.globl Fork
	.ent	Fork
Fork:
	addiu $2,$0,SC_Fork
	syscall
	j	$31
	.end Fork

	.globl Yield
	.ent	Yield
Yield:
	addiu $2,$0,SC_Yield
	syscall
	j	$31
	.end Yield

	.globl PrintInt
	.ent	PrintInt
PrintInt:
	addiu $2,$0,SC_PrintInt
	syscall
	j	$31
	.end PrintInt

/* 虚拟函数以保持gcc的正常运行 */
        .globl  __main
        .ent    __main
__main:
        j       $31
        .end    __main
