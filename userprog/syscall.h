/* syscalls.h 
 * 	Nachos 系统调用接口。这些是可以从用户程序调用的 Nachos 内核操作，
 * 通过 "syscall" 指令陷入内核。
 *
 *	此文件由用户程序和 Nachos 内核包含。 
 */

#ifndef SYSCALLS_H
#define SYSCALLS_H

/* 系统调用代码 -- 由存根用于告诉内核请求哪个系统调用 */
#define SC_Halt		0
#define SC_Exit		1
#define SC_Exec		2
#define SC_Join		3
#define SC_Create	4
#define SC_Open		5
#define SC_Read		6
#define SC_Write	7
#define SC_Close	8
#define SC_Fork		9
#define SC_Yield	10
#define SC_PrintInt	11

#ifndef IN_ASM

/* 系统调用接口。这些是 Nachos 内核需要支持的操作，以便能够运行用户程序。
 *
 * 每个操作都是通过用户程序简单地调用过程来触发的；一个汇编语言存根将系统调用代码
 * 放入寄存器中，并陷入内核。然后在 Nachos 内核中调用内核过程，经过适当的错误检查，
 * 从 exception.cc 中的系统调用入口点调用。
 */

/* 停止 Nachos，并打印性能统计信息 */
void Halt();		

/* 地址空间控制操作：Exit、Exec 和 Join */

/* 此用户程序已完成（状态 = 0 表示正常退出）。 */
void Exit(int status);	

/* 运行存储在 Nachos 文件 "name" 中的可执行文件，并返回 
 * 地址空间标识符
 */
int Exec(char *name);

/* 仅在用户程序 "id" 完成后返回。  
 * 返回退出状态。
 */
int Join(int id); 	

/* 文件系统操作：Create、Open、Read、Write、Close
 * 这些函数的模式类似于 UNIX -- 文件同时表示
 * 文件和硬件 I/O 设备。
 *
 * 如果在进行文件系统作业之前完成此作业，
 * 请注意 Nachos 文件系统有一个存根实现，
 * 这将适用于测试这些例程的目的。
 */

/* 打开的 Nachos 文件的唯一标识符。 */
typedef int OpenFileId;	

/* 当地址空间启动时，它有两个打开的文件，表示 
 * 键盘输入和显示输出（在 UNIX 术语中，stdin 和 stdout）。
 * 可以直接在这些上使用 Read 和 Write，而无需先打开
 * 控制台设备。
 */

#define ConsoleInput	0  
#define ConsoleOutput	1  

/* 创建一个名为 "name" 的 Nachos 文件 */
void Create(char *name);

/* 打开 Nachos 文件 "name"，并返回一个可以 
 * 用于读写该文件的 "OpenFileId"。
 */
OpenFileId Open(char *name);

/* 将 "size" 字节从 "buffer" 写入打开的文件。 */
void Write(char *buffer, int size, OpenFileId id);

/* 从打开的文件中读取 "size" 字节到 "buffer"。  
 * 返回实际读取的字节数 -- 如果打开的文件不够长，
 * 或者如果它是一个 I/O 设备，并且没有足够的 
 * 字符可供读取，则返回可用的任何内容（对于 I/O 设备，
 * 您应该始终等待直到可以返回至少一个字符）。
 */
int Read(char *buffer, int size, OpenFileId id);

/* 关闭文件，我们完成了对它的读写。 */
void Close(OpenFileId id);

/* 用户级线程操作：Fork 和 Yield。 允许多个
 * 线程在用户程序中运行。 
 */

/* Fork 一个线程在与当前线程相同的地址空间中运行一个过程 ("func")。 */
void Fork(void (*func)());

/* 将 CPU 让给另一个可运行的线程，无论是在此地址空间 
 * 内还是外。 
 */
void Yield();

// 打印整数 "num"
void PrintInt(int num);		

#endif /* IN_ASM */

#endif /* SYSCALL_H */
