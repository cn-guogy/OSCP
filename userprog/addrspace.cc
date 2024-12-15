// addrspace.cc 
//	管理地址空间（执行用户程序）的例程。
//
//	为了运行一个用户程序，你必须：
//
//	1. 使用 -N -T 0 选项链接 
//	2. 运行 coff2noff 将目标文件转换为 Nachos 格式
//		（Nachos 目标代码格式本质上只是 UNIX 可执行目标代码格式的简化版本）
//	3. 将 NOFF 文件加载到 Nachos 文件系统中
//		（如果你还没有实现文件系统，你
//		不需要执行最后一步）
//

#include "system.h"
#include "addrspace.h"
#include "noff.h"

//----------------------------------------------------------------------
// SwapHeader
// 	对对象文件头中的字节进行小端到大端的转换，
// 以防文件是在小端机器上生成的，而我们现在在大端机器上运行。
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	创建一个地址空间以运行用户程序。
//	从文件 "executable" 加载程序，并设置一切
//	以便我们可以开始执行用户指令。
//
//	假设目标代码文件是 NOFF 格式。
//
//	首先，设置程序内存到物理内存的转换。
//	目前，这非常简单（1:1），因为我们只进行单程序运行，
//	并且我们有一个单一的未分段页表。
//
//	"executable" 是包含要加载到内存中的目标代码的文件。
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable)
{
    NoffHeader noffH;
    unsigned int i, size;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

// 地址空间有多大？
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// 我们需要增加大小
						// 以留出堆栈的空间
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    ASSERT(numPages <= NumPhysPages);		// 检查我们没有尝试
						// 运行任何过大的程序 --
						// 至少在我们有
						// 虚拟内存之前

    DEBUG('a', "初始化地址空间，页数 %d，大小 %d\n", 
					numPages, size);
// 首先，设置转换 
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
	pageTable[i].virtualPage = i;	// 目前，虚拟页号 = 物理页号
	pageTable[i].physicalPage = i;
	pageTable[i].valid = TRUE;
	pageTable[i].use = FALSE;
	pageTable[i].dirty = FALSE;
	pageTable[i].readOnly = FALSE;  // 如果代码段完全在 
					// 单独的页面上，我们可以将其 
					// 页面设置为只读
    }
    
// 将整个地址空间清零，以清零未初始化的数据段 
// 和堆栈段
    bzero(machine->mainMemory, size);

// 然后，将代码和数据段复制到内存中
    if (noffH.code.size > 0) {
        DEBUG('a', "初始化代码段，地址 0x%x，大小 %d\n", 
			noffH.code.virtualAddr, noffH.code.size);
        executable->ReadAt(&(machine->mainMemory[noffH.code.virtualAddr]),
			noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0) {
        DEBUG('a', "初始化数据段，地址 0x%x，大小 %d\n", 
			noffH.initData.virtualAddr, noffH.initData.size);
        executable->ReadAt(&(machine->mainMemory[noffH.initData.virtualAddr]),
			noffH.initData.size, noffH.initData.inFileAddr);
    }

}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	释放一个地址空间。现在没有任何操作！
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
   delete [] pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	设置用户级寄存器集的初始值。
//
// 	我们将这些直接写入 "machine" 寄存器，
// 以便我们可以立即跳转到用户代码。请注意，这些
// 将在上下文切换时保存/恢复到当前线程的 userRegisters 中。
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // 初始程序计数器 -- 必须是 "Start" 的位置
    machine->WriteRegister(PCReg, 0);	

    // 还需要告诉 MIPS 下一个指令的位置，因为
    // 存在分支延迟的可能性
    machine->WriteRegister(NextPCReg, 4);

   // 将堆栈寄存器设置为地址空间的末尾，那里
   // 我们分配了堆栈；但减去一点，以确保我们不
   // 意外地引用超出末尾！
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "将堆栈寄存器初始化为 %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	在上下文切换时，保存与此地址空间相关的任何机器状态。
//
//	目前，没有任何操作！
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	在上下文切换时，恢复机器状态，以便
// 这个地址空间可以运行。
//
//      目前，告诉机器在哪里找到页表。
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}
