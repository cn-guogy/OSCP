// interrupt.h 修改自 lab6/interrupt.h
//  增加了页错误处理

#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "list.h"

enum IntStatus
{
  IntOff,
  IntOn
};

enum MachineStatus
{
  IdleMode,
  SystemMode,
  UserMode
};

enum IntType
{
  TimerInt,
  DiskInt,
  ConsoleWriteInt,
  ConsoleReadInt,
  NetworkSendInt,
  NetworkRecvInt
};

class PendingInterrupt
{
public:
  PendingInterrupt(VoidFunctionPtr func, _int param, int time, IntType kind);

  VoidFunctionPtr handler;
  _int arg;
  int when;
  IntType type;
};

class Interrupt
{
public:
  Interrupt();
  ~Interrupt();

  IntStatus SetLevel(IntStatus level);

  void Enable();
  IntStatus getLevel() { return level; }

  void Idle();

  void Halt();
  int Exec(char *name);
  void PrintInt(int num);
  void PageFault(int badAddress); // 页错误处理

  void YieldOnReturn();

  MachineStatus getStatus() { return status; }
  void setStatus(MachineStatus st) { status = st; }

  void DumpState();

  void Schedule(VoidFunctionPtr handler, _int arg,
                int fromnow, IntType type);

  void OneTick();

private:
  IntStatus level;
  List *pending;
  bool inHandler;
  bool yieldOnReturn;
  MachineStatus status;

  bool CheckIfDue(bool advanceClock);

  void ChangeLevel(IntStatus old,
                   IntStatus now);
};

#endif // INTERRRUPT_H
