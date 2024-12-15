// stats.h 
//	用于收集Nachos性能统计的数据结构。
//
// 请勿更改 -- 这些统计数据由机器仿真维护
//

#ifndef STATS_H
#define STATS_H

// 以下类定义了要保留的统计信息
// 关于Nachos的行为 -- 经过了多少时间（时钟周期），
// 执行了多少用户指令等。
//
// 该类中的字段是公共的，以便于更新。

class Statistics {
  public:
    int totalTicks;      	// 运行Nachos的总时间
    int idleTicks;       	// 空闲时间（没有线程可运行）
    int systemTicks;	 	// 执行系统代码的时间
    int userTicks;       	// 执行用户代码的时间
				// （这也等于用户指令执行的数量）

    int numDiskReads;		// 磁盘读取请求的数量
    int numDiskWrites;		// 磁盘写入请求的数量
    int numConsoleCharsRead;	// 从键盘读取的字符数量
    int numConsoleCharsWritten; // 写入显示的字符数量
    int numPageFaults;		// 虚拟内存页面错误的数量
    int numPacketsSent;		// 发送的网络数据包数量
    int numPacketsRecvd;	// 接收的网络数据包数量

    Statistics(); 		// 初始化所有值为零

    void Print();		// 打印收集的统计信息
};

// 用于反映操作在真实系统中所需的相对时间的常量。
// “时钟周期”只是一个时间单位 -- 如果你愿意，可以认为是微秒。
//
// 由于Nachos内核代码是直接执行的，内核中花费的时间
// 通过启用中断的调用次数来衡量，这些时间常量并不太精确。

#define UserTick 	1	// 每执行一条用户级指令时前进
#define SystemTick 	10 	// 每次启用中断时前进
#define RotationTime 	500 	// 磁盘旋转一个扇区所需的时间
#define SeekTime 	500    	// 磁盘寻道经过一个轨道所需的时间
#define ConsoleTime 	100	// 读取或写入一个字符所需的时间
#define NetworkTime 	100   	// 发送或接收一个数据包所需的时间
#define TimerTicks 	100    	// （平均）定时器中断之间的时间

#endif // STATS_H
