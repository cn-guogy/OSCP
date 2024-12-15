// lab2
// 修改自threads/scheduler.cc
// 按照优先级顺序插入

#include "scheduler.h"
#include "system.h"

Scheduler::Scheduler()
{
    readyList = new List;
}

Scheduler::~Scheduler()
{
    delete readyList;
}

void Scheduler::ReadyToRun(Thread *thread)
{
    DEBUG('t', "将线程 %s 放入就绪列表。\n", thread->getName());

    thread->setStatus(READY);
    
    readyList->SortedInsert((void *)thread, thread->getPriority()); // 按照优先级排序
}

Thread *
Scheduler::FindNextToRun()
{
    // for (int i = 0; i < readyList->NumElements(); i++)
    // {
    //     thread = (Thread *)readyList->ListItems()[i];
    //     priority = thread->getPriority();
    //     thread->setPriority(priority - 1);
    // }

    return (Thread *)readyList->Remove();
}

void Scheduler::Run(Thread *nextThread)
{
    Thread *oldThread = currentThread;

#ifdef USER_PROGRAM
    if (currentThread->space != NULL)
    {
        currentThread->SaveUserState();
        currentThread->space->SaveState();
    }
#endif

    oldThread->CheckOverflow();

    currentThread = nextThread;
    currentThread->setStatus(RUNNING);

    DEBUG('t', "正在从线程 \"%s\" 切换到线程 \"%s\"\n",
          oldThread->getName(), nextThread->getName());

    SWITCH(oldThread, nextThread);

    DEBUG('t', "现在在线程 \"%s\"\n", currentThread->getName());

    if (threadToBeDestroyed != NULL)
    {
        delete threadToBeDestroyed;
        threadToBeDestroyed = NULL;
    }

#ifdef USER_PROGRAM
    if (currentThread->space != NULL)
    {
        currentThread->RestoreUserState();
        currentThread->space->RestoreState();
    }
#endif
}

void Scheduler::Print()
{
    printf("就绪列表内容:\n");
    readyList->Mapcar((VoidFunctionPtr)ThreadPrint);
}
