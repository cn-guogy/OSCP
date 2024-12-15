// timer.h 
//	用于模拟硬件定时器的数据结构。
//
//	硬件定时器每X毫秒生成一个CPU中断。
//	这意味着它可以用于实现时间片轮转，或者让
//	线程在特定时间段内进入睡眠状态。
//
//	我们通过调度中断来模拟硬件定时器，
//	每当stats->totalTicks增加了TimerTicks时发生中断。
//
//	为了在时间片轮转中引入一些随机性，如果“doRandom”
//	被设置为真，则中断将在随机数量的时钟周期后发生。
//
//  请勿更改 -- 机器仿真的一部分
//


#ifndef TIMER_H
#define TIMER_H


#include "utility.h"

// 以下类定义了一个硬件定时器。 
class Timer {
  public:
    Timer(VoidFunctionPtr timerHandler, _int callArg, bool doRandom);
				// 初始化定时器，以便在每个时间片调用
				// 中断处理程序“timerHandler”。
    ~Timer() {}

// 定时器仿真的内部例程 -- 请勿调用这些

    void TimerExpired();	// 当硬件定时器生成中断时内部调用

    int TimeOfNextInterrupt();  // 计算定时器将生成
				// 下一个中断的时间 

  private:
    bool randomize;		// 如果需要使用随机超时延迟则设置
    VoidFunctionPtr handler;	// 定时器中断处理程序 
    _int arg;			// 传递给中断处理程序的参数

};

#endif // TIMER_H
