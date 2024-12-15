// filesys.h
//	数据结构用于表示Nachos文件系统。
//
//	文件系统是一组存储在磁盘上的文件，组织成目录。对文件系统的操作与“命名”有关——创建、打开和删除文件，
//	给定一个文本文件名。对单个“打开”文件（读取、写入、关闭）的操作可以在OpenFile类（openfile.h）中找到。
//
//	我们定义了两个独立的文件系统实现。
//	“STUB”版本只是将Nachos文件系统操作重新定义为在运行Nachos模拟的机器上的本地UNIX文件系统的操作。
//	这是为了防止在文件系统作业完成之前，进行多程序和虚拟内存作业（使用文件系统）。
//
//	另一个版本是“真实”的文件系统，建立在磁盘模拟器之上。
//	磁盘是使用本地UNIX文件系统模拟的（在名为“DISK”的文件中）。
//
//	在“真实”实现中，文件系统中使用了两个关键数据结构。
//	有一个单一的“根”目录，列出文件系统中的所有文件；与UNIX不同，基线系统不提供分层目录结构。
//	此外，还有一个位图用于分配磁盘扇区。根目录和位图本身都作为文件存储在Nachos文件系统中——这在模拟磁盘初始化时造成了一个有趣的引导问题。
//

#ifndef FS_H
#define FS_H

#include "openfile.h"

#ifdef FILESYS_STUB // 暂时将文件系统调用实现为
                    // 对UNIX的调用，直到真实的文件系统
                    // 实现可用
class FileSystem
{
public:
  FileSystem(bool format) {}

  bool Create(char *name, int initialSize)
  {
    int fileDescriptor = OpenForWrite(name);

    if (fileDescriptor == -1)
      return FALSE;
    Close(fileDescriptor);
    return TRUE;
  }

  OpenFile *Open(char *name)
  {
    int fileDescriptor = OpenForReadWrite(name, FALSE);

    if (fileDescriptor == -1)
      return NULL;
    return new OpenFile(fileDescriptor);
  }

  bool Remove(char *name) { return (bool)(Unlink(name) == 0); }
};

#else // FILESYS
class FileSystem
{
public:
  FileSystem(bool format); // 初始化文件系统。
                           // 必须在“synchDisk”初始化后调用。
                           // 如果“format”，磁盘上没有任何内容，
                           // 因此初始化目录和空闲块的位图。

  bool Create(char *name, int initialSize);
  // 创建一个文件（UNIX creat）

  OpenFile *Open(char *name); // 打开一个文件（UNIX open）

  bool Remove(char *name); // 删除一个文件（UNIX unlink）

  void List(); // 列出文件系统中的所有文件

  void Print(); // 列出所有文件及其内容

private:
  OpenFile *freeMapFile; // 空闲磁盘块的位图，
                         // 作为文件表示
  OpenFile *directoryFile; // “根”目录——文件名列表，
                           // 作为文件表示
};

#endif // FILESYS

#endif // FS_H
