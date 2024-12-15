// lab5
// directory.h 修改自 filesys/directory.h
// 提供获取文件大小和盘区数目的方法

#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "openfile.h"

#define FileNameMaxLen 9

class DirectoryEntry
{
public:
  bool inUse;
  int sector;
  char name[FileNameMaxLen + 1];
};

class Directory
{
public:
  Directory(int size);
  ~Directory();

  void FetchFrom(OpenFile *file);
  void WriteBack(OpenFile *file);

  int Find(char *name);

  bool Add(char *name, int newSector);

  bool Remove(char *name);

  void List();
  void Print();

  int GetFileNum();
  int GetFileSectors(int index);
  int GetFileSize(int index);
  int GetAllFileSize();
  int GetAllFileSectors();

private:
  int tableSize;
  DirectoryEntry *table;

  int FindIndex(char *name);
};

#endif // DIRECTORY_H
