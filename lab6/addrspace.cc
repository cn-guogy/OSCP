// addrspace.cc 修改自 userprog/addrspace.cc
// print方法
// 修改构造函数，每次创建一个地址空间时，寻找空的物理页
// 修改析构函数，释放物理页

#include "system.h"
#include "addrspace.h"
#include "noff.h"

static void
SwapHeader(NoffHeader *noffH)
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

void AddrSpace::print()
{
    printf("ID: %u\n", spaceId);
    printf("页表转储: 总共 %d 页\n", numPages);
    printf("============================================\n");
    printf("虚拟页, 物理页\n");
    for (unsigned int i = 0; i < numPages; i++)
        printf("  %3d,   %3d\n", pageTable[i].virtualPage, pageTable[i].physicalPage);
    printf("============================================\n\n");
}

AddrSpace::AddrSpace(OpenFile *executable)
{

    NoffHeader noffH;
    unsigned int i, size;
    space++;
    spaceId = space;
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) &&
        (WordToHost(noffH.noffMagic) == NOFFMAGIC))
        SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    // 地址空间有多大？
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size + UserStackSize; // 我们需要增加大小
                                                                                          // 以留出堆栈的空间
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    ASSERT(numPages <= NumPhysPages); // 检查我们没有尝试
                                      // 运行任何过大的程序 --
                                      // 至少在我们有
                                      // 虚拟内存之前

    DEBUG('a', "初始化地址空间，页数 %d，大小 %d\n",
          numPages, size);
    // 首先，设置转换
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++)
    {
        pageTable[i].virtualPage = i;                     // 目前，虚拟页号 = 物理页号
        pageTable[i].physicalPage = freePhys_Map->Find(); // 从内存中找到一个空闲的物理页
        pageTable[i].valid = TRUE;
        pageTable[i].use = FALSE;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE; // 如果代码段完全在
                                       // 单独的页面上，我们可以将其
                                       // 页面设置为只读
    }

    // 将整个地址空间清零，以清零未初始化的数据段
    // 和堆栈段
    bzero(machine->mainMemory, size);

    // 然后，将代码和数据段复制到内存中
    if (noffH.code.size > 0)
    {
        DEBUG('a', "初始化代码段，地址 0x%x，大小 %d\n",
              noffH.code.virtualAddr, noffH.code.size);
        unsigned int code_page = noffH.code.virtualAddr / PageSize;
        unsigned int code_phy_addr = pageTable[code_page].physicalPage * PageSize;
        executable->ReadAt(&(machine->mainMemory[code_phy_addr]), noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0)
    {
        DEBUG('a', "初始化数据段，地址 0x%x，大小 %d\n",
              noffH.initData.virtualAddr, noffH.initData.size);
        unsigned int data_page = noffH.initData.virtualAddr / PageSize;
        unsigned int data_offset = noffH.initData.virtualAddr % PageSize;
        unsigned int data_phy_addr = pageTable[data_page].physicalPage * PageSize + data_offset;
        executable->ReadAt(&(machine->mainMemory[data_phy_addr]), noffH.initData.size, noffH.initData.inFileAddr);
    }
    print();
}

AddrSpace::~AddrSpace()
{
    for (unsigned int i = 0; i < numPages; i++)
    {
        freePhys_Map->Clear(pageTable[i].physicalPage);
    }
    delete[] pageTable;
}

void AddrSpace::InitRegisters()
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

void AddrSpace::SaveState()
{
}

void AddrSpace::RestoreState()
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}
