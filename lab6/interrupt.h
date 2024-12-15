// interrupt.h  修改自 threads/interrupt.h
// 添加了exec方法
// 添加了printInt方法

#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "list.h"

enum IntStatus
{
  IntOff,
  IntOn
};

enum MachineStatus
{
  IdleMode,
  SystemMode,
  UserMode
};

enum IntType
{
  TimerInt,
  DiskInt,
  ConsoleWriteInt,
  ConsoleReadInt,
  NetworkSendInt,
  NetworkRecvInt
};

class PendingInterrupt
{
public:
  PendingInterrupt(VoidFunctionPtr func, _int param, int time, IntType kind);
  // 初始化一个将在未来发生的中断

  VoidFunctionPtr handler; // 当中断发生时调用的函数
  _int arg;                // 函数的参数。
  int when;                // 中断应该触发的时间
  IntType type;            // 用于调试
};

// 以下类定义了硬件中断模拟的数据结构。
// 我们记录中断是否被启用或禁用，以及任何计划在未来发生的硬件中断。

class Interrupt
{
public:
  Interrupt();  // 初始化中断模拟
  ~Interrupt(); // 释放数据结构

  IntStatus SetLevel(IntStatus level); // 禁用或启用中断
                                       // 并返回先前的设置。

  void Enable();                         // 启用中断。
  IntStatus getLevel() { return level; } // 返回中断是否
                                         // 被启用或禁用

  void Idle(); // 就绪队列为空，推进
               // 模拟时间直到下一个中断

  void Halt();            // 退出并打印统计信息
  int Exec(char *name);   // 执行用户程序
  void PrintInt(int num); // 打印整数

  void YieldOnReturn(); // 在从中断处理程序返回时引发上下文切换

  MachineStatus getStatus() { return status; } // 空闲，内核，用户
  void setStatus(MachineStatus st) { status = st; }

  void DumpState(); // 打印中断状态

  // 注意：以下内容是硬件模拟代码的内部内容。
  // 请勿直接调用这些。 我应该将它们设为“私有”，
  // 但由于它们被硬件设备模拟器调用，因此需要公开。

  void Schedule(VoidFunctionPtr handler, _int arg, // 安排一个中断发生
                int fromnow, IntType type);        // “fromNow”是中断将在未来（在模拟时间中）发生的时间。
                                                   // 这是由硬件设备模拟器调用的。

  void OneTick(); // 推进模拟时间

private:
  IntStatus level;      // 中断是否被启用或禁用？
  List *pending;        // 计划在未来发生的中断列表
  bool inHandler;       // 如果我们正在运行中断处理程序则为TRUE
  bool yieldOnReturn;   // 如果我们在从中断处理程序返回时要进行上下文切换则为TRUE
  MachineStatus status; // 空闲，内核模式，用户模式

  // 这些函数是中断模拟代码的内部内容

  bool CheckIfDue(bool advanceClock); // 检查中断是否应该
                                      // 现在发生

  void ChangeLevel(IntStatus old,  // SetLevel，不推进
                   IntStatus now); // 模拟时间
};

#endif // INTERRRUPT_H
