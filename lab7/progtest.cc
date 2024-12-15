// progtest.cc 复制自 lab6/progtest.cc
// 无修改

#include "system.h"
#include "console.h"
#include "addrspace.h"
#include "synch.h"

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
    delete executable;

    space->InitRegisters();
    space->RestoreState();
    machine->Run();
    return space->getSpaceId();
}

static Console *console;
static Semaphore *readAvail;
static Semaphore *writeDone;

static void ReadAvail(_int arg) { readAvail->V(); }
static void WriteDone(_int arg) { writeDone->V(); }

void ConsoleTest(char *in, char *out)
{
    char ch;

    console = new Console(in, out, ReadAvail, WriteDone, 0);
    readAvail = new Semaphore("读取可用", 0);
    writeDone = new Semaphore("写入完成", 0);

    for (;;)
    {
        readAvail->P();
        ch = console->GetChar();
        console->PutChar(ch);
        writeDone->P();
        if (ch == 'q')
            return;
    }
}
