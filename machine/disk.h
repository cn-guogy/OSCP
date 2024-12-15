// disk.h
//	用于模拟物理磁盘的数据结构。物理磁盘
//	可以接受（一次）读取/写入磁盘扇区的请求；
//	当请求被满足时，CPU会收到一个中断，
//	然后可以向磁盘发送下一个请求。
//
//	磁盘内容在机器崩溃时会被保留，但如果
//	在系统关闭时正在进行文件系统操作（例如，创建文件），
//	文件系统可能会被损坏。
//
//  请勿更改 -- 机器仿真的一部分
//

#ifndef DISK_H
#define DISK_H

#include "utility.h"

// 以下类定义了一个物理磁盘I/O设备。磁盘
// 具有单个表面，分为“轨道”，每个轨道又分为
// “扇区”（每个轨道的扇区数量相同，每个
// 扇区具有相同数量的存储字节）。
//
// 地址是通过扇区号进行的 -- 磁盘上的每个扇区都有一个
// 唯一的编号：轨道 * 每轨扇区数 + 轨道内的偏移量。
//
// 与其他I/O设备一样，原始物理磁盘是一个异步设备 --
// 读取或写入磁盘部分的请求会立即返回，
// 并且稍后会调用中断以指示操作已完成。
//
// 物理磁盘实际上是通过对UNIX文件的操作来模拟的。
//
// 为了使生活更真实，
// 每个操作的模拟时间反映了“轨道缓冲区” -- RAM用于存储
// 当前轨道的内容，当磁盘头经过时。其想法是磁盘
// 始终将数据传输到轨道缓冲区，以防稍后请求该数据。
// 这有助于消除“跳过扇区”调度的需要 -- 在磁头刚刚
// 经过扇区开始时不久到来的读取请求可以更快地满足，
// 因为其内容在轨道缓冲区中。如今大多数磁盘都配备了轨道缓冲区。
//
// 通过使用 -DNOTRACKBUF 编译可以禁用轨道缓冲区模拟

#define SectorSize 128     // 每个磁盘扇区的字节数
#define SectorsPerTrack 32 // 每个磁盘轨道的扇区数
#define NumTracks 32       // 每个磁盘的轨道数
#define NumSectors (SectorsPerTrack * NumTracks)
// 磁盘的总扇区数

class Disk
{
public:
  Disk(const char *name, VoidFunctionPtr callWhenDone, _int callArg);
  // 创建一个模拟磁盘。
  // 每次请求完成时调用 (*callWhenDone)(callArg)
  ~Disk(); // 释放磁盘。

  void ReadRequest(int sectorNumber, char *data);
  // 读取/写入单个磁盘扇区。
  // 这些例程向磁盘发送请求并立即返回。
  // 仅允许一个请求同时进行！
  void WriteRequest(int sectorNumber, char *data);

  void HandleInterrupt(); // 中断处理程序，当
                          // 磁盘请求完成时调用。

  int ComputeLatency(int newSector, bool writing);
  // 返回对新扇区的请求将花费多长时间：
  // （寻道 + 旋转延迟 + 传输）

private:
  int fileno;              // 模拟磁盘的UNIX文件号
  VoidFunctionPtr handler; // 中断处理程序，当任何磁盘请求完成时调用
  _int handlerArg;         // 传递给中断处理程序的参数
  bool active;             // 磁盘操作是否正在进行？
  int lastSector;          // 上一个磁盘请求
  int bufferInit;          // 轨道缓冲区开始加载的时间

  int TimeToSeek(int newSector, int *rotate); // 到达新轨道的时间
  int ModuloDiff(int to, int from);           // 到和从之间的扇区数
  void UpdateLast(int newSector);
};

#endif // DISK_H
