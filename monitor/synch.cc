// synch.cc
//	用于线程同步的例程。这里定义了三种类型的
//	同步例程：信号量、锁
//   	和条件变量（后两者的实现留给读者）。
//
// 任何同步例程的实现都需要一些
// 原始的原子操作。我们假设Nachos运行在
// 单处理器上，因此可以通过
// 关闭中断来提供原子性。在中断被禁用时，
// 不会发生上下文切换，因此当前线程被保证
// 持有CPU，直到中断被重新启用。
//
// 因为这些例程中的一些可能在中断
// 已经被禁用的情况下被调用（例如Semaphore::V），
// 所以我们在原子操作结束时，不是开启中断，
// 而是始终将中断状态重新设置为其原始值（无论
// 是禁用还是启用）。
//

#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	初始化一个信号量，以便可以用于同步。
//
//	"debugName"是一个任意名称，便于调试。
//	"initialValue"是信号量的初始值。
//----------------------------------------------------------------------

Semaphore::Semaphore(const char *debugName, int initialValue)
{
    name = (char *)debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::~Semaphore
// 	当不再需要信号量时，释放其占用的内存。假设没有人
//	仍在等待信号量！
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	等待直到信号量值 > 0，然后递减。检查信号量
//	值和递减必须原子性地完成，因此我们
//	需要在检查值之前禁用中断。
//
//	注意，Thread::Sleep假设在调用时中断是禁用的。
//----------------------------------------------------------------------

void Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff); // 禁用中断

    while (value == 0)
    {                                         // 信号量不可用
        queue->Append((void *)currentThread); // 所以去睡眠
        currentThread->Sleep();
    }
    value--; // 信号量可用，
             // 消耗其值

    (void)interrupt->SetLevel(oldLevel); // 重新启用中断
}

//----------------------------------------------------------------------
// Semaphore::V
// 	递增信号量值，如果必要则唤醒一个等待者。
//	与P()一样，这个操作必须是原子性的，因此我们需要禁用
//	中断。Scheduler::ReadyToRun()假设在调用时线程
//	是禁用的。
//----------------------------------------------------------------------

void Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL) // 使线程准备好，立即消耗V
        scheduler->ReadyToRun(thread);
    value++;
    (void)interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Lock::Lock
// 	初始化一个锁，以便可以用于同步。
//
//	"debugName"是一个任意名称，便于调试。
//----------------------------------------------------------------------

Lock::Lock(const char *debugName)
{
    name = (char *)debugName;
    owner = NULL;
    lock = new Semaphore(name, 1);
}

//----------------------------------------------------------------------
// Lock::~Lock
// 	当不再需要锁时，释放其占用的内存。与信号量一样，
//	假设没有人仍在等待锁。
//----------------------------------------------------------------------
Lock::~Lock()
{
    delete lock;
}

//----------------------------------------------------------------------
// Lock::Acquire
//      使用二进制信号量来实现锁。记录哪个
//      线程获取了锁，以确保只有同一个线程释放它。
//----------------------------------------------------------------------
void Lock::Acquire()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff); // 禁用中断

    lock->P();                           // 获取信号量
    owner = currentThread;               // 记录锁的新拥有者
    (void)interrupt->SetLevel(oldLevel); // 重新启用中断
}

//----------------------------------------------------------------------
// Lock::Release
//      将锁设置为可用（即释放信号量）。检查
//      当前线程是否被允许释放此锁。
//----------------------------------------------------------------------
void Lock::Release()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff); // 禁用中断

    // 确保：a) 锁是忙的  b) 这个线程是获取它的同一个线程。
    ASSERT(currentThread == owner);
    owner = NULL; // 清除拥有者
    lock->V();    // 释放信号量
    (void)interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Lock::isHeldByCurrentThread
//----------------------------------------------------------------------
bool Lock::isHeldByCurrentThread()
{
    bool result;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    result = currentThread == owner;
    (void)interrupt->SetLevel(oldLevel);
    return (result);
}

//----------------------------------------------------------------------
// Condition::Condition
// 	初始化一个条件变量，以便可以用于
//      同步。
//
//	"debugName"是一个任意名称，便于调试。
//----------------------------------------------------------------------
Condition::Condition(const char *debugName)
{
    name = (char *)debugName;
    queue = new List;
    lock = NULL;
}

//----------------------------------------------------------------------
// Condition::~Condition
// 	当不再需要条件变量时，释放其占用的内存。与信号量一样，
//      假设没有人仍在等待条件。
//----------------------------------------------------------------------

Condition::~Condition()
{
    delete queue;
}

//----------------------------------------------------------------------
// Condition::Wait
//
//      释放锁，放弃CPU直到被信号唤醒，然后
//      重新获取锁。
//
//      前置条件：当前线程持有锁；队列中的线程在
//      等待同一把锁。
//----------------------------------------------------------------------
void Condition::Wait(Lock *conditionLock)
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    ASSERT(conditionLock->isHeldByCurrentThread()); // 检查前置条件
    if (queue->IsEmpty())
    {
        lock = conditionLock; // 有助于强制执行前置条件
    }
    ASSERT(lock == conditionLock); // 另一个前置条件
    queue->Append(currentThread);  // 将此线程添加到等待列表
    conditionLock->Release();      // 释放锁
    currentThread->Sleep();        // 去睡眠
    conditionLock->Acquire();      // 醒来：重新获取锁
    (void)interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Condition::Signal
//      唤醒一个线程，如果有任何线程在等待条件。
//
//      前置条件：当前线程持有锁；队列中的线程在
//      等待同一把锁。
//----------------------------------------------------------------------
void Condition::Signal(Lock *conditionLock)
{
    Thread *nextThread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    ASSERT(conditionLock->isHeldByCurrentThread());
    if (!queue->IsEmpty())
    {
        ASSERT(lock == conditionLock);
        nextThread = (Thread *)queue->Remove();
        scheduler->ReadyToRun(nextThread); // 唤醒线程
    }
    (void)interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Condition::Broadcast
//      唤醒所有等待条件的线程。
//
//      前置条件：当前线程持有锁；队列中的线程在
//      等待同一把锁。
//----------------------------------------------------------------------
void Condition::Broadcast(Lock *conditionLock)
{
    Thread *nextThread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    ASSERT(conditionLock->isHeldByCurrentThread());
    if (!queue->IsEmpty())
    {
        ASSERT(lock == conditionLock);
        while ((nextThread = (Thread *)queue->Remove()))
        {
            scheduler->ReadyToRun(nextThread); // 唤醒线程
        }
    }
    (void)interrupt->SetLevel(oldLevel);
}

// Hoare风格的条件变量
Condition_H::Condition_H(const char *debugName)
{
    name = (char *)debugName;
    count = 0;
    sem = new Semaphore(name, 0); // 使用与条件变量相同的名称
}
Condition_H::~Condition_H()
{
    delete sem;
}

void Condition_H::Wait(Semaphore *mutex, Semaphore *next, int *next_countPtr)
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    count++;
    if (*next_countPtr > 0)
        next->V();
    else
        mutex->V();
    sem->P();
    count--;

    (void)interrupt->SetLevel(oldLevel);
}

void Condition_H::Signal(Semaphore *next, int *next_countPtr)
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    if (count > 0)
    {
        (*next_countPtr)++;
        sem->V();

        next->P();
        (*next_countPtr)--;
    }
    (void)interrupt->SetLevel(oldLevel);
}

void Condition_H::Broadcast(Semaphore *next, int *next_countPtr)
{
    // 尚未实现
}
