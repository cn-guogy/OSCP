// directory.cc
//	管理文件名目录的例程.
//
//	目录是一个固定长度条目的表；每个条目代表一个文件，包含文件名和文件头在磁盘上的位置。每个目录条目的固定大小意味着我们对文件名的最大大小有固定的限制.
//
//	构造函数初始化一个特定大小的空目录；我们使用 ReadFrom/WriteBack 从磁盘获取目录的内容，并将任何修改写回磁盘.
//
//	此外，这个实现有一个限制，即目录的大小不能扩展。换句话说，一旦目录中的所有条目都被使用，就不能再创建更多的文件。修复这个问题是作业的一部分.
//

#include "utility.h"
#include "filehdr.h"
#include "directory.h"

//----------------------------------------------------------------------
// Directory::Directory
// 	初始化一个目录；最初，目录是完全空的。如果磁盘正在格式化，一个空目录就足够了，但否则，我们需要调用 FetchFrom 来从磁盘初始化它。
//
//	"size" 是目录中条目的数量
//----------------------------------------------------------------------

Directory::Directory(int size)
{
    table = new DirectoryEntry[size];
    tableSize = size;
    for (int i = 0; i < tableSize; i++)
        table[i].inUse = FALSE;
}

//----------------------------------------------------------------------
// Directory::~Directory
// 	释放目录数据结构。
//----------------------------------------------------------------------

Directory::~Directory()
{
    delete[] table;
}

//----------------------------------------------------------------------
// Directory::FetchFrom
// 	从磁盘读取目录的内容。
//
//	"file" -- 包含目录内容的文件
//----------------------------------------------------------------------

void Directory::FetchFrom(OpenFile *file)
{
    (void)file->ReadAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::WriteBack
// 	将任何修改写回磁盘上的目录。
//
//	"file" -- 包含新目录内容的文件
//----------------------------------------------------------------------

void Directory::WriteBack(OpenFile *file)
{
    (void)file->WriteAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::FindIndex
// 	在目录中查找文件名，并返回其在目录条目表中的位置。如果名称不在目录中，则返回 -1。
//
//	"name" -- 要查找的文件名
//----------------------------------------------------------------------

int Directory::FindIndex(char *name)
{
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse && !strncmp(table[i].name, name, FileNameMaxLen))
            return i;
    return -1; // 名称不在目录中
}

//----------------------------------------------------------------------
// Directory::Find
// 	在目录中查找文件名，并返回文件头存储的磁盘扇区号。如果名称不在目录中，则返回 -1。
//
//	"name" -- 要查找的文件名
//----------------------------------------------------------------------

int Directory::Find(char *name)
{
    int i = FindIndex(name);

    if (i != -1)
        return table[i].sector;
    return -1;
}

//----------------------------------------------------------------------
// Directory::Add
// 	将文件添加到目录中。如果成功返回 TRUE；如果文件名已经在目录中，或者目录已满，没有更多空间容纳额外的文件名，则返回 FALSE。
//
//	"name" -- 要添加的文件名
//	"newSector" -- 包含添加文件头的磁盘扇区
//----------------------------------------------------------------------

bool Directory::Add(char *name, int newSector)
{
    if (FindIndex(name) != -1)
        return FALSE;

    for (int i = 0; i < tableSize; i++)
        if (!table[i].inUse)
        {
            table[i].inUse = TRUE;
            strncpy(table[i].name, name, FileNameMaxLen);
            table[i].sector = newSector;
            return TRUE;
        }
    return FALSE; // 没有空间。修复当我们有可扩展文件时。
}

//----------------------------------------------------------------------
// Directory::Remove
// 	从目录中删除文件名。如果成功返回 TRUE；如果文件不在目录中，则返回 FALSE。
//
//	"name" -- 要删除的文件名
//----------------------------------------------------------------------

bool Directory::Remove(char *name)
{
    int i = FindIndex(name);

    if (i == -1)
        return FALSE; // 名称不在目录中
    table[i].inUse = FALSE;
    return TRUE;
}

//----------------------------------------------------------------------
// Directory::List
// 	列出目录中的所有文件名。
//----------------------------------------------------------------------

void Directory::List()
{
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse)
            printf("%s\n", table[i].name);
}

//----------------------------------------------------------------------
// Directory::Print
// 	列出目录中的所有文件名、它们的 FileHeader 位置，以及每个文件的内容。用于调试。
//----------------------------------------------------------------------

void Directory::Print()
{
    FileHeader *hdr = new FileHeader;

    printf("目录内容:\n");
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse)
        {
            printf("名称: %s, 扇区: %d\n", table[i].name, table[i].sector);
            hdr->FetchFrom(table[i].sector);
            hdr->Print();
        }
    printf("\n");
    delete hdr;
}
