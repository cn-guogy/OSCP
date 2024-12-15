// timer.cc
//	模拟硬件定时器设备的例程。
//
//      硬件定时器每X毫秒生成一个CPU中断。
//      这意味着它可以用于实现时间片轮转。
//
//      我们通过调度中断来模拟硬件定时器，
//      每当stats->totalTicks增加了TimerTicks时发生中断。
//
//      为了在时间片轮转中引入一些随机性，如果“doRandom”
//      被设置为真，则中断将在随机数量的时钟周期后发生。
//
//	请记住 -- 这里的内容不是Nachos的一部分。它只是
//	Nachos运行的硬件的模拟。
//
//  请勿更改 -- 机器仿真的一部分
//

#include "timer.h"
#include "system.h"

// 虚拟函数，因为C++不允许指向成员函数的指针
static void TimerHandler(_int arg)
{
    Timer *p = (Timer *)arg;
    p->TimerExpired();
}

//----------------------------------------------------------------------
// Timer::Timer
//      初始化硬件定时器设备。保存每次中断调用的位置，
//      然后安排定时器开始生成中断。
//
//      "timerHandler"是定时器设备的中断处理程序。
//		每当定时器到期时，它在中断被禁用的情况下被调用。
//      "callArg"是传递给中断处理程序的参数。
//      "doRandom" -- 如果为真，则安排中断在随机的时间间隔发生。
//----------------------------------------------------------------------

Timer::Timer(VoidFunctionPtr timerHandler, _int callArg, bool doRandom)
{
    randomize = doRandom;
    handler = timerHandler;
    arg = callArg;

    // 安排来自定时器设备的第一个中断
    interrupt->Schedule(TimerHandler, (_int)this, TimeOfNextInterrupt(),
                        TimerInt);
}

//----------------------------------------------------------------------
// Timer::TimerExpired
//      模拟硬件定时器设备生成的中断的例程。
//      安排下一个中断，并调用中断处理程序。
//----------------------------------------------------------------------

void Timer::TimerExpired()
{
    // 安排下一个定时器设备中断
    interrupt->Schedule(TimerHandler, (_int)this, TimeOfNextInterrupt(),
                        TimerInt);

    // 调用此设备的Nachos中断处理程序
    (*handler)(arg);
}

//----------------------------------------------------------------------
// Timer::TimeOfNextInterrupt
//      返回硬件定时器设备将下次引发中断的时间。
//      如果启用了随机化，则使其成为（伪）随机延迟。
//----------------------------------------------------------------------

int Timer::TimeOfNextInterrupt()
{
    if (randomize)
        return 1 + (Random() % (TimerTicks * 2));
    else
        return TimerTicks;
}
