// synch.h
//	用于线程同步的数据结构。
//
//	这里定义了三种同步方式：信号量、锁和条件变量。
//	信号量的实现已给出；后两者仅提供了过程
//	接口——它们将在第一次作业中实现。
//
//	请注意，所有同步对象在初始化时都需要一个“名称”。
//	这仅用于调试目的。

#ifndef SYNCH_H
#define SYNCH_H

#include "thread.h"
#include "list.h"

// 以下类定义了一个“信号量”，其值为非负整数。
// 信号量只有两个操作 P() 和 V():
//
//	P() -- 等待直到值 > 0，然后递减
//
//	V() -- 增加，如果有线程在 P() 中等待则唤醒它
//
// 请注意，接口*不*允许线程直接读取信号量的值——
// 即使你读取了值，你所知道的也只是值曾经是什么。
// 你不知道现在的值是什么，因为在你将值
// 放入寄存器时，可能发生了上下文切换，
// 其他线程可能已经调用了 P 或 V，因此真实值可能
// 现在不同。

class Semaphore
{
public:
  Semaphore(const char *debugName, int initialValue); // 设置初始值
  ~Semaphore();                                       // 释放信号量
  char *getName() { return name; }                    // 调试辅助

  void P(); // 这些是信号量的唯一操作
  void V(); // 它们都是*原子*操作

private:
  char *name;  // 用于调试
  int value;   // 信号量值，始终 >= 0
  List *queue; // 在 P() 中等待值 > 0 的线程
};

// 以下类定义了一个“锁”。锁可以是忙碌或空闲。
// 锁上仅允许两种操作：
//
//	Acquire -- 等待直到锁为空闲，然后将其设置为忙碌
//
//	Release -- 将锁设置为空闲，唤醒在 Acquire 中等待的线程（如果有必要）
//
// 此外，按照惯例，只有获取锁的线程可以释放它。
// 与信号量一样，你不能读取锁的值
// （因为值可能在你读取后立即改变）。

class Lock
{
public:
  Lock(const char *debugName);     // 初始化锁为免费
  ~Lock();                         // 释放锁
  char *getName() { return name; } // 调试辅助

  void Acquire(); // 这些是锁的唯一操作
  void Release(); // 它们都是*原子*操作

  bool isHeldByCurrentThread(); // 如果当前线程
                                // 持有此锁，则为真。用于
                                // 在 Release 和下面的
                                // 条件变量操作中检查。

private:
  char *name;      // 用于调试
  Thread *owner;   // 记住谁获取了锁
  Semaphore *lock; // 使用信号量作为实际锁
};

// 以下类定义了一个“条件变量”。条件变量没有值，
// 但线程可以排队，等待该变量。条件变量的操作只有：
//
//	Wait() -- 释放锁，放弃 CPU 直到被信号唤醒，
//		然后重新获取锁
//
//	Signal() -- 唤醒一个线程，如果有线程在等待
//		该条件
//
//	Broadcast() -- 唤醒所有在该条件上等待的线程
//
// 所有对条件变量的操作必须在当前线程获取锁时进行。
// 实际上，对给定条件变量的所有访问必须由同一把锁保护。
// 换句话说，必须在调用条件变量操作的线程之间强制互斥。
//
// 在 Nachos 中，条件变量被假定遵循 *Mesa* 风格的
// 语义。当 Signal 或 Broadcast 唤醒另一个线程时，
// 它只是将线程放入就绪列表，唤醒线程重新获取锁的责任
// 在 Wait() 中处理。相比之下，有些定义条件变量
// 遵循 *Hoare* 风格的语义——在这种情况下，信号线程
// 放弃对锁和 CPU 的控制，唤醒线程立即运行，并在
// 唤醒线程离开临界区时将控制权交还给信号线程。
//
// 使用 Mesa 风格语义的结果是，其他线程可以获取锁，
// 并在唤醒线程有机会运行之前更改数据结构。

class Condition
{
public:
  Condition(const char *debugName); // 初始化条件为
                                    // “没有人等待”
  ~Condition();                     // 释放条件
  char *getName() { return (name); }

  void Wait(Lock *conditionLock);      // 这些是条件变量的 3 个操作；
                                       // 释放锁并进入睡眠是
                                       // 在 Wait() 中*原子*的
  void Signal(Lock *conditionLock);    // conditionLock 必须由
  void Broadcast(Lock *conditionLock); // 当前线程持有
                                       // 所有这些操作

private:
  char *name;
  List *queue; // 等待条件的线程
  Lock *lock;  // 调试辅助：用于检查
               // Wait、Signal 和 Broadcast
               // 的参数正确性
};

// 在这里，条件变量使用 Hoare 风格实现。
// 我们使用信号量来实现条件变量。算法在教科书第 195 页中给出。
// -ptang (1995年8月)

class Condition_H
{
public:
  Condition_H(const char *debugName); // 初始化条件为
                                      // “没有人等待”
  ~Condition_H();                     // 释放条件
  char *getName() { return (name); }

  void Wait(Semaphore *mutex, Semaphore *next, int *next_countPtr);
  // mutex 和 next 是两个
  // 监视器环的信号量，
  // next_count 是信号量 next 的计数器。
  void Signal(Semaphore *next, int *next_countPtr);
  void Broadcast(Semaphore *next, int *next_countPtr);

private:
  char *name;
  // 以及你需要定义的其他一些内容

  Semaphore *sem; // 等待线程的信号量；
  int count;      // 等待线程的数量；
};

#endif // SYNCH_H
