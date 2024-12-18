// openfile.cc 
//	管理打开的 Nachos 文件的例程。与 UNIX 一样，
//	在我们可以读取或写入文件之前，文件必须被打开。
//	完成后，我们可以关闭它（在 Nachos 中，通过删除
//	OpenFile 数据结构）。
//
//	同样为了方便，我们在文件打开时将文件头保留在
//	内存中。
//


#include "filehdr.h"
#include "openfile.h"
#include "system.h"

//----------------------------------------------------------------------
// OpenFile::OpenFile
// 	打开一个 Nachos 文件以进行读取和写入。在文件打开时
//	将文件头加载到内存中。
//
//	"sector" -- 此文件的文件头在磁盘上的位置
//----------------------------------------------------------------------

OpenFile::OpenFile(int sector)
{ 
    hdr = new FileHeader;
    hdr->FetchFrom(sector);
    seekPosition = 0;
}

//----------------------------------------------------------------------
// OpenFile::~OpenFile
// 	关闭一个 Nachos 文件，释放任何在内存中的数据结构。
//----------------------------------------------------------------------

OpenFile::~OpenFile()
{
    delete hdr;
}

//----------------------------------------------------------------------
// OpenFile::Seek
// 	改变打开文件中的当前位置 -- 下一个读取或写入的起始点。
//
//	"position" -- 下一个读取/写入的文件位置
//----------------------------------------------------------------------

void
OpenFile::Seek(int position)
{
    seekPosition = position;
}	

//----------------------------------------------------------------------
// OpenFile::Read/Write
// 	读取/写入文件的一部分，从 seekPosition 开始。
//	返回实际写入或读取的字节数，并作为副作用，
//	增加文件中的当前位置信息。
//
//	使用更原始的 ReadAt/WriteAt 实现。
//
//	"into" -- 用于从磁盘读取数据的缓冲区 
//	"from" -- 包含要写入磁盘的数据的缓冲区 
//	"numBytes" -- 要传输的字节数
//----------------------------------------------------------------------

int
OpenFile::Read(char *into, int numBytes)
{
   int result = ReadAt(into, numBytes, seekPosition);
   seekPosition += result;
   return result;
}

int
OpenFile::Write(char *into, int numBytes)
{
   int result = WriteAt(into, numBytes, seekPosition);
   seekPosition += result;
   return result;
}

//----------------------------------------------------------------------
// OpenFile::ReadAt/WriteAt
// 	读取/写入文件的一部分，从 "position" 开始。
//	返回实际写入或读取的字节数，但没有副作用（当然，
//	Write 会修改文件）。
//
//	请求的开始或结束不保证在一个完整的磁盘扇区
//	边界上；然而，磁盘只知道如何一次读取/写入一个完整的磁盘
//	扇区。因此：
//
//	对于 ReadAt：
//	   我们读取所有完整或部分扇区，但只复制我们感兴趣的部分。
//	对于 WriteAt：
//	   我们必须首先读取任何将被部分写入的扇区，
//	   以便不覆盖未修改的部分。然后我们复制
//	   要修改的数据，并写回所有完整或部分扇区。
//
//	"into" -- 用于从磁盘读取数据的缓冲区 
//	"from" -- 包含要写入磁盘的数据的缓冲区 
//	"numBytes" -- 要传输的字节数
//	"position" -- 文件中第一个要读取/写入的字节的偏移量
//----------------------------------------------------------------------

int
OpenFile::ReadAt(char *into, int numBytes, int position)
{
    int fileLength = hdr->FileLength();
    int i, firstSector, lastSector, numSectors;
    char *buf;

    if ((numBytes <= 0) || (position >= fileLength))
    	return 0; 				// 检查请求
    if ((position + numBytes) > fileLength)		
	numBytes = fileLength - position;
    DEBUG('f', "正在读取 %d 字节，位置 %d，文件长度 %d.\n", 	
			numBytes, position, fileLength);

    firstSector = divRoundDown(position, SectorSize);
    lastSector = divRoundDown(position + numBytes - 1, SectorSize);
    numSectors = 1 + lastSector - firstSector;

    // 读取我们需要的所有完整和部分扇区
    buf = new char[numSectors * SectorSize];
    for (i = firstSector; i <= lastSector; i++)	
        synchDisk->ReadSector(hdr->ByteToSector(i * SectorSize), 
					&buf[(i - firstSector) * SectorSize]);

    // 复制我们想要的部分
    bcopy(&buf[position - (firstSector * SectorSize)], into, numBytes);
    delete [] buf;
    return numBytes;
}

int
OpenFile::WriteAt(char *from, int numBytes, int position)
{
    int fileLength = hdr->FileLength();
    int i, firstSector, lastSector, numSectors;
    bool firstAligned, lastAligned;
    char *buf;

    if ((numBytes <= 0) || (position >= fileLength))  // 对于原始的 Nachos 文件系统
//    if ((numBytes <= 0) || (position > fileLength))  // 对于 lab4 ...
	return 0;				// 检查请求
    if ((position + numBytes) > fileLength)
	numBytes = fileLength - position;
    DEBUG('f', "正在写入 %d 字节，位置 %d，文件长度 %d.\n", 	
			numBytes, position, fileLength);

    firstSector = divRoundDown(position, SectorSize);
    lastSector = divRoundDown(position + numBytes - 1, SectorSize);
    numSectors = 1 + lastSector - firstSector;

    buf = new char[numSectors * SectorSize];

    firstAligned = (bool)(position == (firstSector * SectorSize));
    lastAligned = (bool)((position + numBytes) == ((lastSector + 1) * SectorSize));

// 如果要部分修改，则读取第一个和最后一个扇区
    if (!firstAligned)
        ReadAt(buf, SectorSize, firstSector * SectorSize);	
    if (!lastAligned && ((firstSector != lastSector) || firstAligned))
        ReadAt(&buf[(lastSector - firstSector) * SectorSize], 
				SectorSize, lastSector * SectorSize);	

// 复制我们想要更改的字节 
    bcopy(from, &buf[position - (firstSector * SectorSize)], numBytes);

// 写回修改后的扇区
    for (i = firstSector; i <= lastSector; i++)	
        synchDisk->WriteSector(hdr->ByteToSector(i * SectorSize), 
					&buf[(i - firstSector) * SectorSize]);
    delete [] buf;
    return numBytes;
}

//----------------------------------------------------------------------
// OpenFile::Length
// 	返回文件中的字节数。
//----------------------------------------------------------------------

int
OpenFile::Length() 
{ 
    return hdr->FileLength(); 
}
