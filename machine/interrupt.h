// interrupt.h 
//	数据结构用于模拟低级中断硬件。
//
//	硬件提供一个例程（SetLevel）来启用或禁用
//	中断。
//
//	为了模拟硬件，我们需要跟踪所有
//	硬件设备可能引发的中断，以及它们
//	何时应该发生。
//
//	该模块还跟踪模拟时间。时间仅在以下情况
//	下推进：
//		中断被重新启用
//		执行用户指令
//		就绪队列为空
//
//	因此，与真实硬件不同，中断（因此时间片
//	上下文切换）不能在启用中断的代码中的任何地方发生，
//	而只能在模拟时间推进的代码中的那些地方发生
//	（以便可以调用硬件模拟中的中断）。
//
//	注意：这意味着不正确同步的代码可能在这个硬件
//	模拟中正常工作（即使时间片随机化），
//	但在真实硬件上可能无法工作。 （仅仅因为我们不能
//	总是检测到您的程序在现实生活中何时会失败，并不
//	意味着编写不正确同步的代码是可以的！）
//
//  请勿更改 -- 机器仿真的一部分
//

#ifndef INTERRUPT_H
#define INTERRUPT_H


#include "list.h"

// 中断可以被禁用（IntOff）或启用（IntOn）
enum IntStatus { IntOff, IntOn };

// Nachos可以运行内核代码（SystemMode），用户代码（UserMode），
// 或者没有可运行线程，因为就绪列表
// 是空的（IdleMode）。
enum MachineStatus {IdleMode, SystemMode, UserMode};

// IntType记录哪个硬件设备生成了中断。
// 在Nachos中，我们支持一个硬件定时器设备，一个磁盘，一个控制台
// 显示器和键盘，以及一个网络。
enum IntType { TimerInt, DiskInt, ConsoleWriteInt, ConsoleReadInt, 
				NetworkSendInt, NetworkRecvInt};

// 以下类定义了一个计划在未来发生的中断。
// 内部数据结构保持公共，以便更简单地操作。

class PendingInterrupt {
  public:
    PendingInterrupt(VoidFunctionPtr func, _int param, int time, IntType kind);
				// 初始化一个将在未来发生的中断

    VoidFunctionPtr handler;    // 当中断发生时调用的函数
    _int arg;           // 函数的参数。
    int when;			// 中断应该触发的时间
    IntType type;		// 用于调试
};

// 以下类定义了硬件中断模拟的数据结构。
// 我们记录中断是否被启用或禁用，以及任何计划在未来发生的硬件中断。

class Interrupt {
  public:
    Interrupt();			// 初始化中断模拟
    ~Interrupt();			// 释放数据结构
    
    IntStatus SetLevel(IntStatus level);// 禁用或启用中断 
					// 并返回先前的设置。

    void Enable();			// 启用中断。
    IntStatus getLevel() {return level;}// 返回中断是否
					// 被启用或禁用
    
    void Idle(); 			// 就绪队列为空，推进 
					// 模拟时间直到下一个中断

    void Halt(); 			// 退出并打印统计信息
    
    void YieldOnReturn();		// 在从中断处理程序返回时引发上下文切换

    MachineStatus getStatus() { return status; } // 空闲，内核，用户
    void setStatus(MachineStatus st) { status = st; }

    void DumpState();			// 打印中断状态
    

    // 注意：以下内容是硬件模拟代码的内部内容。
    // 请勿直接调用这些。 我应该将它们设为“私有”，
    // 但由于它们被硬件设备模拟器调用，因此需要公开。

    void Schedule(VoidFunctionPtr handler, _int arg, // 安排一个中断发生
	int fromnow, IntType type); // “fromNow”是中断将在未来（在模拟时间中）发生的时间。
	                    // 这是由硬件设备模拟器调用的。
    
    void OneTick();       		// 推进模拟时间

  private:
    IntStatus level;		// 中断是否被启用或禁用？
    List *pending;		// 计划在未来发生的中断列表
    bool inHandler;		// 如果我们正在运行中断处理程序则为TRUE
    bool yieldOnReturn; 	// 如果我们在从中断处理程序返回时要进行上下文切换则为TRUE
    MachineStatus status;	// 空闲，内核模式，用户模式

    // 这些函数是中断模拟代码的内部内容

    bool CheckIfDue(bool advanceClock); // 检查中断是否应该
					// 现在发生

    void ChangeLevel(IntStatus old, 	// SetLevel，不推进
	IntStatus now);  		// 模拟时间
};

#endif // INTERRRUPT_H
