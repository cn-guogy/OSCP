// stats.cc 修改自 machine/stats.cc
// 增加了numPageWrites的初始化和输出

#include "utility.h"
#include "stats.h"

Statistics::Statistics()
{
    totalTicks = idleTicks = systemTicks = userTicks = 0;
    numDiskReads = numDiskWrites = 0;
    numConsoleCharsRead = numConsoleCharsWritten = 0;
    numPageFaults = numPacketsSent = numPacketsRecvd = 0;
    numPageWrites = 0; // 初始化为0
}

void Statistics::Print()
{
    printf("时钟周期: 总计 %d, 空闲 %d, 系统 %d, 用户 %d\n", totalTicks,
           idleTicks, systemTicks, userTicks);
    printf("磁盘I/O: 读取 %d, 写入 %d\n", numDiskReads, numDiskWrites);
    printf("控制台I/O: 读取 %d, 写入 %d\n", numConsoleCharsRead,
           numConsoleCharsWritten);
    printf("分页: 页面错误 %d,写回%d\n", numPageFaults, numPageWrites); // 输出写回
    printf("网络I/O: 接收的数据包 %d, 发送的数据包 %d\n", numPacketsRecvd,
           numPacketsSent);
}
