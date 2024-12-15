// lab2
// 修改自thread/thread.cc
// 默认优先级9

#include "thread.h"
#include "switch.h"
#include "synch.h"
#include "system.h"

#define STACK_FENCEPOST 0xdeadbeef

Thread::Thread(const char *threadName, int priority)
{
    this->priority = priority;
    name = (char *)threadName;
    stackTop = NULL;
    stack = NULL;
    status = JUST_CREATED;
#ifdef USER_PROGRAM
    space = NULL;
#endif
}

Thread::~Thread()
{
    DEBUG('t', "正在删除线程 \"%s\"\n", name);

    ASSERT(this != currentThread);
    if (stack != NULL)
        DeallocBoundedArray((char *)stack, StackSize * sizeof(_int));
}

void Thread::Fork(VoidFunctionPtr func, _int arg)
{
#ifdef HOST_ALPHA
    DEBUG('t', "正在创建线程 \"%s\"，函数地址 = 0x%lx, 参数 = %ld\n",
          name, (long)func, arg);
#else
    DEBUG('t', "正在创建线程 \"%s\"，函数地址 = 0x%x, 参数 = %d\n",
          name, (int)func, arg);
#endif

    StackAllocate(func, arg);

    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    scheduler->ReadyToRun(this);
    (void)interrupt->SetLevel(oldLevel);
}

void Thread::CheckOverflow()
{
    if (stack != NULL)
#ifdef HOST_SNAKE
        ASSERT((unsigned int)stack[StackSize - 1] == STACK_FENCEPOST);
#else
        ASSERT((unsigned int)*stack == STACK_FENCEPOST);
#endif
}

void Thread::Finish()
{
    (void)interrupt->SetLevel(IntOff);
    ASSERT(this == currentThread);

    DEBUG('t', "正在结束线程 \"%s\"\n", getName());

    threadToBeDestroyed = currentThread;
    Sleep();
}

void Thread::Yield()
{
    Thread *nextThread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    ASSERT(this == currentThread);

    DEBUG('t', "正在让出线程 \"%s\"\n", getName());

    nextThread = scheduler->FindNextToRun();
    if (nextThread != NULL)
    {
        scheduler->ReadyToRun(this);
        scheduler->Run(nextThread);
    }
    (void)interrupt->SetLevel(oldLevel);
}

void Thread::Sleep()
{
    Thread *nextThread;

    ASSERT(this == currentThread);
    ASSERT(interrupt->getLevel() == IntOff);

    DEBUG('t', "线程 \"%s\" 正在休眠\n", getName());

    status = BLOCKED;
    while ((nextThread = scheduler->FindNextToRun()) == NULL)
        interrupt->Idle();

    scheduler->Run(nextThread);
}

static void ThreadFinish() { currentThread->Finish(); }
static void InterruptEnable() { interrupt->Enable(); }
void ThreadPrint(_int arg)
{
    Thread *t = (Thread *)arg;
    t->Print();
}

void Thread::StackAllocate(VoidFunctionPtr func, _int arg)
{
    stack = (int *)AllocBoundedArray(StackSize * sizeof(_int));

#ifdef HOST_SNAKE
    stackTop = stack + 16;
    stack[StackSize - 1] = STACK_FENCEPOST;
#else
#ifdef HOST_SPARC
    stackTop = stack + StackSize - 96;
#else
    stackTop = stack + StackSize - 4;
#ifdef HOST_i386
#endif
#endif
    *stack = STACK_FENCEPOST;
#endif

    machineState[PCState] = (_int)ThreadRoot;
    machineState[StartupPCState] = (_int)InterruptEnable;
    machineState[InitialPCState] = (_int)func;
    machineState[InitialArgState] = arg;
    machineState[WhenDonePCState] = (_int)ThreadFinish;
}

#ifdef USER_PROGRAM
#include "machine.h"

void Thread::SaveUserState()
{
    for (int i = 0; i < NumTotalRegs; i++)
        userRegisters[i] = machine->ReadRegister(i);
}

void Thread::RestoreUserState()
{
    for (int i = 0; i < NumTotalRegs; i++)
        machine->WriteRegister(i, userRegisters[i]);
}
#endif
