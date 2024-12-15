// sysdep.cc
//	实现系统依赖接口。Nachos使用这里定义的
//	例程，而不是直接调用UNIX库，
//	以简化在不同版本的UNIX之间的移植，甚至到
//	其他系统，如MSDOS。
//
//	在UNIX上，几乎所有这些例程都是对
//	底层UNIX系统调用的简单封装。
//
//	注意：所有这些例程都涉及对底层
//	主机机器（例如，DECstation，SPARC等）的操作，支持
//	Nachos模拟代码。Nachos实现类似的操作，
//	（例如打开文件），但这些是基于
//	硬件设备实现的，这些设备通过对底层
//	主机工作站操作系统中的例程的调用进行模拟。
//
//	这个文件包含大量对C例程的调用。C++要求
//	我们用“extern "C"块”包装所有C定义。
// 	这防止了C++编译器改变名称的内部形式。
//

#include <unistd.h>
extern "C"
{
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <sys/errno.h>
#ifdef HOST_i386
#include <sys/time.h>
#endif
#ifdef HOST_SPARC
#include <sys/time.h>
#endif
#ifdef HOST_ALPHA
#include <sys/time.h>
#endif

    // UNIX例程由本文件中的过程调用

#ifdef HOST_SNAKE
// int creat(char *name, unsigned short mode);
// int open(const char *name, int flags, ...);
#else
#ifndef HOST_ALPHA
#ifndef HOST_LINUX
    int creat(const char *name, unsigned short mode);
    int open(const char *name, int flags, ...);
#endif
#endif
// void signal(int sig, VoidFunctionPtr func); -- 这现在可能有效！
#if defined(HOST_i386) || defined(HOST_ALPHA)
    int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
               struct timeval *timeout);
#else
    int select(int numBits, void *readFds, void *writeFds, void *exceptFds,
               struct timeval *timeout);
#endif
#endif

    // int unlink(char *name);
    // int read(int filedes, char *buf, int numBytes);
    // int write(int filedes, char *buf, int numBytes);
    // int lseek(int filedes, int offset, int whence);
    // int tell(int filedes);
    // int close(int filedes);
    // int unlink(char *name);

    // 定义在平台之间略有不同，因此不要
    // 定义，除非gcc抱怨
    // extern int recvfrom(int s, void *buf, int len, int flags, void *from, int *fromlen);
    // extern int sendto(int s, void *msg, int len, int flags, void *to, int tolen);

    void srand(unsigned seed);
    int rand(void);
    unsigned sleep(unsigned);
    void abort();
    void exit(int);
    int getpagesize();

#ifndef HOST_ALPHA
#ifndef HOST_LINUX
    int mprotect(char *addr, int len, int prot);

    int socket(int, int, int);
    int bind(int, const void *, int);
    int recvfrom(int, void *, int, int, void *, int *);
    int sendto(int, const void *, int, int, void *, int);
#endif
#endif
}

#include "interrupt.h"
#include "system.h"

//----------------------------------------------------------------------
// PollFile
// 	检查打开的文件或打开的套接字，看看是否有任何
//	字符可以立即读取。如果有，读取它们
//	并返回TRUE。
//
//	在网络情况下，如果没有线程可以运行，
//	并且没有字符可以读取，
//	我们需要给另一方一个机会来获取我们主机的CPU
//	（否则，我们会非常缓慢，因为UNIX时间片
//	不频繁，这就像忙等待）。因此，我们
//	延迟一段固定的短时间，然后允许自己
//	重新调度（有点像Yield，但以UNIX的术语表示）。
//
//	"fd" -- 要轮询的文件描述符
//----------------------------------------------------------------------

bool PollFile(int fd)
{
    int rfd = (1 << fd), wfd = 0, xfd = 0, retVal;
    struct timeval pollTime;

    // 决定如果文件上没有字符要等待多长时间
    pollTime.tv_sec = 0;
    if (interrupt->getStatus() == IdleMode)
        pollTime.tv_usec = 20000; // 延迟以让其他nachos运行
    else
        pollTime.tv_usec = 0; // 没有延迟

// 轮询文件或套接字
#if defined(HOST_i386) || defined(HOST_ALPHA)
    retVal = select(32, (fd_set *)&rfd, (fd_set *)&wfd, (fd_set *)&xfd, &pollTime);
#else
    retVal = select(32, &rfd, &wfd, &xfd, &pollTime);
#endif

    ASSERT((retVal == 0) || (retVal == 1));
    if (retVal == 0)
        return FALSE; // 没有字符等待被读取
    return TRUE;
}

//----------------------------------------------------------------------
// OpenForWrite
// 	打开一个文件以进行写入。如果文件不存在，则创建它；如果文件已经存在，则截断它。
//	返回文件描述符。
//
//	"name" -- 文件名
//----------------------------------------------------------------------

