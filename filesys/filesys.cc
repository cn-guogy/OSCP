// filesys.cc
//	管理文件系统整体操作的例程。
//	实现从文本文件名到文件的映射例程。
//
//	文件系统中的每个文件都有：
//	   存储在磁盘扇区中的文件头
//		（文件头数据结构的大小正好是1个磁盘扇区的大小）
//	   一些数据块
//	   文件系统目录中的一个条目
//
// 	文件系统由几个数据结构组成：
//	   空闲磁盘扇区的位图（参见 bitmap.h）
//	   文件名和文件头的目录
//
//      位图和目录都表示为普通文件。它们的文件头位于特定的扇区
//	（扇区0和扇区1），以便文件系统在启动时可以找到它们。
//
//	文件系统假设位图和目录文件在Nachos运行时持续“打开”。
//
//	对于那些修改目录和/或位图的操作（如创建、删除），如果操作成功，
//	更改会立即写回磁盘（在此期间这两个文件保持打开状态）。如果操作失败，
//	并且我们修改了部分目录和/或位图，我们只需丢弃更改的版本，
//	而不将其写回磁盘。
//
// 	我们目前的实现有以下限制：
//
//	   没有对并发访问的同步
//	   文件具有固定大小，在创建文件时设置
//	   文件不能大于约3KB
//	   没有层次目录结构，并且只能向系统添加有限数量的文件
//	   没有尝试使系统对故障具有鲁棒性
//	    （如果Nachos在修改文件系统的操作中途退出，可能会损坏磁盘）
//

#include "disk.h"
#include "bitmap.h"
#include "directory.h"
#include "filehdr.h"
#include "filesys.h"

// 包含空闲扇区位图和文件目录的文件头的扇区。
// 这些文件头放置在众所周知的扇区中，以便在启动时可以找到它们。
#define FreeMapSector 0
#define DirectorySector 1

// 位图和目录的初始文件大小；在文件系统支持可扩展文件之前，
// 目录大小设置可以加载到磁盘上的最大文件数量。
#define FreeMapFileSize (NumSectors / BitsInByte)
#define NumDirEntries 10
#define DirectoryFileSize (sizeof(DirectoryEntry) * NumDirEntries)

//----------------------------------------------------------------------
// FileSystem::FileSystem
// 	初始化文件系统。如果format = TRUE，磁盘上没有任何内容，
//	我们需要初始化磁盘以包含一个空目录和一个空闲扇区的位图（几乎所有扇区都标记为可用）。
//
//	如果format = FALSE，我们只需打开表示位图和目录的文件。
//
//	"format" -- 我们是否应该初始化磁盘？
//----------------------------------------------------------------------

FileSystem::FileSystem(bool format)
{
    DEBUG('f', "正在初始化文件系统。\n");
    if (format)
    {
        BitMap *freeMap = new BitMap(NumSectors);            // 创建文件位图
        Directory *directory = new Directory(NumDirEntries); // 创建包含10个文件目录项的文件目录表
        FileHeader *mapHdr = new FileHeader;                 // 创建文件位图的文件头
        FileHeader *dirHdr = new FileHeader;                 // 创建文件目录的文件头
        DEBUG('f', "正在格式化文件系统。\n");
        // 首先，为目录和位图的文件头分配空间
        // （确保没有其他人抢占这些空间！）
        freeMap->Mark(FreeMapSector);   // 0号扇区被文件位图文件头占用
        freeMap->Mark(DirectorySector); // 1号扇区被文件目录的文件头占用
        // 其次，为包含目录和位图文件内容的数据块分配空间。必须有足够的空间！
        // 分配空间
        ASSERT(mapHdr->Allocate(freeMap, FreeMapFileSize));
        ASSERT(dirHdr->Allocate(freeMap, DirectoryFileSize));
        // 将位图和目录的文件头刷新回磁盘
        // 我们需要在“打开”文件之前执行此操作，因为打开
        // 会从磁盘读取文件头（而当前磁盘上有垃圾数据）。
        DEBUG('f', "正在将文件头写回磁盘。\n");
        // openfile中存储文件头和文件读写位置 初始为0
        mapHdr->WriteBack(FreeMapSector);
        dirHdr->WriteBack(DirectorySector);
        // 现在可以打开位图和目录文件
        // 文件系统操作假设这两个文件在Nachos运行时保持打开状态。
        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);
        // 一旦我们打开了文件，就可以将每个文件的初始版本
        // 写回磁盘。此时目录是完全空的；但位图已更改以反映
        // 磁盘上的扇区已分配给文件头，并保存目录和位图的文件数据。
        DEBUG('f', "正在将位图和目录写回磁盘。\n");
        freeMap->WriteBack(freeMapFile); // 刷新更改到磁盘
        directory->WriteBack(directoryFile);
        if (DebugIsEnabled('f'))
        {
            freeMap->Print();
            directory->Print();
        }
        delete freeMap;
        delete directory;
        delete mapHdr;
        delete dirHdr;
    }
    else
    {
        // 如果我们不格式化磁盘，只需打开表示
        // 位图和目录的文件；这些文件在Nachos运行时保持打开
        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);
    }
}

