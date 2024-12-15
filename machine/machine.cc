// machine.cc 
//	用于模拟用户程序执行的例程。
//
//  请勿更改 -- 机器仿真的一部分
//

#include "machine.h"
#include "system.h"

// 用户程序执行时可能生成的异常的文本名称，用于调试。
static const char* exceptionNames[] = { "无异常", "系统调用", 
				"页面错误/无TLB条目", "页面只读",
				"总线错误", "地址错误", "溢出",
				"非法指令" };

//----------------------------------------------------------------------
// CheckEndian
// 	检查主机是否确实使用其所声明的格式存储整数的字节。遇到错误时停止。
//----------------------------------------------------------------------

static
void CheckEndian()
{
    union checkit {
        char charword[4];
        unsigned int intword;
    } check;

    check.charword[0] = 1;
    check.charword[1] = 2;
    check.charword[2] = 3;
    check.charword[3] = 4;

#ifdef HOST_IS_BIG_ENDIAN
    ASSERT (check.intword == 0x01020304);
#else
    ASSERT (check.intword == 0x04030201);
#endif
}

//----------------------------------------------------------------------
// Machine::Machine
// 	初始化用户程序执行的仿真。
//
//	"debug" -- 如果为TRUE，在每条用户指令执行后进入调试器。
//----------------------------------------------------------------------

Machine::Machine(bool debug)
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
        registers[i] = 0;
    mainMemory = new char[MemorySize];
    for (i = 0; i < MemorySize; i++)
      	mainMemory[i] = 0;
#ifdef USE_TLB
    tlb = new TranslationEntry[TLBSize];
    for (i = 0; i < TLBSize; i++)
	tlb[i].valid = FALSE;
    pageTable = NULL;
#else	// 使用线性页表
    tlb = NULL;
    pageTable = NULL;
#endif

    singleStep = debug;
    CheckEndian();
}

//----------------------------------------------------------------------
// Machine::~Machine
// 	释放用于模拟用户程序执行的数据结构。
//----------------------------------------------------------------------

Machine::~Machine()
{
    delete [] mainMemory;
    if (tlb != NULL)
        delete [] tlb;
}

//----------------------------------------------------------------------
// Machine::RaiseException
// 	将控制权从用户模式转移到Nachos内核，因为
//	用户程序要么调用了系统调用，要么发生了某些异常
//	（例如地址转换失败）。
//
//	"which" -- 内核陷阱的原因
//	"badVaddr" -- 导致陷阱的虚拟地址（如果适用）
//----------------------------------------------------------------------

void
Machine::RaiseException(ExceptionType which, int badVAddr)
{
    DEBUG('m', "异常: %s\n", exceptionNames[which]);
    
//  ASSERT(interrupt->getStatus() == UserMode);
    registers[BadVAddrReg] = badVAddr;
    DelayedLoad(0, 0);			// 完成任何正在进行的操作
    interrupt->setStatus(SystemMode);
    ExceptionHandler(which);		// 此时中断已启用
    interrupt->setStatus(UserMode);
}

//----------------------------------------------------------------------
// Machine::Debugger
// 	用户程序的基本调试器。请注意，我们不能使用
//	gdb来调试用户程序，因为gdb不在Nachos上运行。
//	可以，但你需要实现*更多*的系统调用
//	才能使其工作！
//
//	因此，只允许单步执行，并打印内存的内容。
//----------------------------------------------------------------------

void Machine::Debugger()
{
    char *buf = new char[80];
    int num;

    interrupt->DumpState();
    DumpState();
    printf("%d> ", stats->totalTicks);
    fflush(stdout);
    fgets(buf, 80, stdin);
    if (sscanf(buf, "%d", &num) == 1)
	runUntilTime = num;
    else {
	runUntilTime = 0;
	switch (*buf) {
	  case '\n':
	    break;
	    
	  case 'c':
	    singleStep = FALSE;
	    break;
	    
	  case '?':
	    printf("机器命令:\n");
	    printf("    <return>  执行一条指令\n");
	    printf("    <number>  运行直到给定的计时周期\n");
	    printf("    c         运行直到完成\n");
	    printf("    ?         打印帮助信息\n");
	    break;
	}
    }
    delete [] buf;
}
 
//----------------------------------------------------------------------
// Machine::DumpState
// 	打印用户程序的CPU状态。我们可能会打印内存的内容，
//	但这似乎有些过头。
//----------------------------------------------------------------------

void
Machine::DumpState()
{
    int i;
    
    printf("机器寄存器:\n");
    for (i = 0; i < NumGPRegs; i++)
	switch (i) {
	  case StackReg:
	    printf("\tSP(%d):\t0x%x%s", i, registers[i],
		   ((i % 4) == 3) ? "\n" : "");
	    break;
	    
	  case RetAddrReg:
	    printf("\tRA(%d):\t0x%x%s", i, registers[i],
		   ((i % 4) == 3) ? "\n" : "");
	    break;
	  
	  default:
	    printf("\t%d:\t0x%x%s", i, registers[i],
		   ((i % 4) == 3) ? "\n" : "");
	    break;
	}
    
    printf("\tHi:\t0x%x", registers[HiReg]);
    printf("\tLo:\t0x%x\n", registers[LoReg]);
    printf("\tPC:\t0x%x", registers[PCReg]);
    printf("\tNextPC:\t0x%x", registers[NextPCReg]);
    printf("\tPrevPC:\t0x%x\n", registers[PrevPCReg]);
    printf("\tLoad:\t0x%x", registers[LoadReg]);
    printf("\tLoadV:\t0x%x\n", registers[LoadValueReg]);
    printf("\n");
}

//----------------------------------------------------------------------
// Machine::ReadRegister/WriteRegister
//   	获取或写入用户程序寄存器的内容。
//----------------------------------------------------------------------

int Machine::ReadRegister(int num)
    {
	ASSERT((num >= 0) && (num < NumTotalRegs));
	return registers[num];
    }

void Machine::WriteRegister(int num, int value)
    {
	ASSERT((num >= 0) && (num < NumTotalRegs));
	// DEBUG('m', "写入寄存器 %d, 值 %d\n", num, value);
	registers[num] = value;
    }
