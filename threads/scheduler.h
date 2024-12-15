// scheduler.h
//	线程调度器和调度程序的数据结构。
//	主要是准备运行的线程列表。
//
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "list.h"
#include "thread.h"

// 以下类定义了调度器/调度程序抽象 --
// 用于跟踪哪个线程正在运行，以及哪些线程准备好但未运行的数据结构和操作。

class Scheduler
{
public:
  Scheduler();  // 初始化准备线程列表
  ~Scheduler(); // 释放准备列表

  void ReadyToRun(Thread *thread); // 线程可以被调度。
  Thread *FindNextToRun();         // 从准备列表中出队第一个线程（如果有），并返回该线程。
  void Run(Thread *nextThread);    // 使 nextThread 开始运行
  void Print();                    // 打印准备列表的内容

private:
  List *readyList; // 准备运行但未运行的线程队列
};

#endif // SCHEDULER_H
