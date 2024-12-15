// status.h 修改自 machine/stats.h
// 添加了numPageWrites
#ifndef STATS_H
#define STATS_H

class Statistics
{
public:
  int totalTicks;
  int idleTicks;
  int systemTicks;
  int userTicks;

  int numDiskReads;
  int numDiskWrites;
  int numConsoleCharsRead;
  int numConsoleCharsWritten;
  int numPageFaults;
  int numPageWrites; // 虚拟内存页面写入的数量
  int numPacketsSent;
  int numPacketsRecvd;

  Statistics();

  void Print();
};

#define UserTick 1
#define SystemTick 10
#define RotationTime 500
#define SeekTime 500
#define ConsoleTime 100
#define NetworkTime 100
#define TimerTicks 100

#endif // STATS_H