//----------------------------------------------------------------------
// FileSystem::Create
// 	在Nachos文件系统中创建一个文件（类似于UNIX创建）。
//	由于我们无法动态增加文件的大小，必须给Create提供文件的初始大小。
//
//	创建文件的步骤是：
//	  确保文件不存在
//        为文件头分配一个扇区
// 	  在磁盘上为文件的数据块分配空间
//	  将名称添加到目录中
//	  将新的文件头存储到磁盘上
//	  将更改刷新到位图和目录回到磁盘
//
//	如果一切正常返回TRUE，否则返回FALSE。
//
// 	创建失败的情况：
//   		文件已在目录中
//	 	没有可用的文件头空间
//	 	目录中没有可用的文件条目
//	 	没有可用的文件数据块空间
//
// 	请注意，此实现假设没有对文件系统的并发访问！
//
//	"name" -- 要创建的文件名
//	"initialSize" -- 要创建的文件大小
//----------------------------------------------------------------------

bool FileSystem::Create(char *name, int initialSize)
{
    Directory *directory;
    BitMap *freeMap;
    FileHeader *hdr;
    int sector;
    bool success;

    DEBUG('f', "正在创建文件 %s, 大小 %d\n", name, initialSize);

    directory = new Directory(NumDirEntries);
    directory->FetchFrom(directoryFile);

    if (directory->Find(name) != -1)
        success = FALSE; // 文件已在目录中
    else
    {
        freeMap = new BitMap(NumSectors);
        freeMap->FetchFrom(freeMapFile);
        sector = freeMap->Find(); // 找到一个扇区来保存文件头
        if (sector == -1)
            success = FALSE; // 没有可用的文件头块
        else if (!directory->Add(name, sector))
            success = FALSE; // 目录中没有空间
        else
        {
            hdr = new FileHeader;
            if (!hdr->Allocate(freeMap, initialSize))
                success = FALSE; // 磁盘上没有空间用于数据
            else
            {
                success = TRUE;
                // 一切正常，将所有更改刷新回磁盘
                hdr->WriteBack(sector);
                directory->WriteBack(directoryFile);
                freeMap->WriteBack(freeMapFile);
            }
            delete hdr;
        }
        delete freeMap;
    }
    delete directory;
    return success;
}

//----------------------------------------------------------------------
// FileSystem::Open
// 	打开一个文件以进行读写。
//	要打开一个文件：
//	  使用目录查找文件头的位置
//	  将头部加载到内存中
//
//	"name" -- 要打开的文件的文本名称
//----------------------------------------------------------------------

OpenFile *
FileSystem::Open(char *name)
{
    Directory *directory = new Directory(NumDirEntries);
    OpenFile *openFile = NULL;
    int sector;

    DEBUG('f', "正在打开文件 %s\n", name);
    directory->FetchFrom(directoryFile);
    sector = directory->Find(name);
    if (sector >= 0)
        openFile = new OpenFile(sector); // 在目录中找到了名称
    delete directory;
    return openFile; // 如果未找到则返回NULL
}

//----------------------------------------------------------------------
// FileSystem::Remove
// 	从文件系统中删除一个文件。这需要：
//	    从目录中删除它
//	    删除其头部的空间
//	    删除其数据块的空间
//	    将更改写入目录，位图回到磁盘
//
//	如果文件被删除则返回TRUE，如果文件不在文件系统中则返回FALSE。
//
//	"name" -- 要删除的文件的文本名称
//----------------------------------------------------------------------

bool FileSystem::Remove(char *name)
{
    Directory *directory;
    BitMap *freeMap;
    FileHeader *fileHdr;
    int sector;

    directory = new Directory(NumDirEntries);
    directory->FetchFrom(directoryFile);
    sector = directory->Find(name);
    if (sector == -1)
    {
        delete directory;
        return FALSE; // 文件未找到
    }
    fileHdr = new FileHeader;
    fileHdr->FetchFrom(sector);

    freeMap = new BitMap(NumSectors);
    freeMap->FetchFrom(freeMapFile);

    fileHdr->Deallocate(freeMap); // 删除数据块
    freeMap->Clear(sector);       // 删除头部块
    directory->Remove(name);

    freeMap->WriteBack(freeMapFile);     // 刷新到磁盘
    directory->WriteBack(directoryFile); // 刷新到磁盘
    delete fileHdr;
    delete directory;
    delete freeMap;
    return TRUE;
}

//----------------------------------------------------------------------
// FileSystem::List
// 	列出文件系统目录中的所有文件。
//----------------------------------------------------------------------

void FileSystem::List()
{
    Directory *directory = new Directory(NumDirEntries);

    directory->FetchFrom(directoryFile);
    directory->List();
    delete directory;
}

//----------------------------------------------------------------------
// FileSystem::Print
// 	打印文件系统的所有信息：
//	  位图的内容
//	  目录的内容
//	  对于目录中的每个文件，
//	      文件头的内容
//	      文件中的数据
//----------------------------------------------------------------------

void FileSystem::Print()
{
    FileHeader *bitHdr = new FileHeader;
    FileHeader *dirHdr = new FileHeader;
    BitMap *freeMap = new BitMap(NumSectors);
    Directory *directory = new Directory(NumDirEntries);

    printf("位图文件头:\n");
    bitHdr->FetchFrom(FreeMapSector);
    bitHdr->Print();

    printf("目录文件头:\n");
    dirHdr->FetchFrom(DirectorySector);
    dirHdr->Print();

    freeMap->FetchFrom(freeMapFile);
    freeMap->Print();

    directory->FetchFrom(directoryFile);
    directory->Print();

    delete bitHdr;
    delete dirHdr;
    delete freeMap;
    delete directory;
}
