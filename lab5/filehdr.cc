// filehdr.cc 更改自lab4/filehdr.cc
// 获取二级索引
// 设置二级索引
// 获取盘号
// 更改文件大小时，判断是否使用二级索引
// 分配空间
// 回收空间
// print

#include "system.h"
#include "filehdr.h"
#include "disk.h"

// 整数转布尔数组
// bool * Getpermission(){
//     int permission = this->permission;
//     bool *permissions = new bool[3];
//     for (int i = 0; i < 3; i++)
//     {
//         permissions[i] = permission % 2;
//         permission /= 2;
//     }
//     return permissions;
// }

// 更改文件大小
bool FileHeader::ChangeFileSize(int fileSize)
{
    // 空闲扇区
    BitMap *freeMap = new BitMap(NumSectors);
    OpenFile *bitMapFile = new OpenFile(0);
    freeMap->FetchFrom(bitMapFile);
    int numSectors = this->numSectors();
    int numSectorsSet = divRoundUp(fileSize, SectorSize);

    // 空间不足
    if (freeMap->NumClear() < (numSectorsSet - numSectors))
        return false;

    this->numBytes = fileSize;

    // 分配空闲扇区
    if (this->ifIndirect())
        this->indirectSector = freeMap->Find();
    int *indirect = this->GetIndirect();
    for (numSectors; numSectors < numSectorsSet; numSectors++)
        if (numSectors < NumDirect)
        {
            dataSectors[numSectors] = freeMap->Find();
        }
        else
        {
            indirect[numSectors - NumDirect] = freeMap->Find();
        }
    this->SetIndirect(indirect);
    freeMap->WriteBack(bitMapFile);
    return true;
}

bool FileHeader::Allocate(BitMap *freeMap, int fileSize)
{
    int numSectors = this->numSectors();
    int numSectorsSet = divRoundUp(fileSize, SectorSize);

    // 空间不足
    if (freeMap->NumClear() < (numSectorsSet - numSectors))
        return false;

    this->numBytes = fileSize;

    // 分配空闲扇区
    if (this->ifIndirect()){
        printf("*****启用二级索引*****\n%d", this->indirectSector);
        this->indirectSector = freeMap->Find();}
    int *indirect = this->GetIndirect();
    for (numSectors; numSectors < numSectorsSet; numSectors++)
        if (numSectors < NumDirect)
        {
            dataSectors[numSectors] = freeMap->Find();
        }
        else
        {
            indirect[numSectors - NumDirect] = freeMap->Find();
        }
    this->SetIndirect(indirect);
    return true;
}

void FileHeader::Deallocate(BitMap *freeMap)
{
    int i = 0;
    int numSectors = this->numSectors();
    int *indirect = this->GetIndirect();
    for (i; i < numSectors; i++)
        // 一级索引
        if (i < NumDirect)
            freeMap->Clear((int)dataSectors[i]);
        // 二级索引
        else
            freeMap->Clear((int)indirect[i - NumDirect]);
    if (this->ifIndirect())
        freeMap->Clear((int)indirectSector);
    this->indirectSector = 0;
    return;
}

void FileHeader::Print()
{

    printf("文件头内容。文件大小: %d. 修改时间: %s. ", numBytes, ctime(&modTime)); // 增加修改时间按的输出

    printf("文件块:\n");
    int numSectors = this->numSectors();
    int *indirect = this->GetIndirect();
    for (int i = 0; i < numSectors; i++)
        if (i < NumDirect)
            printf("%d ", dataSectors[i]);
        else
            printf("%d ", indirect[i - NumDirect]);

    printf("\n文件内容:\n");
    int i, j, k;
    char *data = new char[SectorSize];
    for (i = k = 0; i < numSectors; i++)
    {
        if (i < NumDirect)
            synchDisk->ReadSector(dataSectors[i], data);
        else
            synchDisk->ReadSector(indirect[i - NumDirect], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++)
            if ('\040' <= data[j] && data[j] <= '\176')
                printf("%c", data[j]);
            else
                printf("\\%x", (unsigned char)data[j]);
        printf("\n");
    }
}

int FileHeader::ByteToSector(int offset)
{
    int sectorIndex = offset / SectorSize;
    if (sectorIndex < NumDirect)
        return dataSectors[sectorIndex];
    else
    {
        int *indirect = this->GetIndirect();
        return indirect[sectorIndex - NumDirect];
    }
}

void FileHeader::FetchFrom(int sectorNumber)
{
    synchDisk->ReadSector(sectorNumber, (char *)this);
}

void FileHeader::WriteBack(int sectorNumber)
{
    synchDisk->WriteSector(sectorNumber, (char *)this);
}

int FileHeader::FileLength()
{
    return numBytes;
}

// 设置修改时间
void FileHeader::SetModTime(long time)
{
    this->modTime = time;
}

// 获取修改时间
long FileHeader::GetModTime()
{
    return modTime;
}

bool FileHeader::ifIndirect()
{
    return (this->numSectors() > NumDirect && indirectSector == 0);
}

// 获取二级索引
int *FileHeader::GetIndirect()
{
    int *indirect = new int[NumIndirect];
    synchDisk->ReadSector(indirectSector, (char *)indirect);
    return indirect;
}

// 设置二级索引
void FileHeader::SetIndirect(int *indirect)
{
    if (this->indirectSector == 0)
        return;
    synchDisk->WriteSector(indirectSector, (char *)indirect);
}

// 计算得到文件块数
int FileHeader::numSectors()
{
    return divRoundUp(this->numBytes, SectorSize);
}

// 无参数构造函数
FileHeader::FileHeader()
{
    memset(dataSectors, 0, sizeof(dataSectors));
    indirectSector = 0;
    modTime = 0;
}