// directory.cc

#include "utility.h"
#include "filehdr.h"
#include "directory.h"

int Directory::GetAllFileSize()
{
    int size = 0;
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse)
            size += GetFileSize(i);
    return size;
}

int Directory::GetAllFileSectors()
{
    int sectors = 0;
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse)
            sectors += GetFileSectors(i);
    return sectors;
}

int Directory::GetFileSectors(int index)
{
    FileHeader *hdr = new FileHeader;
    hdr->FetchFrom(table[index].sector);
    int Sectors = hdr->numSectors();
    delete hdr;
    return Sectors;
}

int Directory::GetFileSize(int index)
{
    FileHeader *hdr = new FileHeader;
    hdr->FetchFrom(table[index].sector);
    int size = hdr->FileLength();
    delete hdr;
    return size;
}

int Directory::GetFileNum()
{
    int num = 0;
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse)
            num++;
    return num;
}

Directory::Directory(int size)
{
    table = new DirectoryEntry[size];
    tableSize = size;
    for (int i = 0; i < tableSize; i++)
        table[i].inUse = FALSE;
}

Directory::~Directory()
{
    delete[] table;
}

void Directory::FetchFrom(OpenFile *file)
{
    (void)file->ReadAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

void Directory::WriteBack(OpenFile *file)
{
    (void)file->WriteAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

int Directory::FindIndex(char *name)
{
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse && !strncmp(table[i].name, name, FileNameMaxLen))
            return i;
    return -1; // 名称不在目录中
}

int Directory::Find(char *name)
{
    int i = FindIndex(name);

    if (i != -1)
        return table[i].sector;
    return -1;
}

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

bool Directory::Remove(char *name)
{
    int i = FindIndex(name);

    if (i == -1)
        return FALSE; // 名称不在目录中
    table[i].inUse = FALSE;
    return TRUE;
}

void Directory::List()
{
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse)
            printf("%s\n", table[i].name);
}

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
