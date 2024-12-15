// translate.cc 
//	用于将虚拟地址转换为物理地址的例程。
//	软件设置一个合法转换的表。我们在每次内存引用时查找
//	该表以找到真实的物理内存位置。
//
// 这里支持两种类型的转换。
//
//	线性页表 -- 虚拟页号用作索引
//	到表中，以找到物理页号。
//
//	翻译后备缓冲区 -- 在表中进行关联查找
//	以找到具有相同虚拟页号的条目。如果找到，
//	则使用该条目进行转换。
//	如果没有，则会通过异常陷入软件。
//
//	实际上，TLB的大小远小于物理内存的大小
//	（在拥有数千页的机器上，16个条目是常见的）。因此，
//	还必须有一个备份转换方案
//	（例如页表），但硬件不需要知道
//	任何关于此的事情。
//
//	请注意，TLB的内容是特定于地址空间的。
//	如果地址空间发生变化，TLB的内容也会变化！
//
// 不要更改 -- 机器仿真的一部分
//

#include "machine.h"
#include "addrspace.h"
#include "system.h"

// 将字和短字转换为模拟机器的小端格式的例程。
// 当主机机器也是小端时，这些最终会变成NOP（DEC和Intel）。

unsigned int
WordToHost(unsigned int word) {
#ifdef HOST_IS_BIG_ENDIAN
	 register unsigned long result;
	 result = (word >> 24) & 0x000000ff;
	 result |= (word >> 8) & 0x0000ff00;
	 result |= (word << 8) & 0x00ff0000;
	 result |= (word << 24) & 0xff000000;
	 return result;
#else 
	 return word;
#endif /* HOST_IS_BIG_ENDIAN */
}

unsigned short
ShortToHost(unsigned short shortword) {
#ifdef HOST_IS_BIG_ENDIAN
	 register unsigned short result;
	 result = (shortword << 8) & 0xff00;
	 result |= (shortword >> 8) & 0x00ff;
	 return result;
#else 
	 return shortword;
#endif /* HOST_IS_BIG_ENDIAN */
}

unsigned int
WordToMachine(unsigned int word) { return WordToHost(word); }

unsigned short
ShortToMachine(unsigned short shortword) { return ShortToHost(shortword); }


//----------------------------------------------------------------------
// Machine::ReadMem
//      从虚拟内存地址 "addr" 读取 "size" (1, 2, 或 4) 字节到 
//	"value" 指向的位置。
//
//   	如果从虚拟到物理内存的转换步骤失败，返回 FALSE。
//
//	"addr" -- 要读取的虚拟地址
//	"size" -- 要读取的字节数 (1, 2, 或 4)
//	"value" -- 写入结果的地方
//----------------------------------------------------------------------

bool
Machine::ReadMem(int addr, int size, int *value)
{
    int data;
    ExceptionType exception;
    int physicalAddress;
    
    DEBUG('a', "读取虚拟地址 0x%x, 大小 %d\n", addr, size);
    
    exception = Translate(addr, &physicalAddress, size, FALSE);
    if (exception != NoException) {
	machine->RaiseException(exception, addr);
	return FALSE;
    }
    switch (size) {
      case 1:
	data = machine->mainMemory[physicalAddress];
	*value = data;
	break;
	
      case 2:
	data = *(unsigned short *) &machine->mainMemory[physicalAddress];
	*value = ShortToHost(data);
	break;
	
      case 4:
	data = *(unsigned int *) &machine->mainMemory[physicalAddress];
	*value = WordToHost(data);
	break;

      default: ASSERT(FALSE);
    }
    
    DEBUG('a', "\t读取的值 = %8.8x\n", *value);
    return (TRUE);
}

//----------------------------------------------------------------------
// Machine::WriteMem
//      将 "value" 的 "size" (1, 2, 或 4) 字节内容写入
//	虚拟内存地址 "addr"。
//
//   	如果从虚拟到物理内存的转换步骤失败，返回 FALSE。
//
//	"addr" -- 要写入的虚拟地址
//	"size" -- 要写入的字节数 (1, 2, 或 4)
//	"value" -- 要写入的数据
//----------------------------------------------------------------------

