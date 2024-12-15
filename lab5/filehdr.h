// 修改自filesys/filehdr.h

#ifndef FILEHDR_H
#define FILEHDR_H

#include "disk.h"
#include "bitmap.h"

#define NumDirect (int)((SectorSize - 1 * sizeof(long) - 2 * sizeof(int)) / sizeof(int))
#define NumIndirect (SectorSize / sizeof(int))
#define MaxSectorNum (NumDirect + NumIndirect)
#define MaxFileSize (MaxSectorNum * SectorSize)

class FileHeader
{
public:
  FileHeader(); // 构造函数初始化
  bool Allocate(BitMap *bitMap, int fileSize);
  void Deallocate(BitMap *bitMap);

  void FetchFrom(int sectorNumber);
  void WriteBack(int sectorNumber);

  int ByteToSector(int offset);

  int FileLength();

  void Print();
  bool ChangeFileSize(int newSize); // 更改文件大小
  void SetModTime(long time);       // 设置修改时间
  long GetModTime();                // 获取修改时间
  int numSectors();                 // 计算得到文件块数

  bool ifIndirect();     // 是否使用了两级索引
  int *GetIndirect();              // 获取二级索引
  void SetIndirect(int *indirect); // 设置二级索引

  //bool *GetPermission(); // 获取文件权限

private:
  long modTime;
  int numBytes;
  int dataSectors[NumDirect];
  int indirectSector;
 // int permission;
};

#endif
