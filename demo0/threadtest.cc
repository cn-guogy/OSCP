// threadtest.cc
//	简单的线程测试案例。
//
//	创建两个线程，并通过调用 Thread::Yield 进行上下文切换，
//	以展示线程系统的内部工作原理。
//

#include "system.h"

//----------------------------------------------------------------------
// SimpleThread
// 	循环 5 次，每次将 CPU 让给另一个就绪线程
//	"which" 只是一个标识线程的数字，用于调试。
//----------------------------------------------------------------------

void SimpleThread(_int which)
{
    int num;

    for (num = 0; num < 5; num++)
    {
        printf("*** 线程 %d 循环了 %d 次\n", (int)which, num);
        currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// ThreadTest
// 	通过分叉一个线程来设置两个线程之间的乒乓切换，
//	调用 SimpleThread，然后自己调用 SimpleThread。
//----------------------------------------------------------------------

void ThreadTest()
{
    DEBUG('t', "进入简单测试");

    Thread *ta = new Thread("分叉线程 a");
    Thread *tb = new Thread("分叉线程 b");
    ta->Println();
    tb->Println();

    // 3 个线程，运行顺序：0, 1, 2, 0, 1, 2...
    ta->Fork(SimpleThread, 1);
    tb->Fork(SimpleThread, 2);
    SimpleThread(0);
    /*
        // 3 个线程，运行顺序：2, 1, 0, 2, 1, 0...
        ta->Fork(SimpleThread, 1);
        tb->Fork(SimpleThread, 0);
        SimpleThread(2);
    */
}
