// disk.cc
//	用于模拟物理磁盘设备的例程；对磁盘的读写
//	被模拟为对UNIX文件的读写。
//	有关磁盘行为的详细信息，请参见disk.h（因此也包括此模拟的行为）。
//
//	磁盘操作是异步的，因此我们必须在模拟操作完成时
//	调用中断处理程序。
//
//  请勿更改 -- 机器仿真的一部分
//

#include "disk.h"
#include "system.h"

// 我们将其放在表示磁盘的UNIX文件的前面，
// 以减少意外将有用文件视为磁盘的可能性
// （这可能会破坏文件的内容）。
#define MagicNumber 0x456789ab
#define MagicSize sizeof(int)

#define DiskSize (MagicSize + (NumSectors * SectorSize))

// 虚拟过程，因为我们不能获取成员函数的指针
static void DiskDone(_int arg) { ((Disk *)arg)->HandleInterrupt(); }

//----------------------------------------------------------------------
// Disk::Disk()
// 	初始化一个模拟磁盘。打开UNIX文件（如果不存在则创建），
// 	并检查魔术数字以确保可以将其视为Nachos磁盘存储。
//
//	"name" -- 模拟Nachos磁盘的文件的文本名称
//	"callWhenDone" -- 磁盘读/写请求完成时调用的中断处理程序
//	"callArg" -- 传递给中断处理程序的参数
//----------------------------------------------------------------------

Disk::Disk(const char *name, VoidFunctionPtr callWhenDone, _int callArg)
{
    int magicNum;
    int tmp = 0;

    DEBUG('d', "正在初始化磁盘, 0x%x 0x%x\n", callWhenDone, callArg);
    handler = callWhenDone;
    handlerArg = callArg;
    lastSector = 0;
    bufferInit = 0;

    fileno = OpenForReadWrite((char *)name, FALSE);
    if (fileno >= 0)
    { // 文件存在，检查魔术数字
        Read(fileno, (char *)&magicNum, MagicSize);
        ASSERT(magicNum == MagicNumber);
    }
    else
    { // 文件不存在，创建它
        fileno = OpenForWrite((char *)name);
        magicNum = MagicNumber;
        WriteFile(fileno, (char *)&magicNum, MagicSize); // 写入魔术数字

        // 需要在文件末尾写入，以便读取不会返回EOF
        Lseek(fileno, DiskSize - sizeof(int), 0);
        WriteFile(fileno, (char *)&tmp, sizeof(int));
    }
    active = FALSE;
}

//----------------------------------------------------------------------
// Disk::~Disk()
// 	通过关闭表示磁盘的UNIX文件来清理磁盘模拟。
//----------------------------------------------------------------------

Disk::~Disk()
{
    Close(fileno);
}

//----------------------------------------------------------------------
// Disk::PrintSector()
// 	转储磁盘读/写请求中的数据，用于调试。
//----------------------------------------------------------------------

static void
PrintSector(bool writing, int sector, char *data)
{
    int *p = (int *)data;

    if (writing)
        printf("正在写入扇区: %d\n", sector);
    else
        printf("正在读取扇区: %d\n", sector);
    for (unsigned int i = 0; i < (SectorSize / sizeof(int)); i++)
        printf("%x ", p[i]);
    printf("\n");
}

//----------------------------------------------------------------------
// Disk::ReadRequest/WriteRequest
// 	模拟对单个磁盘扇区的读/写请求
//	   立即对UNIX文件进行读/写
//	   设置一个中断处理程序，以便稍后调用，
//	      当模拟器表示操作已完成时通知调用者。
//
//	注意，磁盘只允许读取/写入整个扇区，
//	而不是部分扇区。
//
//	"sectorNumber" -- 要读/写的磁盘扇区
//	"data" -- 要写入的字节，保存传入字节的缓冲区
//----------------------------------------------------------------------

void Disk::ReadRequest(int sectorNumber, char *data)
{
    int ticks = ComputeLatency(sectorNumber, FALSE);

    ASSERT(!active); // 一次只能有一个请求
    ASSERT((sectorNumber >= 0) && (sectorNumber < NumSectors));

    DEBUG('d', "从扇区 %d 读取\n", sectorNumber);
    Lseek(fileno, SectorSize * sectorNumber + MagicSize, 0);
    Read(fileno, data, SectorSize);
    if (DebugIsEnabled('d'))
        PrintSector(FALSE, sectorNumber, data);

    active = TRUE;
    UpdateLast(sectorNumber);
    stats->numDiskReads++;
    interrupt->Schedule(DiskDone, (_int)this, ticks, DiskInt);
}

