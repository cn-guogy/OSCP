// filehdr.h 
//	管理磁盘文件头的数据结构。  
//
//	文件头描述了在磁盘上找到文件数据的位置，
//	以及有关文件的其他信息（例如，它的
//	长度、所有者等。）
//



#ifndef FILEHDR_H
#define FILEHDR_H

#include "disk.h"
#include "bitmap.h"

#define NumDirect 	(int)((SectorSize - 2 * sizeof(int)) / sizeof(int))
#define MaxFileSize 	(NumDirect * SectorSize)

// 以下类定义了Nachos的“文件头”（在UNIX术语中，  
// “i-node”），描述了在磁盘上找到文件中所有数据的位置。
// 文件头组织为一个简单的指针表，指向
// 数据块。 
//
// 文件头数据结构可以存储在内存中或磁盘上。
// 当它在磁盘上时，它存储在一个单一的扇区中——这意味着
// 我们假设这个数据结构的大小与一个磁盘扇区相同。 
// 没有间接寻址，这
// 限制了最大文件长度为不到4K字节。
//
// 没有构造函数；而是文件头可以通过为文件分配块（
// 如果是新文件）或通过
// 从磁盘读取来初始化。

class FileHeader {
  public:
    bool Allocate(BitMap *bitMap, int fileSize);// 初始化文件头， 
						//  包括在磁盘上分配空间 
						//  用于文件数据
    void Deallocate(BitMap *bitMap);  		// 释放此文件的 
						//  数据块

    void FetchFrom(int sectorNumber); 	// 从磁盘初始化文件头
    void WriteBack(int sectorNumber); 	// 将修改写回文件头
					//  到磁盘

    int ByteToSector(int offset);	// 将字节偏移量转换为文件
					// 所在的磁盘扇区
					// 包含该字节

    int FileLength();			// 返回文件的长度 
					// 以字节为单位

    void Print();			// 打印文件的内容。

  private:
    int numBytes;			// 文件中的字节数
    int numSectors;			// 文件中的数据扇区数
    int dataSectors[NumDirect];		// 每个数据块在磁盘上的扇区号 
					// 在文件中
};

#endif // FILEHDR_H
