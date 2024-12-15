// threadtest.cc 
//	简单的线程作业测试案例。
//
//	创建两个线程，并通过调用 Thread::Yield 使它们在彼此之间进行上下文切换，
//	以说明线程系统的内部工作原理。

#include "system.h"

//----------------------------------------------------------------------
// SimpleThread
// 	循环 5 次，每次将 CPU 让给另一个就绪线程 
//	每次迭代。
//
//	"which" 只是一个标识线程的数字，用于调试
//	目的。
//----------------------------------------------------------------------

void
SimpleThread(_int which)
{
    int num;
    
    for (num = 0; num < 5; num++) {
	printf("*** 线程 %d 循环了 %d 次\n", (int) which, num);
        currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// ThreadTest
// 	通过分叉一个线程来调用 SimpleThread，
//	然后自己调用 SimpleThread，设置两个线程之间的乒乓。
//
//----------------------------------------------------------------------

void
ThreadTest()
{
    DEBUG('t', "进入 SimpleTest");

    Thread *t = new Thread("分叉线程");

    t->Fork(SimpleThread, 1);
    SimpleThread(0);
}
