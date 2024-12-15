// thread.h
//	管理线程的数据结构。线程表示
//	程序中代码的顺序执行。
//	因此，线程的状态包括程序计数器、
//	处理器寄存器和执行栈。
//
// 	请注意，由于我们为每个线程分配固定大小的栈，
//	因此可能会导致栈溢出——例如，
//	通过递归到过深的级别。导致这种情况的最常见原因
//	是在栈上分配大型数据结构。
//	例如，这将导致问题：
//
//		void foo() { int buf[1000]; ...}
//
//	相反，您应该动态分配所有数据结构：
//
//		void foo() { int *buf = new int[1000]; ...}
//
//
// 	如果您溢出栈，会发生不好的事情，
//	在最坏的情况下，问题可能不会被明确捕获。
//	相反，唯一的症状可能是奇怪的段错误。
//	（当然，其他问题也可能导致段错误，
//	因此这并不是您线程栈太小的确切迹象。）
//
//	如果您发现自己遇到段错误，可以尝试的一个方法是
//	增加线程栈的大小——ThreadStackSize。
//
//  	在此接口中，分叉线程需要两个步骤。
//	我们必须首先为其分配一个数据结构：“t = new Thread”。
//	只有这样我们才能进行分叉：“t->fork(f, arg)”。

#ifndef THREAD_H
#define THREAD_H

#include "utility.h"

#ifdef USER_PROGRAM
#include "machine.h"
#include "addrspace.h"
#endif

// CPU寄存器状态在上下文切换时保存。
// SPARC和MIPS只需要10个寄存器，但Snake需要18个。
// 为了简单起见，这只是所有架构中的最大值。
#define MachineStateSize 18

// 线程私有执行栈的大小。
// 如果这不够大，请小心!!!!!
#define StackSize (sizeof(_int) * 1024) // 以字为单位

// 线程状态
enum ThreadStatus
{
  JUST_CREATED,
  RUNNING,
  READY,
  BLOCKED
};

// 外部函数，虚拟例程，其唯一工作是调用Thread::Print
extern void ThreadPrint(_int arg);

// 以下类定义了“线程控制块”——
// 代表单个执行线程。
//
//  每个线程都有：
//     用于激活记录的执行栈（“stackTop”和“stack”）
//     在不运行时保存CPU寄存器的空间（“machineState”）
//     一个“状态”（运行/就绪/阻塞）
//
//  一些线程也属于用户地址空间；
//  仅在内核中运行的线程具有NULL地址空间。

class Thread
{
private:
  // 注意：请勿更改这两个成员的顺序。
  // 它们必须处于此位置以使SWITCH正常工作。
  int *stackTop;                       // 当前栈指针
  _int machineState[MachineStateSize]; // 除stackTop外的所有寄存器

public:
  Thread(const char *debugName); // 初始化一个线程
  ~Thread();                     // 释放一个线程
                                 // 注意——被删除的线程
                                 // 在调用删除时不能正在运行

  // 基本线程操作

  void Fork(VoidFunctionPtr func, _int arg); // 使线程运行 (*func)(arg)
  void Yield();                              // 如果有其他线程可运行，则放弃CPU
  void Sleep();                              // 将线程置于睡眠状态并
                                             // 放弃处理器
  void Finish();                             // 线程执行完毕

  void CheckOverflow(); // 检查线程是否
                        // 溢出其栈
  void setStatus(ThreadStatus st) { status = st; }
  char *getName() { return (name); }
  void Print() { printf("%s, ", name); }

private:
  // 此类的一些私有数据在上面列出

  int *stack;          // 栈的底部
                       // 如果这是主线程则为NULL
                       // （如果为NULL，则不要释放栈）
  ThreadStatus status; // 就绪、运行或阻塞
  char *name;

  void StackAllocate(VoidFunctionPtr func, _int arg);
  // 为线程分配栈。
  // 由Fork()内部使用

#ifdef USER_PROGRAM
  // 运行用户程序的线程实际上有*两*组CPU寄存器——
  // 一组用于执行用户代码时的状态，
  // 一组用于执行内核代码时的状态。

  int userRegisters[NumTotalRegs]; // 用户级CPU寄存器状态

public:
  void SaveUserState();    // 保存用户级寄存器状态
  void RestoreUserState(); // 恢复用户级寄存器状态

  AddrSpace *space; // 此线程正在运行的用户代码。
#endif
};

// 魔法的机器相关例程，在switch.s中定义

extern "C"
{
  // 线程执行栈上的第一个帧；
  //    启用中断
  //	调用“func”
  //	（当func返回时，如果有的话）调用ThreadFinish()
  void ThreadRoot();

  // 停止运行oldThread并开始运行newThread
  void SWITCH(Thread *oldThread, Thread *newThread);
}

#endif // THREAD_H
