// stats.h 
//	管理Nachos性能统计的例程。
//
// 请勿更改 -- 这些统计数据由机器仿真维护。
//



#include "utility.h"
#include "stats.h"

//----------------------------------------------------------------------
// Statistics::Statistics
// 	在系统启动时将性能指标初始化为零。
//----------------------------------------------------------------------

Statistics::Statistics()
{
    totalTicks = idleTicks = systemTicks = userTicks = 0;
    numDiskReads = numDiskWrites = 0;
    numConsoleCharsRead = numConsoleCharsWritten = 0;
    numPageFaults = numPacketsSent = numPacketsRecvd = 0;
}

//----------------------------------------------------------------------
// Statistics::Print
// 	在系统关闭时打印性能指标。
//----------------------------------------------------------------------

void
Statistics::Print()
{
    printf("时钟周期: 总计 %d, 空闲 %d, 系统 %d, 用户 %d\n", totalTicks, 
	idleTicks, systemTicks, userTicks);
    printf("磁盘I/O: 读取 %d, 写入 %d\n", numDiskReads, numDiskWrites);
    printf("控制台I/O: 读取 %d, 写入 %d\n", numConsoleCharsRead, 
	numConsoleCharsWritten);
    printf("分页: 页面错误 %d\n", numPageFaults);
    printf("网络I/O: 接收的数据包 %d, 发送的数据包 %d\n", numPacketsRecvd, 
	numPacketsSent);
}
