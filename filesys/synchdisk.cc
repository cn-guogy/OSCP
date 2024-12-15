// synchdisk.cc 
//	用于同步访问磁盘的例程。物理磁盘 
//	是一个异步设备（磁盘请求立即返回，
//	中断稍后发生）。这是在磁盘之上提供的一个层，
//	提供同步接口（请求等待直到请求完成）。
//
//	使用信号量来同步中断处理程序与
//	待处理请求。并且，由于物理磁盘只能
//	同时处理一个操作，因此使用锁来强制互斥。
//


#include "synchdisk.h"

//----------------------------------------------------------------------
// DiskRequestDone
// 	磁盘中断处理程序。需要将其作为C例程，因为 
//	C++无法处理成员函数的指针。
//----------------------------------------------------------------------

static void
DiskRequestDone (_int arg)
{
    SynchDisk* dsk = (SynchDisk *)arg;	// 磁盘 -> dsk

    dsk->RequestDone();					// 磁盘 -> dsk
}

//----------------------------------------------------------------------
// SynchDisk::SynchDisk
// 	初始化与物理磁盘的同步接口，
//	并依次初始化物理磁盘。
//
//	"name" -- 用作磁盘数据存储的UNIX文件名
//	   （通常为 "DISK"）
//----------------------------------------------------------------------

SynchDisk::SynchDisk(const char* name)
{
    semaphore = new Semaphore("同步磁盘", 0);
    lock = new Lock("同步磁盘锁");
    disk = new Disk(name, DiskRequestDone, (_int) this);
}

//----------------------------------------------------------------------
// SynchDisk::~SynchDisk
// 	释放同步磁盘抽象所需的数据结构。
//----------------------------------------------------------------------

SynchDisk::~SynchDisk()
{
    delete disk;
    delete lock;
    delete semaphore;
}

//----------------------------------------------------------------------
// SynchDisk::ReadSector
// 	将磁盘扇区的内容读取到缓冲区中。只有在数据被读取后才返回。
//
//	"sectorNumber" -- 要读取的磁盘扇区
//	"data" -- 用于保存磁盘扇区内容的缓冲区
//----------------------------------------------------------------------

void
SynchDisk::ReadSector(int sectorNumber, char* data)
{
    lock->Acquire();			// 每次只能进行一个磁盘I/O
    disk->ReadRequest(sectorNumber, data);
    semaphore->P();			// 等待中断
    lock->Release();
}

//----------------------------------------------------------------------
// SynchDisk::WriteSector
// 	将缓冲区的内容写入磁盘扇区。只有在数据被写入后才返回。
//
//	"sectorNumber" -- 要写入的磁盘扇区
//	"data" -- 磁盘扇区的新内容
//----------------------------------------------------------------------

void
SynchDisk::WriteSector(int sectorNumber, char* data)
{
    lock->Acquire();			// 每次只能进行一个磁盘I/O
    disk->WriteRequest(sectorNumber, data);
    semaphore->P();			// 等待中断
    lock->Release();
}

//----------------------------------------------------------------------
// SynchDisk::RequestDone
// 	磁盘中断处理程序。唤醒任何等待磁盘
//	请求完成的线程。
//----------------------------------------------------------------------

void
SynchDisk::RequestDone()
{ 
    semaphore->V();
}
