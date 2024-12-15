// filesys.cc
// 实现了printinfo

#include "disk.h"
#include "bitmap.h"
#include "directory.h"
#include "filehdr.h"
#include "filesys.h"

#define FreeMapSector 0
#define DirectorySector 1

#define FreeMapFileSize (NumSectors / BitsInByte)
#define NumDirEntries 10
#define DirectoryFileSize (sizeof(DirectoryEntry) * NumDirEntries)


// 输出文件系统信息
void FileSystem::PrintInfo()
{
    BitMap *freeMap = new BitMap(NumSectors);
    freeMap->FetchFrom(freeMapFile);
    int freeSectorsNum = freeMap->NumClear();
    Directory *directory = new Directory(NumDirEntries);
    directory->FetchFrom(directoryFile);
    int fileNum = directory->GetFileNum();
    int fileSize = directory->GetAllFileSize();
    int fileSectors = directory->GetAllFileSectors();
    printf("文件系统信息：\n");
    printf("总磁盘：%d 盘区、 %d 字节\n", NumSectors, NumSectors * SectorSize);
    printf("已使用：%d 盘区、 %d 字节\n", NumSectors - freeSectorsNum, (NumSectors - freeSectorsNum) * SectorSize);
    printf("未使用：%d 盘区、 %d 字节\n", freeSectorsNum, freeSectorsNum * SectorSize);
    printf("普通文件数目：%d\n", fileNum);
    printf("普通文件字节数：%d\n", fileSize);
    printf("普通文件占用的空间大小：%d\n", fileSectors * SectorSize);
    printf("内碎片字节数：%d\n", fileSectors * SectorSize - fileSize);
}

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

void FileSystem::List()
{
    Directory *directory = new Directory(NumDirEntries);

    directory->FetchFrom(directoryFile);
    directory->List();
    delete directory;
}

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
