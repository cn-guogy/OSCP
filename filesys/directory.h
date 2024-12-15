// directory.h 
//	管理类似 UNIX 的文件名目录的数据结构。
// 
//      目录是一个对的表：<文件名, 扇区号>，
//	给出目录中每个文件的名称，以及 
//	在磁盘上找到其文件头的位置（描述
//	如何找到文件数据块的数据结构）。
//
//      我们假设调用者提供了互斥访问。
//



#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "openfile.h"

#define FileNameMaxLen 		9	// 为了简单起见，我们假设 
					// 文件名长度 <= 9 个字符

// 以下类定义了一个“目录条目”，表示目录中的一个文件。
// 每个条目给出文件的名称，以及在磁盘上找到
// 文件头的位置。
//
// 内部数据结构保持公共，以便目录操作可以
// 直接访问它们。

class DirectoryEntry {
  public:
    bool inUse;				// 该目录条目是否正在使用？
    int sector;				// 在磁盘上找到该文件的 
					//   文件头的位置 
    char name[FileNameMaxLen + 1];	// 文件的文本名称，+1 用于 
					// 结尾的 '\0'
};

// 以下类定义了一个类似 UNIX 的“目录”。每个条目在
// 目录中描述一个文件，以及在磁盘上找到它的位置。
//
// 目录数据结构可以存储在内存中或磁盘上。
// 当它在磁盘上时，它作为常规的 Nachos 文件存储。
//
// 构造函数在内存中初始化一个目录结构；
// FetchFrom/WriteBack 操作将目录信息
// 从/到磁盘传输。

class Directory {
  public:
    Directory(int size); 		// 初始化一个空目录
					// 以容纳 "size" 个文件
    ~Directory();			// 释放目录

    void FetchFrom(OpenFile *file);  	// 从磁盘初始化目录内容
    void WriteBack(OpenFile *file);	// 将修改写回 
					// 目录内容到磁盘

    int Find(char *name);		// 查找文件的扇区号： 
					// "name" 的文件头

    bool Add(char *name, int newSector);  // 将文件名添加到目录中

    bool Remove(char *name);		// 从目录中移除文件

    void List();			// 打印目录中所有文件的名称
					//  
    void Print();			// 详细打印目录的内容
					//  -- 所有文件的名称及其内容。

  private:
    int tableSize;			// 目录条目的数量
    DirectoryEntry *table;		// 对的表： 
					// <文件名, 文件头位置> 

    int FindIndex(char *name);		// 查找与 "name" 
					// 对应的目录表索引
};

#endif // DIRECTORY_H
