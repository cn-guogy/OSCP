// synchdisk.h 
// 	导出原始磁盘设备的同步接口的数据结构.
//



#ifndef SYNCHDISK_H
#define SYNCHDISK_H

#include "disk.h"
#include "synch.h"

// 以下类定义了一个“同步”磁盘抽象。
// 与其他I/O设备一样，原始物理磁盘是一个异步设备 --
// 读取或写入磁盘部分的请求立即返回，
// 并且稍后会发生中断以信号操作完成。
// （此外，磁盘设备的物理特性假设
// 只能请求一个操作）。
//
// 该类提供了抽象，对于任何单独的线程
// 发出请求时，它会等待操作完成后再返回。
class SynchDisk {
  public:
    SynchDisk(const char* name);    		// 初始化一个同步磁盘，
					// 通过初始化原始磁盘。
    ~SynchDisk();			// 释放同步磁盘数据
    
    void ReadSector(int sectorNumber, char* data);
    					// 读取/写入磁盘扇区，返回
    					// 仅在数据实际被读取 
					// 或写入后。这些调用
    					// Disk::ReadRequest/WriteRequest 并
					// 然后等待请求完成。
    void WriteSector(int sectorNumber, char* data);
    
    void RequestDone();			// 由磁盘设备中断
					// 处理程序调用，以信号
					// 当前磁盘操作已完成。

  private:
    Disk *disk;		  		// 原始磁盘设备
    Semaphore *semaphore; 		// 用于同步请求线程 
					// 与中断处理程序
    Lock *lock;		  		// 只能向磁盘发送一个读/写请求
					// 一次
};

#endif // SYNCHDISK_H