int OpenForWrite(char *name)
{
    int fd = open(name, O_RDWR | O_CREAT | O_TRUNC, 0666);

    ASSERT(fd >= 0);
    return fd;
}

//----------------------------------------------------------------------
// OpenForReadWrite
// 	打开一个文件以进行读取或写入。
//	返回文件描述符，如果文件不存在则返回错误。
//
//	"name" -- 文件名
//----------------------------------------------------------------------

int OpenForReadWrite(char *name, bool crashOnError)
{
    int fd = open(name, O_RDWR, 0);

    ASSERT(!crashOnError || fd >= 0);
    return fd;
}

//----------------------------------------------------------------------
// Read
// 	从打开的文件中读取字符。如果读取失败则中止。
//----------------------------------------------------------------------

void Read(int fd, char *buffer, int nBytes)
{
    int retVal = read(fd, buffer, nBytes);
    ASSERT(retVal == nBytes);
}

//----------------------------------------------------------------------
// ReadPartial
// 	从打开的文件中读取字符，返回尽可能多的字符。
//----------------------------------------------------------------------

int ReadPartial(int fd, char *buffer, int nBytes)
{
    return read(fd, buffer, nBytes);
}

//----------------------------------------------------------------------
// WriteFile
// 	向打开的文件写入字符。如果写入失败则中止。
//----------------------------------------------------------------------

void WriteFile(int fd, char *buffer, int nBytes)
{
    int retVal = write(fd, buffer, nBytes);
    ASSERT(retVal == nBytes);
}

//----------------------------------------------------------------------
// Lseek
// 	更改打开文件中的位置。如果出错则中止。
//----------------------------------------------------------------------

void Lseek(int fd, int offset, int whence)
{
    int retVal = lseek(fd, offset, whence);
    ASSERT(retVal >= 0);
}

//----------------------------------------------------------------------
// Tell
// 	报告打开文件中的当前位置。
//----------------------------------------------------------------------

int Tell(int fd)
{
#ifdef HOST_i386
    return lseek(fd, 0, SEEK_CUR); // 386BSD没有tell()系统调用
#else
    return tell(fd);
#endif
}

//----------------------------------------------------------------------
// Close
// 	关闭一个文件。如果出错则中止。
//----------------------------------------------------------------------

void Close(int fd)
{
    int retVal = close(fd);
    ASSERT(retVal >= 0);
}

//----------------------------------------------------------------------
// Unlink
// 	删除一个文件。
//----------------------------------------------------------------------

// bool
int Unlink(char *name)
{
    return (bool)unlink(name);
}

//----------------------------------------------------------------------
// OpenSocket
// 	打开一个进程间通信（IPC）连接。现在，
//	只打开一个数据报端口，其他Nachos（模拟
//	网络上的工作站）可以向这个Nachos发送消息。
//----------------------------------------------------------------------

int OpenSocket()
{
    int sockID;

    sockID = socket(AF_UNIX, SOCK_DGRAM, 0);
    ASSERT(sockID >= 0);

    return sockID;
}

//----------------------------------------------------------------------
// CloseSocket
// 	关闭IPC连接。
//----------------------------------------------------------------------

void CloseSocket(int sockID)
{
    (void)close(sockID);
}

//----------------------------------------------------------------------
// InitSocketName
// 	初始化UNIX套接字地址 -- 神奇的！
//----------------------------------------------------------------------

static void
InitSocketName(struct sockaddr_un *uname, char *name)
{
    uname->sun_family = AF_UNIX;
    strcpy(uname->sun_path, name);
}

//----------------------------------------------------------------------
// AssignNameToSocket
// 	给IPC端口一个UNIX文件名，以便其他Nachos实例
//	可以找到该端口。
//----------------------------------------------------------------------

void AssignNameToSocket(char *socketName, int sockID)
{
    struct sockaddr_un uName;
    int retVal;

    (void)unlink(socketName); // 以防它仍然存在于上次

    InitSocketName(&uName, socketName);
    retVal = bind(sockID, (struct sockaddr *)&uName, sizeof(uName));
    ASSERT(retVal >= 0);
    DEBUG('n', "创建套接字 %s\n", socketName);
}

//----------------------------------------------------------------------
// DeAssignNameToSocket
// 	删除我们分配给IPC端口的UNIX文件名，以便清理。
//----------------------------------------------------------------------
void DeAssignNameToSocket(char *socketName)
{
    (void)unlink(socketName);
}

//----------------------------------------------------------------------
// PollSocket
// 	如果有任何消息等待到达IPC端口，则返回TRUE。
//----------------------------------------------------------------------
bool PollSocket(int sockID)
{
    return PollFile(sockID); // 在UNIX中，套接字ID只是文件ID
}