void Disk::WriteRequest(int sectorNumber, char *data)
{
    int ticks = ComputeLatency(sectorNumber, TRUE);

    ASSERT(!active);
    ASSERT((sectorNumber >= 0) && (sectorNumber < NumSectors));

    DEBUG('d', "写入扇区 %d\n", sectorNumber);
    Lseek(fileno, SectorSize * sectorNumber + MagicSize, 0);
    WriteFile(fileno, data, SectorSize);
    if (DebugIsEnabled('d'))
        PrintSector(TRUE, sectorNumber, data);

    active = TRUE;
    UpdateLast(sectorNumber);
    stats->numDiskWrites++;
    interrupt->Schedule(DiskDone, (_int)this, ticks, DiskInt);
}

//----------------------------------------------------------------------
// Disk::HandleInterrupt()
// 	当是时候调用磁盘中断处理程序时，
//	告诉Nachos内核磁盘请求已完成。
//----------------------------------------------------------------------

void Disk::HandleInterrupt()
{
    active = FALSE;
    (*handler)(handlerArg);
}

//----------------------------------------------------------------------
// Disk::TimeToSeek()
//	返回将磁盘头定位到正确轨道所需的时间。
//	由于当我们完成寻道时，可能正好在旋转经过头部的扇区中，
//	我们还返回头部到下一个扇区边界的时间。
//
//   	磁盘以每个SeekTime时钟周期寻道一个轨道（参见stats.h）
//   	并以每个RotationTime时钟周期旋转一个扇区
//----------------------------------------------------------------------

int Disk::TimeToSeek(int newSector, int *rotation)
{
    int newTrack = newSector / SectorsPerTrack;
    int oldTrack = lastSector / SectorsPerTrack;
    int seek = abs(newTrack - oldTrack) * SeekTime;
    // 寻道需要多长时间？
    int over = (stats->totalTicks + seek) % RotationTime;
    // 当我们完成寻道时，是否会在扇区中间？

    *rotation = 0;
    if (over > 0) // 如果是，则需要向上舍入到下一个完整扇区
        *rotation = RotationTime - over;
    return seek;
}

//----------------------------------------------------------------------
// Disk::ModuloDiff()
// 	返回目标扇区"to"和当前扇区位置"from"之间的旋转延迟扇区数
//----------------------------------------------------------------------

int Disk::ModuloDiff(int to, int from)
{
    int toOffset = to % SectorsPerTrack;
    int fromOffset = from % SectorsPerTrack;

    return ((toOffset - fromOffset) + SectorsPerTrack) % SectorsPerTrack;
}

//----------------------------------------------------------------------
// Disk::ComputeLatency()
// 	返回从当前磁盘头位置读取/写入磁盘扇区所需的时间。
//
//   	延迟 = 寻道时间 + 旋转延迟 + 传输时间
//   	磁盘以每个SeekTime时钟周期寻道一个轨道（参见stats.h）
//   	并以每个RotationTime时钟周期旋转一个扇区
//
//   	要找到旋转延迟，我们首先必须弄清楚在寻道后（如果有的话）
//   	磁盘头将位于何处。然后我们计算在此之后完全旋转到newSector所需的时间。
//
//   	磁盘还有一个“轨道缓冲区”；磁盘不断将当前磁盘轨道的内容读取到缓冲区中。
//   	这使得对当前轨道的读取请求能够更快地满足。
//   	轨道缓冲区的内容在每次寻道到新轨道后被丢弃。
//----------------------------------------------------------------------

int Disk::ComputeLatency(int newSector, bool writing)
{
    int rotation;
    int seek = TimeToSeek(newSector, &rotation);
    int timeAfter = stats->totalTicks + seek + rotation;

#ifndef NOTRACKBUF // 如果您不想要轨道缓冲区的内容，请打开此选项
    // 检查轨道缓冲区是否适用
    if ((writing == FALSE) && (seek == 0) && (((timeAfter - bufferInit) / RotationTime) > ModuloDiff(newSector, bufferInit / RotationTime)))
    {
        DEBUG('d', "请求延迟 = %d\n", RotationTime);
        return RotationTime; // 从轨道缓冲区传输扇区的时间
    }
#endif

    rotation += ModuloDiff(newSector, timeAfter / RotationTime) * RotationTime;

    DEBUG('d', "请求延迟 = %d\n", seek + rotation + RotationTime);
    return (seek + rotation + RotationTime);
}

//----------------------------------------------------------------------
// Disk::UpdateLast
//   	跟踪最近请求的扇区。以便我们可以知道
//	轨道缓冲区中有什么。
//----------------------------------------------------------------------

void Disk::UpdateLast(int newSector)
{
    int rotate;
    int seek = TimeToSeek(newSector, &rotate);

    if (seek != 0)
        bufferInit = stats->totalTicks + seek + rotate;
    lastSector = newSector;
    DEBUG('d', "更新最后一个扇区 = %d, %d\n", lastSector, bufferInit);
}
