// sysdep.h
//	系统依赖接口。Nachos使用这里定义的例程，而不是直接调用UNIX库函数，以简化在不同版本的UNIX之间的移植，甚至是
//	其他系统，如MSDOS和Macintosh。
//

#ifndef SYSDEP_H
#define SYSDEP_H

// 检查文件以查看是否有字符可读。
// 如果文件中没有字符，则返回而不等待。
extern bool PollFile(int fd);

// 文件操作：打开/读取/写入/lseek/关闭，并检查错误
// 用于模拟磁盘和控制台设备。
extern int OpenForWrite(char *name);
extern int OpenForReadWrite(char *name, bool crashOnError);
extern void Read(int fd, char *buffer, int nBytes);
extern int ReadPartial(int fd, char *buffer, int nBytes);
extern void WriteFile(int fd, char *buffer, int nBytes);
extern void Lseek(int fd, int offset, int whence);
extern int Tell(int fd);
extern void Close(int fd);
// extern bool Unlink(char *name);
extern int Unlink(char *name);

// 进程间通信操作，用于模拟网络
extern int OpenSocket();
extern void CloseSocket(int sockID);
extern void AssignNameToSocket(char *socketName, int sockID);
extern void DeAssignNameToSocket(char *socketName);
extern bool PollSocket(int sockID);
extern void ReadFromSocket(int sockID, char *buffer, int packetSize);
extern void SendToSocket(int sockID, char *buffer, int packetSize, char *toName);

// 进程控制：中止、退出和延迟
extern void Abort();
extern void Exit(int exitCode);
extern void Delay(int seconds);

// 初始化系统，以便在用户按下ctl-C时调用cleanUp例程
extern void CallOnUserAbort(VoidNoArgFunctionPtr cleanUp);

// 初始化伪随机数生成器
extern void RandomInit(unsigned seed);
extern int Random();

// 分配、释放一个数组，使得在数组的任一端进行解引用都会导致错误
extern char *AllocBoundedArray(int size);
extern void DeallocBoundedArray(char *p, int size);

// 其他C库例程，Nachos使用这些例程。
// 这些被认为是可移植的，因此我们不包括包装器。
extern "C"
{
    int atoi(const char *str);
    double atof(const char *str);
    int abs(int i);

#include <stdio.h>  // 用于printf，fprintf
#include <string.h> // 用于DEBUG等
}

#endif // SYSDEP_H