//----------------------------------------------------------------------
// ReadFromSocket
// 	从IPC端口读取固定大小的数据包。如果出错则中止。
//----------------------------------------------------------------------
void ReadFromSocket(int sockID, char *buffer, int packetSize)
{
    int retVal;
    //    extern int errno;
    struct sockaddr_un uName;
    unsigned int size = sizeof(uName);

    retVal = recvfrom(sockID, buffer, packetSize, 0,
                      (struct sockaddr *)&uName, &size);

    if (retVal != packetSize)
    {
        perror("在recvfrom中出错");
#ifdef HOST_ALPHA
        printf("调用: %lx, 返回 %d, %d\n", (long)buffer, retVal, errno);
#else
        printf("调用: %x, 返回 %d, %d\n", (int)buffer, retVal, errno);
#endif
    }
    ASSERT(retVal == packetSize);
}

//----------------------------------------------------------------------
// SendToSocket
// 	将固定大小的数据包传输到另一个Nachos的IPC端口。
//----------------------------------------------------------------------
void SendToSocket(int sockID, char *buffer, int packetSize, char *toName)
{
    //  extern int errno;
    struct sockaddr_un uName;
    int retVal;

    InitSocketName(&uName, toName);

    /*
     * 修改者：Marcello Lioy: 1996年3月4日
     *
     * 这现在循环，直到数据包成功发送，或者因其他原因失败
     * 除了套接字已满。
     */
    while (1)
    {
#if defined(HOST_LINUX) || defined(HOST_ALPHA)
        retVal = sendto(sockID, buffer, packetSize, 0,
                        (struct sockaddr *)&uName, sizeof(uName));
#else
        retVal = sendto(sockID, buffer, packetSize, 0,
                        (char *)&uName, sizeof(uName));
#endif /* HOST_LINUX */
        if (!(retVal < 0))
            break;
        else if (retVal < 0 && errno != ENOBUFS)
        {
            perror("套接字写入失败:");
            ASSERT(0);
        }
        sleep(1); // 这给接收者一个机会来读取
    }

    return;
}

//----------------------------------------------------------------------
// CallOnUserAbort
// 	安排在用户中止时调用“func”（例如，
//	通过按ctl-C）。
//----------------------------------------------------------------------

void CallOnUserAbort(VoidNoArgFunctionPtr func)
{
#ifdef HOST_ALPHA
    (void)signal(SIGINT, (void (*)(int))func);
#else
    (void)signal(SIGINT, (VoidFunctionPtr)func);
#endif
}

//----------------------------------------------------------------------
// Sleep
// 	让运行Nachos的UNIX进程睡眠x秒，
//	以便用户有时间在另一个UNIX shell中启动另一个Nachos的实例。
//----------------------------------------------------------------------

void Delay(int seconds)
{
    (void)sleep((unsigned)seconds);
}

//----------------------------------------------------------------------
// Abort
// 	退出并生成核心转储。
//----------------------------------------------------------------------

void Abort()
{
    abort();
}

//----------------------------------------------------------------------
// Exit
// 	退出而不生成核心转储。
//----------------------------------------------------------------------

void Exit(int exitCode)
{
    exit(exitCode);
}

//----------------------------------------------------------------------
// RandomInit
// 	初始化伪随机数生成器。我们使用
//	现在已过时的“srand”和“rand”，因为它们更具可移植性！
//----------------------------------------------------------------------

void RandomInit(unsigned seed)
{
    srand(seed);
}

//----------------------------------------------------------------------
// Random
// 	返回一个伪随机数。
//----------------------------------------------------------------------

int Random()
{
    return rand();
}

//----------------------------------------------------------------------
// AllocBoundedArray
// 	返回一个数组，前后各有两个页面未映射，以捕获非法引用
//	超出数组的末尾。特别有助于捕获超出固定大小线程执行栈的溢出。
//
//	注意：只返回有用的部分！
//
//	"size" -- 所需的有用空间量（以字节为单位）
//----------------------------------------------------------------------

char *
AllocBoundedArray(int size)
{
    int pgSize = getpagesize();
    char *ptr = new char[pgSize * 2 + size];

    mprotect(ptr, pgSize, 0);
    mprotect(ptr + pgSize + size, pgSize, 0);
    return ptr + pgSize;
}

//----------------------------------------------------------------------
// DeallocBoundedArray
// 	释放一个整数数组，解除其两个边界页面的保护。
//
//	"ptr" -- 要释放的数组
//	"size" -- 数组中的有用空间量（以字节为单位）
//----------------------------------------------------------------------

void DeallocBoundedArray(char *ptr, int size)
{
    int pgSize = getpagesize();

    mprotect(ptr - pgSize, pgSize, PROT_READ | PROT_WRITE | PROT_EXEC);
    mprotect(ptr + size, pgSize, PROT_READ | PROT_WRITE | PROT_EXEC);
    delete[] (ptr - pgSize);
}
