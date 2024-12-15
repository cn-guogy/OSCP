// machine.h
//	用于模拟在Nachos上运行的用户程序的执行的数据结构。
//
//	用户程序被加载到“mainMemory”中；对Nachos来说，
//	这看起来就像一个字节数组。当然，Nachos的内核也在内存中 --
//	但与如今大多数机器一样，内核被加载到与用户程序分开的内存区域中，
//	并且对内核内存的访问不会被翻译或分页。
//
//	在Nachos中，用户程序是由模拟器逐条指令执行的。
//	每个内存引用都会被翻译、检查错误等。
//
//  请勿更改 -- 机器仿真的一部分
//

#ifndef MACHINE_H
#define MACHINE_H

#include "utility.h"
#include "translate.h"
#include "disk.h"

// 与用户内存的大小和格式相关的定义

#define PageSize SectorSize // 将页面大小设置为
							// 磁盘扇区大小，以简化

#define NumPhysPages 32
#define MemorySize (NumPhysPages * PageSize)
#define TLBSize 4 // 如果有TLB，则将其设置为小

enum ExceptionType
{
	NoException,		   // 一切正常！
	SyscallException,	   // 程序执行了系统调用。
	PageFaultException,	   // 找不到有效的翻译
	ReadOnlyException,	   // 尝试写入标记为
						   // “只读”的页面
	BusErrorException,	   // 翻译导致无效的物理地址
	AddressErrorException, // 未对齐的引用或超出
						   // 地址空间末尾的引用
	OverflowException,	   // 加法或减法中的整数溢出。
	IllegalInstrException, // 未实现或保留的指令。

	NumExceptionTypes
};

// 用户程序的CPU状态。完整的MIPS寄存器集，加上一些
// 额外的寄存器，因为我们需要能够在
// 任何两条指令之间启动/停止用户程序（因此我们需要跟踪
// 加载延迟槽等）。

#define StackReg 29	  // 用户的栈指针
#define RetAddrReg 31 // 保存过程调用的返回地址
#define NumGPRegs 32  // MIPS上的32个通用寄存器
#define HiReg 32	  // 用于保存乘法结果的双寄存器
#define LoReg 33
#define PCReg 34		// 当前程序计数器
#define NextPCReg 35	// 下一个程序计数器（用于分支延迟）
#define PrevPCReg 36	// 上一个程序计数器（用于调试）
#define LoadReg 37		// 延迟加载的目标寄存器。
#define LoadValueReg 38 // 延迟加载要加载的值。
#define BadVAddrReg 39	// 异常时失败的虚拟地址

#define NumTotalRegs 40

// 以下类定义了一条指令，以未解码的二进制形式表示
// 	解码以识别
//	    要执行的操作
//	    要操作的寄存器
//	    任何立即操作数值

class Instruction
{
public:
	void Decode(); // 解码指令的二进制表示

	unsigned int value; // 指令的二进制表示

	unsigned char opCode;	  // 指令类型。这与
							  // 指令中的操作码字段不同：请参见mips.h中的定义
	unsigned char rs, rt, rd; // 指令中的三个寄存器。
	int extra;				  // 立即数或目标或shamt字段或偏移量。
							  // 立即数是符号扩展的。
};

// 以下类定义了用户程序所见的模拟主机工作站硬件，
// 包括CPU寄存器、主内存等。
// 用户程序不应该能够分辨它们是在我们的
// 模拟器上运行还是在真实硬件上运行，除了
//	我们不支持浮点指令
//	Nachos的系统调用接口与UNIX不同
//	  （Nachos中有10个系统调用，而UNIX中有200个！）
// 如果我们要实现更多的UNIX系统调用，我们应该能够在
// Nachos上运行Nachos！
//
// 此类中的过程在machine.cc、mipssim.cc和
// translate.cc中定义。

class Machine
{
public:
	Machine(bool debug); // 初始化硬件的仿真
						 // 以运行用户程序
	~Machine(); // 释放数据结构

	// 可由Nachos内核调用的例程
	void Run(); // 运行用户程序

	int ReadRegister(int num); // 读取CPU寄存器的内容

	void WriteRegister(int num, int value);
	// 将值存储到CPU寄存器中

	// 机器仿真内部的例程 -- 请勿调用这些

	void OneInstruction(Instruction *instr);
	// 运行用户程序的一条指令。
	void DelayedLoad(int nextReg, int nextVal);
	// 执行待处理的延迟加载（修改寄存器）

	bool ReadMem(int addr, int size, int *value);
	bool WriteMem(int addr, int size, int value);
	// 读取或写入虚拟内存的1、2或4个字节
	// （在addr处）。如果找不到
	// 正确的翻译，则返回FALSE。

	ExceptionType Translate(int virtAddr, int *physAddr, int size, bool writing);
	// 翻译地址，并检查
	// 对齐。适当地设置使用和脏位
	// 在翻译条目中，
	// 如果翻译无法完成，则返回异常代码。

	void RaiseException(ExceptionType which, int badVAddr);
	// 因系统调用或其他异常而陷入Nachos内核。

	void Debugger();  // 调用用户程序调试器
	void DumpState(); // 打印用户CPU和内存状态

	// 数据结构 -- 所有这些都可以被Nachos内核代码访问。
	// “public”是为了方便。
	//
	// 请注意，用户程序与内核之间的*所有*通信
	// 都是通过这些数据结构进行的。

	char *mainMemory; // 存储用户程序的物理内存，
					  // 在执行时存储代码和数据
	int registers[NumTotalRegs]; // 执行用户程序的CPU寄存器

	// 注意：用户程序中虚拟地址到物理地址的硬件翻译
	// （相对于“mainMemory”的起始位置）
	// 可以通过以下方式控制：
	//	传统的线性页表
	//  	软件加载的翻译后备缓冲区（tlb） -- 一个缓存
	//	  虚拟页号到物理页号的映射
	//
	// 如果“tlb”为NULL，则使用线性页表
	// 如果“tlb”非NULL，则Nachos内核负责管理
	//	TLB的内容。
	// 但内核可以使用任何数据结构
	//	它想要（例如，分段分页）来处理TLB缓存未命中。
	//
	// 为了简单起见，页表指针和TLB指针都是
	// 公共的。然而，虽然可以有多个页表（每个地址
	// 空间一个，存储在内存中），但只有一个TLB（在硬件中实现）。
	// 因此，TLB指针应被视为*只读*，尽管
	// TLB的内容可以被内核软件修改。

	TranslationEntry *tlb; // 此指针应被视为
						   // 对Nachos内核代码“只读”

	TranslationEntry *pageTable;
	unsigned int pageTableSize;

private:
	bool singleStep; // 在每条
					 // 模拟指令后返回到调试器
	int runUntilTime; // 当模拟
					  // 时间达到此值时返回到调试器
};

extern void ExceptionHandler(ExceptionType which);
// 处理用户系统调用和异常的
// 入口点
// 定义在exception.cc

// 将字和短字转换为模拟机器的小端格式的例程。如果主机机器
// 是小端（DEC和Intel），这些最终将是NOPs。
//
// 每种格式中存储的内容：
//	主机字节顺序：
//	   内核数据结构
//	   用户寄存器
//	模拟机器字节顺序：
//	   主内存的内容

unsigned int WordToHost(unsigned int word);
unsigned short ShortToHost(unsigned short shortword);
unsigned int WordToMachine(unsigned int word);
unsigned short ShortToMachine(unsigned short shortword);

#endif // MACHINE_H