bool
Machine::WriteMem(int addr, int size, int value)
{
    ExceptionType exception;
    int physicalAddress;
     
    DEBUG('a', "写入虚拟地址 0x%x, 大小 %d, 值 0x%x\n", addr, size, value);

    exception = Translate(addr, &physicalAddress, size, TRUE);
    if (exception != NoException) {
	machine->RaiseException(exception, addr);
	return FALSE;
    }
    switch (size) {
      case 1:
	machine->mainMemory[physicalAddress] = (unsigned char) (value & 0xff);
	break;

      case 2:
	*(unsigned short *) &machine->mainMemory[physicalAddress]
		= ShortToMachine((unsigned short) (value & 0xffff));
	break;
      
      case 4:
	*(unsigned int *) &machine->mainMemory[physicalAddress]
		= WordToMachine((unsigned int) value);
	break;
	
      default: ASSERT(FALSE);
    }
    
    return TRUE;
}

//----------------------------------------------------------------------
// Machine::Translate
// 	将虚拟地址转换为物理地址，使用 
//	页表或TLB。检查对齐和各种其他 
//	错误，如果一切正常，设置转换表条目的使用/脏位，并将转换后的物理 
//	地址存储在 "physAddr" 中。如果发生错误，返回异常类型。
//
//	"virtAddr" -- 要转换的虚拟地址
//	"physAddr" -- 存储物理地址的地方
//	"size" -- 读取或写入的内存量
// 	"writing" -- 如果为 TRUE，检查 TLB 中的 "只读" 位
//----------------------------------------------------------------------

ExceptionType
Machine::Translate(int virtAddr, int* physAddr, int size, bool writing)
{
    int i;
    unsigned int vpn, offset;
    TranslationEntry *entry;
    unsigned int pageFrame;

    DEBUG('a', "\t转换 0x%x, %s: ", virtAddr, writing ? "写入" : "读取");

// 检查对齐错误
    if (((size == 4) && (virtAddr & 0x3)) || ((size == 2) && (virtAddr & 0x1))){
	DEBUG('a', "在 %d 处的对齐问题, 大小 %d!\n", virtAddr, size);
	return AddressErrorException;
    }
    
    // 我们必须有 TLB 或页表，但不能同时有两者！
    ASSERT(tlb == NULL || pageTable == NULL);	
    ASSERT(tlb != NULL || pageTable != NULL);	

// 从虚拟地址计算虚拟页号和页内偏移
    vpn = (unsigned) virtAddr / PageSize;
    offset = (unsigned) virtAddr % PageSize;
    
    if (tlb == NULL) {		// => 页表 => vpn 是表中的索引
	if (vpn >= pageTableSize) {
	    DEBUG('a', "虚拟页号 %d 超过页表大小 %d!\n", 
			virtAddr, pageTableSize);
	    return AddressErrorException;
	} else if (!pageTable[vpn].valid) {
	    DEBUG('a', "虚拟页号 %d 超过页表大小 %d!\n", 
			virtAddr, pageTableSize);
	    return PageFaultException;
	}
	entry = &pageTable[vpn];
    } else {
        for (entry = NULL, i = 0; i < TLBSize; i++)
    	    if (tlb[i].valid && ((unsigned int)tlb[i].virtualPage == vpn)) {
		entry = &tlb[i];			// 找到！
		break;
	    }
	if (entry == NULL) {				// 未找到
    	    DEBUG('a', "*** 未找到此虚拟页的有效 TLB 条目!\n");
    	    return PageFaultException;		// 实际上，这是一个 TLB 故障，
						// 页可能在内存中，
						// 但不在 TLB 中
	}
    }

    if (entry->readOnly && writing) {	// 尝试写入只读页
	DEBUG('a', "%d 在 TLB 中映射为只读的 %d!\n", virtAddr, i);
	return ReadOnlyException;
    }
    pageFrame = entry->physicalPage;

    // 如果 pageFrame 太大，说明有问题！ 
    // 无效的转换被加载到页表或 TLB 中。 
    if (pageFrame >= NumPhysPages) { 
	DEBUG('a', "*** 帧 %d > %d!\n", pageFrame, NumPhysPages);
	return BusErrorException;
    }
    entry->use = TRUE;		// 设置使用和脏位
    if (writing)
	entry->dirty = TRUE;
    *physAddr = pageFrame * PageSize + offset;
    ASSERT((*physAddr >= 0) && ((*physAddr + size) <= MemorySize));
    DEBUG('a', "物理地址 = 0x%x\n", *physAddr);
    return NoException;
}
