// lab2
// 修改自threads/thread.h
// 增加了私有成员priority提供了设置和获取优先级的方法

#ifndef THREAD_H
#define THREAD_H

#include "utility.h"

#ifdef USER_PROGRAM
#include "machine.h"
#include "addrspace.h"
#endif

#define MachineStateSize 18
#define StackSize (sizeof(_int) * 1024)

enum ThreadStatus
{
  JUST_CREATED,
  RUNNING,
  READY,
  BLOCKED
};

extern void ThreadPrint(_int arg);

class Thread
{
private:
  int *stackTop;
  _int machineState[MachineStateSize];
  int *stack;
  ThreadStatus status;
  char *name;
  int priority; // 0-99, 默认9

  void StackAllocate(VoidFunctionPtr func, _int arg);

public:
  Thread(const char *debugName, int priority = 9); // 缺省默认优先级9
  ~Thread();

  void Fork(VoidFunctionPtr func, _int arg);
  void Yield();
  void Sleep();
  void Finish();

  void CheckOverflow();
  void setStatus(ThreadStatus st) { status = st; }
  char *getName() { return (name); }
  void Print() { printf("%s, ", name); }
  void setPriority(int priority) { this->priority = priority; } // 设置优先级
  int getPriority(void) { return this->priority; }              // 获取优先级

#ifdef USER_PROGRAM
private:
  int userRegisters[NumTotalRegs];

public:
  void SaveUserState();
  void RestoreUserState();

  AddrSpace *space;
#endif
};

extern "C"
{
  void ThreadRoot();
  void SWITCH(Thread *oldThread, Thread *newThread);
}

#endif
