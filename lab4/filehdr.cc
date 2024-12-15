// filehdr.cc
//	拓展文件大小
// 初始化分配空间
#include "system.h"
#include "filehdr.h"
#include "disk.h"

int FileHeader::numSectors()
{
    return divRoundUp(this->numBytes, SectorSize);
}

// 无参数构造函数
FileHeader::FileHeader()
{
    memset(dataSectors, 0, sizeof(dataSectors));
    modTime = 0;
}

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
    for (numSectors; numSectors < numSectorsSet; numSectors++)
        dataSectors[numSectors] = freeMap->Find();
    freeMap->WriteBack(bitMapFile);

    return true;
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

bool FileHeader::Allocate(BitMap *freeMap, int fileSize)
{
    int numSectors = this->numSectors();
    int numSectorsSet = divRoundUp(fileSize, SectorSize);

    // 空间不足
    if (freeMap->NumClear() < (numSectorsSet - numSectors))
        return false;

    this->numBytes = fileSize;

    // 分配空闲扇区
    for (numSectors; numSectors < numSectorsSet; numSectors++)
        dataSectors[numSectors] = freeMap->Find();
    return true;
}

void FileHeader::Deallocate(BitMap *freeMap)
{
    int i = 0;
    int numSectors = this->numSectors();
    for (i; i < numSectors; i++)
        freeMap->Clear((int)dataSectors[i]);
    return;
}

void FileHeader::FetchFrom(int sectorNumber)
{
    synchDisk->ReadSector(sectorNumber, (char *)this);
}

void FileHeader::WriteBack(int sectorNumber)
{
    synchDisk->WriteSector(sectorNumber, (char *)this);
}

int FileHeader::ByteToSector(int offset)
{
    return (dataSectors[offset / SectorSize]);
}

int FileHeader::FileLength()
{
    return numBytes;
}

void FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];
    int numSectors = this->numSectors();
    printf("文件头内容。文件大小: %d. 修改时间: %s.", numBytes, ctime(&modTime)); // 增加修改时间按的输出

    printf("文件块号: \n");
    for (i = 0; i < numSectors; i++)
        printf("%d ", dataSectors[i]);

    printf("\n文件内容:\n");
    for (i = k = 0; i < numSectors; i++)
    {
        synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++)
        {
            if ('\040' <= data[j] && data[j] <= '\176') // isprint(data[j])
                printf("%c", data[j]);
            else
                printf("\\%x", (unsigned char)data[j]);
        }
        printf("\n");
    }
    delete[] data;
}
