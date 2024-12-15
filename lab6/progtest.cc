// progtest.cc 修改自 userprog/progtest.cc
// 为StartProcess添加返回值

#include "system.h"
#include "console.h"
#include "addrspace.h"
#include "synch.h"

//----------------------------------------------------------------------
// StartProcess
// 	运行用户程序。打开可执行文件，将其加载到
//	内存中，并跳转到它。
//----------------------------------------------------------------------

int StartProcess(char *filename)
{
    OpenFile *executable = fileSystem->Open(filename);
    AddrSpace *space;

    if (executable == NULL)
    {
        printf("无法打开文件 %s\n", filename);
        return -1;
    }
    space = new AddrSpace(executable);
    currentThread->space = space;
    delete executable; // 关闭文件

    space->InitRegisters(); // 设置初始寄存器值
    space->RestoreState();  // 加载页表寄存器

    machine->Run(); // 跳转到用户程序
    ASSERT(FALSE);  // machine->Run 永远不会返回;
                    // 地址空间通过执行系统调用 "exit" 退出
    return space->getSpaceId();
}

// 控制台测试所需的数据结构。发出
// I/O 请求的线程在信号量上等待，直到 I/O 完成。

static Console *console;
static Semaphore *readAvail;
static Semaphore *writeDone;

//----------------------------------------------------------------------
// ConsoleInterruptHandlers
// 	唤醒请求 I/O 的线程。
//----------------------------------------------------------------------

static void ReadAvail(_int arg) { readAvail->V(); }
static void WriteDone(_int arg) { writeDone->V(); }

//----------------------------------------------------------------------
// ConsoleTest
// 	通过将输入中输入的字符回显到
//	输出中来测试控制台。当用户输入 'q' 时停止。
//----------------------------------------------------------------------

void ConsoleTest(char *in, char *out)
{
    char ch;

    console = new Console(in, out, ReadAvail, WriteDone, 0);
    readAvail = new Semaphore("读取可用", 0);
    writeDone = new Semaphore("写入完成", 0);

    for (;;)
    {
        readAvail->P(); // 等待字符到达
        ch = console->GetChar();
        console->PutChar(ch); // 回显字符！
        writeDone->P();       // 等待写入完成
        if (ch == 'q')
            return; // 如果输入 'q'，则退出
    }
}
