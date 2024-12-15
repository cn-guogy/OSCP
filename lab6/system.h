// system.h 修改自 threads/system.h
// 增加了对bitmap的引用
#ifndef SYSTEM_H
#define SYSTEM_H

#include "utility.h"
#include "thread.h"
#include "scheduler.h"
#include "interrupt.h"
#include "stats.h"
#include "timer.h"

// 初始化和清理例程
extern void Initialize(int argc, char **argv); // 初始化，
											   // 在任何其他操作之前调用
extern void Cleanup();						   // 清理，当 Nachos 完成时调用

extern Thread *currentThread;		// 持有 CPU 的线程
extern Thread *threadToBeDestroyed; // 刚刚完成的线程
extern Scheduler *scheduler;		// 就绪队列
extern Interrupt *interrupt;		// 中断状态
extern Statistics *stats;			// 性能指标
extern Timer *timer;				// 硬件闹钟

#ifdef USER_PROGRAM
#include "machine.h"
#include "bitmap.h"
extern Machine *machine; // 用户程序内存和寄存器
extern BitMap *freePhys_Map;
extern int space;
#endif

#ifdef FILESYS_NEEDED // FILESYS 或 FILESYS_STUB
#include "filesys.h"
extern FileSystem *fileSystem;
#endif

#ifdef FILESYS
#include "synchdisk.h"
extern SynchDisk *synchDisk;
#endif

#ifdef NETWORK
#include "post.h"
extern PostOffice *postOffice;
#endif

#endif // SYSTEM_H
