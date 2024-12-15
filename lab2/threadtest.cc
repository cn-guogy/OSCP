// lab2
// 修改自threads/threadtest.cc
// 修改了测试内容

#include "system.h"

void SimpleThread(_int which)
{
    int num;

    for (num = 0; num < 5; num++)
    {
        printf("*** 线程 %d 循环了 %d 次，优先级=%d\n", (int)which, num, currentThread->getPriority());
        if (num != 4)
            currentThread->Yield();
    }
}

void ThreadTest()
{
    DEBUG('t', "进入简单测试");

    Thread *t1 = new Thread("线程 1");
    t1->setPriority(1);
    Thread *t2 = new Thread("线程 2");
    t2->setPriority(2);
    Thread *t3 = new Thread("线程 3");
    t3->setPriority(3);

    t1->Fork(SimpleThread, 1);
    t2->Fork(SimpleThread, 2);
    t3->Fork(SimpleThread, 3);
    SimpleThread(0);
}
