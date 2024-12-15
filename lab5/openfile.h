// 修改自filesys/openfile.h

#ifndef OPENFILE_H
#define OPENFILE_H
#include <time.h>
#include "utility.h"

class FileHeader;

class OpenFile
{
public:
  OpenFile(int sector);
  ~OpenFile();
  void WriteBack(); // 写回
  void Seek(int position);

  int Read(char *into, int numBytes);
  int Write(char *from, int numBytes);

  int ReadAt(char *into, int numBytes, int position);
  int WriteAt(char *from, int numBytes, int position);

  int Length();
  void SetModTime(time_t time); // 设置修改时间
  time_t GetModTime();          // 获取修改时间

private:
  FileHeader *hdr;
  int seekPosition;
  int sectorNumber;
  bool changed;
};

#endif
