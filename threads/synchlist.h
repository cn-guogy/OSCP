// synchlist.h
//	用于同步访问列表的数据结构。
//
//	通过用同步例程包围 List 抽象来实现。
//

#ifndef SYNCHLIST_H
#define SYNCHLIST_H

#include "list.h"
#include "synch.h"

// 以下类定义了一个“同步列表” -- 一个列表，其具有以下约束：
//	1. 尝试从列表中移除项目的线程将会
//	等待直到列表中有元素。
//	2. 一次只能有一个线程可以访问列表数据结构

class SynchList
{
public:
  SynchList();  // 初始化一个同步列表
  ~SynchList(); // 释放一个同步列表

  void Append(void *item); // 将项目附加到列表的末尾，
                           // 并唤醒任何在移除中等待的线程
  void *Remove(); // 从列表前面移除第一个项目，
                  // 如果列表为空则等待
                  // 对列表中的每个项目应用函数
  void Mapcar(VoidFunctionPtr func);

private:
  List *list;           // 未同步的列表
  Lock *lock;           // 强制对列表的互斥访问
  Condition *listEmpty; // 如果列表为空则在 Remove 中等待
};

#endif // SYNCHLIST_H
