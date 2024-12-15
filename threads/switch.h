/* switch.h
 *	实现上下文切换所需的定义。
 *
 *	上下文切换本质上是机器相关的，因为
 *	要保存的寄存器、如何设置初始
 *	调用帧等，都是特定于处理器架构的。
 *
 * 	此文件当前支持 DEC MIPS、SUN SPARC、HP PA-RISC、
 *  Intel 386 和 DEC ALPHA 架构。
 */

#ifndef SWITCH_H
#define SWITCH_H

#ifdef HOST_MIPS

/* 在上下文切换期间必须保存的寄存器。
 * 这些是从 Thread 对象开始的偏移量，
 * 以字节为单位，用于 switch.s
 */
#define SP 0
#define S0 4
#define S1 8
#define S2 12
#define S3 16
#define S4 20
#define S5 24
#define S6 28
#define S7 32
#define FP 36
#define PC 40

/* 为了分叉一个线程，我们设置它的保存寄存器状态，以便
 * 当我们切换到该线程时，它将开始在 ThreadRoot 中运行。
 *
 * 以下是我们需要设置的初始寄存器，以
 * 将值传递给 ThreadRoot（例如，包含要运行的线程的过程）。
 * 第一组是 ThreadRoot 使用的寄存器；第二组是这些初始
 * 值在 Thread 对象中的位置 -- 用于 Thread::AllocateStack()。
 */

#define InitialPC s0
#define InitialArg s1
#define WhenDonePC s2
#define StartupPC s3

#define PCState (PC / 4 - 1)
#define FPState (FP / 4 - 1)
#define InitialPCState (S0 / 4 - 1)
#define InitialArgState (S1 / 4 - 1)
#define WhenDonePCState (S2 / 4 - 1)
#define StartupPCState (S3 / 4 - 1)

#endif // HOST_MIPS

#ifdef HOST_SPARC

/* 在上下文切换期间必须保存的寄存器。 见上面的注释。 */
#define I0 4
#define I1 8
#define I2 12
#define I3 16
#define I4 20
#define I5 24
#define I6 28
#define I7 32

/* 用于清理代码的别名。  */
#define FP I6
#define PC I7

/* ThreadRoot 的寄存器。 见上面的注释。 */
#define InitialPC % o0
#define InitialArg % o1
#define WhenDonePC % o2
#define StartupPC % o3

#define PCState (PC / 4 - 1)
#define InitialPCState (I0 / 4 - 1)
#define InitialArgState (I1 / 4 - 1)
#define WhenDonePCState (I2 / 4 - 1)
#define StartupPCState (I3 / 4 - 1)
#endif // HOST_SPARC

#ifdef HOST_SNAKE

/* 在上下文切换期间必须保存的寄存器。 见上面的注释。 */
#define SP 0
#define S0 4
#define S1 8
#define S2 12
#define S3 16
#define S4 20
#define S5 24
#define S6 28
#define S7 32
#define S8 36
#define S9 40
#define S10 44
#define S11 48
#define S12 52
#define S13 56
#define S14 60
#define S15 64
#define PC 68

/* ThreadRoot 的寄存器。 见上面的注释。 */
#define InitialPC % r3 /* S0 */
#define InitialArg % r4
#define WhenDonePC % r5
#define StartupPC % r6

#define PCState (PC / 4 - 1)
#define InitialPCState (S0 / 4 - 1)
#define InitialArgState (S1 / 4 - 1)
#define WhenDonePCState (S2 / 4 - 1)
#define StartupPCState (S3 / 4 - 1)
#endif // HOST_SNAKE

#ifdef HOST_i386

/* 寄存器从线程对象开始的偏移量 */
#define _ESP 0
#define _EAX 4
#define _EBX 8
#define _ECX 12
#define _EDX 16
#define _EBP 20
#define _ESI 24
#define _EDI 28
#define _PC 32

/* 这些定义用于 Thread::AllocateStack()。 */
#define PCState (_PC / 4 - 1)
#define FPState (_EBP / 4 - 1)
#define InitialPCState (_ESI / 4 - 1)
#define InitialArgState (_EDX / 4 - 1)
#define WhenDonePCState (_EDI / 4 - 1)
#define StartupPCState (_ECX / 4 - 1)

#define InitialPC % esi
#define InitialArg % edx
#define WhenDonePC % edi
#define StartupPC % ecx
#endif // HOST_i386

// Roberto Rossi (roberto@csr.unibo.it) - 1994
#ifdef HOST_ALPHA

#include <regdef.h>

/* 在上下文切换期间必须保存的寄存器。
 * 这些是从 Thread 对象开始的偏移量，
 * 以字节为单位，用于 switch.s
 */
#define SP 0
#define S0 8
#define S1 16
#define S2 24
#define S3 32
#define S4 40
#define S5 48
#define FP 56
#define GP 64
#define PC 72

/* 为了分叉一个线程，我们设置它的保存寄存器状态，以便
 * 当我们切换到该线程时，它将开始在 ThreadRoot 中运行。
 *
 * 以下是我们需要设置的初始寄存器，以
 * 将值传递给 ThreadRoot（例如，包含要运行的线程的过程）。
 * 第一组是 ThreadRoot 使用的寄存器；第二组是这些初始
 * 值在 Thread 对象中的位置 -- 用于 Thread::AllocateStack()。
 */
#define InitialPC s0
#define InitialArg s1
#define WhenDonePC s2
#define StartupPC s3

#define PCState (PC / 8 - 1)
#define FPState (FP / 8 - 1)
#define InitialPCState (S0 / 8 - 1)
#define InitialArgState (S1 / 8 - 1)
#define WhenDonePCState (S2 / 8 - 1)
#define StartupPCState (S3 / 8 - 1)
#endif // HOST_ALPHA

#endif // SWITCH_H
