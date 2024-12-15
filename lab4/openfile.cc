// lab4c
// 修改自lab4/openfile2.cc

#include "filehdr.h"
#include "openfile.h"
#include "system.h"

void OpenFile::WriteBack()
{
    hdr->WriteBack(this->sectorNumber); // 写回文件头扇区
    this->changed = false;
}

OpenFile::OpenFile(int sector)
{
    sectorNumber = sector;
    hdr = new FileHeader();
    hdr->FetchFrom(sector);
    seekPosition = 0;
}

OpenFile::~OpenFile()
{
    if (this->changed)
        this->WriteBack(); // 如果changed为true，写回文件头
    this->changed = false;
    delete hdr;
}

void OpenFile::Seek(int position)
{
    seekPosition = position;
}

int OpenFile::Read(char *into, int numBytes)
{
    int result = ReadAt(into, numBytes, seekPosition);
    seekPosition += result;
    return result;
}

int OpenFile::Write(char *into, int numBytes)
{
    int result = WriteAt(into, numBytes, seekPosition);
    seekPosition += result;
    return result;
}

int OpenFile::ReadAt(char *into, int numBytes, int position)
{
    int fileLength = hdr->FileLength();
    int i, firstSector, lastSector, numSectors;
    char *buf;

    if ((numBytes <= 0) || (position > fileLength))
        return 0;
    if ((position + numBytes) > fileLength)
    {
        numBytes = fileLength - position;
    }
    DEBUG('f', "正在读取 %d 字节，位置 %d，文件长度 %d.\n",
          numBytes, position, fileLength);

    firstSector = divRoundDown(position, SectorSize);
    lastSector = divRoundDown(position + numBytes - 1, SectorSize);
    numSectors = 1 + lastSector - firstSector;

    buf = new char[numSectors * SectorSize];
    for (i = firstSector; i <= lastSector; i++)
        synchDisk->ReadSector(hdr->ByteToSector(i * SectorSize),
                              &buf[(i - firstSector) * SectorSize]);

    bcopy(&buf[position - (firstSector * SectorSize)], into, numBytes);
    delete[] buf;
    return numBytes;
}

int OpenFile::WriteAt(char *from, int numBytes, int position)
{
    int fileLength = hdr->FileLength();
    int i, firstSector, lastSector, numSectors;
    bool firstAligned, lastAligned;
    char *buf;

    if ((numBytes <= 0) || (position > fileLength)) // 对于原始的 Nachos 文件系统
                                                    //    if ((numBytes <= 0) || (position > fileLength))  // 对于 lab4 ...
        return 0;                                   // 检查请求
    if ((position + numBytes) > fileLength)
        if (!hdr->ChangeFileSize(position + numBytes))
        // 修改文件大小
        {
            numBytes = fileLength - position;
        }
        else
        {
            this->changed = true;
        }

    DEBUG('f', "正在写入 %d 字节，位置 %d，文件长度 %d.\n",
          numBytes, position, fileLength);

    firstSector = divRoundDown(position, SectorSize);
    lastSector = divRoundDown(position + numBytes - 1, SectorSize);
    numSectors = 1 + lastSector - firstSector;

    buf = new char[numSectors * SectorSize];

    firstAligned = (bool)(position == (firstSector * SectorSize));
    lastAligned = (bool)((position + numBytes) == ((lastSector + 1) * SectorSize));

    if (!firstAligned)
        ReadAt(buf, SectorSize, firstSector * SectorSize);
    if (!lastAligned && ((firstSector != lastSector) || firstAligned))
        ReadAt(&buf[(lastSector - firstSector) * SectorSize],
               SectorSize, lastSector * SectorSize);

    bcopy(from, &buf[position - (firstSector * SectorSize)], numBytes);

    for (i = firstSector; i <= lastSector; i++)
        synchDisk->WriteSector(hdr->ByteToSector(i * SectorSize),
                               &buf[(i - firstSector) * SectorSize]);
    delete[] buf;
    SetModTime(time(NULL)); // 设置修改时间
    return numBytes;
}

int OpenFile::Length()
{
    return hdr->FileLength();
}

time_t OpenFile::GetModTime() // 获取最后修改时间
{
    return hdr->GetModTime();
}

void OpenFile::SetModTime(long modTime) // 设置最后修改时间
{
    hdr->SetModTime(modTime);
}
