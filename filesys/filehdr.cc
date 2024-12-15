// filehdr.cc 
//	管理磁盘文件头的例程（在 UNIX 中，这被称为 i-node）。
//
//	文件头用于定位文件数据在磁盘上的存储位置。我们将其实现为一个固定大小的指针表——表中的每个条目指向包含该部分文件数据的磁盘扇区
//	（换句话说，没有间接或双重间接块）。表的大小选择为文件头刚好足够放入一个磁盘扇区，
//
//	与真实系统不同，我们不在文件头中跟踪文件权限、所有权、最后修改日期等信息。
//
//	文件头可以通过两种方式初始化：
//	   对于新文件，通过修改内存中的数据结构
//	     指向新分配的数据块
//	   对于已经在磁盘上的文件，通过从磁盘读取文件头
//

#include "system.h"
#include "filehdr.h"

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	为新创建的文件初始化一个新的文件头。
//	从空闲磁盘块的映射中分配数据块给文件。
//	如果没有足够的空闲块来容纳新文件，则返回 FALSE。
//
//	"freeMap" 是空闲磁盘扇区的位图
//	"fileSize" 是文件大小
//----------------------------------------------------------------------

bool
FileHeader::Allocate(BitMap *freeMap, int fileSize)
{ 
    numBytes = fileSize;
    numSectors  = divRoundUp(fileSize, SectorSize);
    if (freeMap->NumClear() < numSectors)
	return FALSE;		// 空间不足

    for (int i = 0; i < numSectors; i++)
	dataSectors[i] = freeMap->Find();
    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	释放为此文件分配的所有数据块空间。
//
//	"freeMap" 是空闲磁盘扇区的位图
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(BitMap *freeMap)
{
    for (int i = 0; i < numSectors; i++) {
	ASSERT(freeMap->Test((int) dataSectors[i]));  // 应该被标记！
	freeMap->Clear((int) dataSectors[i]);
    }
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	从磁盘获取文件头的内容。
//
//	"sector" 是包含文件头的磁盘扇区
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	将修改后的文件头内容写回磁盘。
//
//	"sector" 是包含文件头的磁盘扇区
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	返回存储文件中特定字节的磁盘扇区。
//      这本质上是从虚拟地址（文件中的偏移量）到物理地址（存储该偏移量数据的扇区）的转换。
//
//	"offset" 是文件中该字节的位置
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
    return(dataSectors[offset / SectorSize]);
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	返回文件中的字节数。
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	打印文件头的内容，以及文件头指向的所有数据块的内容。
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    printf("文件头内容。文件大小: %d. 文件块:\n", numBytes);
    for (i = 0; i < numSectors; i++)
	printf("%d ", dataSectors[i]);
    printf("\n文件内容:\n");
    for (i = k = 0; i < numSectors; i++) {
	synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
	    if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
		printf("%c", data[j]);
            else
		printf("\\%x", (unsigned char)data[j]);
	}
        printf("\n"); 
    }
    delete [] data;
}
