// addrSpace.cc 修改自 lab6/addrspace.cc
// 创建交换文件方法
// 找到需要换出的页FIFO

#include "system.h"
#include "addrspace.h"
#include "noff.h"

void AddrSpace::WriteToSwap(int outPage)
{
    if (pageTable[outPage].dirty)
    {
        swapFile->WriteAt(&(machine->mainMemory[pageTable[outPage].physicalPage * PageSize]),
                          PageSize, outPage * PageSize);
        stats->numPageWrites++;
        printf("将页 %d 写入交换文件\n", outPage);
    }
    else
    {
        printf("此页没有被修改，不需要写入交换文件\n");
    }
}

int AddrSpace::FindPageOut()
{
    return pageInMem[idx];
}

void AddrSpace::GetPageToMem(int needPage)
{
    stats->numPageFaults++;
    int outPage = FindPageOut();
    if (outPage < 0)
    {
        printf("此时内存还未达到最大帧数，所以可以分配一个物理页");
        pageTable[needPage].physicalPage = freePhys_Map->Find();
        printf("将页 %d 分配到物理页 %d\n", needPage, pageTable[needPage].physicalPage);
    }
    else
    {
        WriteToSwap(outPage);
        pageTable[needPage].physicalPage = pageTable[outPage].physicalPage;
        pageTable[outPage].physicalPage = -1;
        pageTable[outPage].valid = FALSE;
        printf("将页 %d 换出到交换文件\n", outPage);
    }
    pageInMem[idx] = needPage;
    idx = (idx + 1) % maxFramesPerProc;
    pageTable[needPage].valid = TRUE;
    pageTable[needPage].use = TRUE;
    pageTable[needPage].dirty = FALSE;

    swapFile->ReadAt(&(machine->mainMemory[pageTable[needPage].physicalPage * PageSize]),
                     PageSize, needPage * PageSize);
    print();
}

OpenFile *AddrSpace::CreateSwapFile(int pageSize)
{
    char swapFileName[20];
    sprintf(swapFileName, "SWAP%d", spaceId);
    if (!fileSystem->Create(swapFileName, 0))
    {
        printf("无法创建交换文件 %s\n", swapFileName);
        currentThread->Finish();
    }
    swapFile = fileSystem->Open(swapFileName);
    if (swapFile == NULL)
    {
        printf("无法打开交换文件 %s\n", swapFileName);
        currentThread->Finish();
    }
    char *pageBuf = new char[PageSize];
    bzero(pageBuf, PageSize);
    for (int i = 0; i < numPages; i++)
        swapFile->WriteAt(pageBuf, PageSize, i * PageSize);
    delete[] pageBuf;
    return swapFile;
}

static void SwapHeader(NoffHeader *noffH)
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
    printf("虚拟页, 物理页,valid,use,dirty\n");
    for (unsigned int i = 0; i < numPages; i++)
        printf("  %3d,   %3d,  %3d, %3d, %3d\n", pageTable[i].virtualPage, pageTable[i].physicalPage, pageTable[i].valid, pageTable[i].use, pageTable[i].dirty);
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

    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size + UserStackSize;
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    ASSERT(numPages <= NumPhysPages);

    DEBUG('a', "初始化地址空间，页数 %d，大小 %d\n",
          numPages, size);
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++)
    {
        pageTable[i].virtualPage = i;
        pageTable[i].physicalPage =-1;
        pageTable[i].valid = FALSE;
        pageTable[i].use = FALSE;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE;
    }

    CreateSwapFile(PageSize);

    if (noffH.code.size > 0)
    {
        DEBUG('a', "初始化代码段，地址 0x%x，大小 %d\n",
              noffH.code.virtualAddr, noffH.code.size);
        char *buf = new char[noffH.code.size];
        executable->ReadAt(buf, noffH.code.size, noffH.code.inFileAddr);
        swapFile->WriteAt(buf, noffH.code.size, noffH.code.virtualAddr);
        delete[] buf;
    }
    if (noffH.initData.size > 0)
    {
        DEBUG('a', "初始化数据段，地址 0x%x，大小 %d\n",
              noffH.initData.virtualAddr, noffH.initData.size);
        char *buf = new char[noffH.initData.size];
        executable->ReadAt(buf, noffH.initData.size, noffH.initData.inFileAddr);
        swapFile->WriteAt(buf, noffH.initData.size, noffH.initData.virtualAddr);
        delete[] buf;
    }

    print();

    pageInMem = new int[maxFramesPerProc];
    for (i = 0; i < maxFramesPerProc; i++)
        pageInMem[i] = -1;
    idx = 0;
    return;
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

    machine->WriteRegister(PCReg, 0);

    machine->WriteRegister(NextPCReg, 4);

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
