// console.cc 
//	用于模拟串行端口到控制台设备的例程。
//	控制台有输入（键盘）和输出（显示器）。
//	这些都是通过对 UNIX 文件的操作来模拟的。
//	模拟的设备是异步的，
//	因此我们必须调用中断处理程序（经过模拟的延迟后），
//	以指示一个字节已经到达和/或一个已写入的字节已经离开。
//
//  请勿更改 -- 机器仿真的一部分
//


#include "console.h"
#include "system.h"

// 虚拟函数，因为 C++ 对成员函数指针的处理很奇怪
static void ConsoleReadPoll(_int c) 
{ Console *console = (Console *)c; console->CheckCharAvail(); }
static void ConsoleWriteDone(_int c)
{ Console *console = (Console *)c; console->WriteDone(); }

//----------------------------------------------------------------------
// Console::Console
// 	初始化硬件控制台设备的仿真。
//
//	"readFile" -- 模拟键盘的 UNIX 文件（NULL -> 使用 stdin）
//	"writeFile" -- 模拟显示器的 UNIX 文件（NULL -> 使用 stdout）
// 	"readAvail" 是当字符从键盘到达时调用的中断处理程序
// 	"writeDone" 是当字符已输出时调用的中断处理程序，
//	以便可以请求下一个字符进行输出
//----------------------------------------------------------------------

Console::Console(char *readFile, char *writeFile, VoidFunctionPtr readAvail, 
		VoidFunctionPtr writeDone, _int callArg)
{
    if (readFile == NULL)
	readFileNo = 0;					// 键盘 = stdin
    else
    	readFileNo = OpenForReadWrite(readFile, TRUE);	// 应该是只读
    if (writeFile == NULL)
	writeFileNo = 1;				// 显示器 = stdout
    else
    	writeFileNo = OpenForWrite(writeFile);

    // 设置以模拟异步中断的内容
    writeHandler = writeDone;
    readHandler = readAvail;
    handlerArg = callArg;
    putBusy = FALSE;
    incoming = EOF;

    // 开始轮询传入的数据包
    interrupt->Schedule(ConsoleReadPoll, (_int)this, ConsoleTime, ConsoleReadInt);
}

//----------------------------------------------------------------------
// Console::~Console
// 	清理控制台仿真
//----------------------------------------------------------------------

Console::~Console()
{
    if (readFileNo != 0)
	Close(readFileNo);
    if (writeFileNo != 1)
	Close(writeFileNo);
}

//----------------------------------------------------------------------
// Console::CheckCharAvail()
// 	定期调用以检查是否有字符可供从模拟键盘输入（例如，是否已输入）。
//
//	仅在有缓冲区空间时读取（如果之前的字符已被 Nachos 内核从缓冲区中提取）。
//	一旦字符被放入缓冲区，调用 "read" 中断处理程序。
//----------------------------------------------------------------------

void
Console::CheckCharAvail()
{
    char c;

    // 安排下次轮询数据包的时间
    interrupt->Schedule(ConsoleReadPoll, (_int)this, ConsoleTime, 
			ConsoleReadInt);

    // 如果字符已经缓冲，或者没有可读字符，则不执行任何操作
    if ((incoming != EOF) || !PollFile(readFileNo))
	return;	  

    // 否则，读取字符并通知用户
    Read(readFileNo, &c, sizeof(char));
    incoming = c ;
    stats->numConsoleCharsRead++;
    (*readHandler)(handlerArg);	
}

//----------------------------------------------------------------------
// Console::WriteDone()
// 	内部例程，当调用中断处理程序以告知 Nachos 内核输出字符已完成时调用。
//----------------------------------------------------------------------

void
Console::WriteDone()
{
    putBusy = FALSE;
    stats->numConsoleCharsWritten++;
    (*writeHandler)(handlerArg);
}

//----------------------------------------------------------------------
// Console::GetChar()
// 	从输入缓冲区读取字符，如果有的话。
//	返回字符，或者如果没有缓冲则返回 EOF。
//----------------------------------------------------------------------

char
Console::GetChar()
{
   char ch = incoming;

   incoming = EOF;
   return ch;
}

//----------------------------------------------------------------------
// Console::PutChar()
// 	将字符写入模拟显示器，安排将来发生的中断，并返回。
//----------------------------------------------------------------------

void
Console::PutChar(char ch)
{
    ASSERT(putBusy == FALSE);
    WriteFile(writeFileNo, &ch, sizeof(char));
    putBusy = TRUE;
    interrupt->Schedule(ConsoleWriteDone, (_int)this, ConsoleTime,
					ConsoleWriteInt);
}
