// addrspace.h修改自test/addresspacae.h
// 添加print方法

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "../machine/translate.h"
#include "filesys.h"

#define UserStackSize 1024 // 根据需要增加这个值！

class AddrSpace
{
public:
  AddrSpace(OpenFile *executable); // 创建一个地址空间，
                                   // 用存储在文件 "executable" 中的程序初始化它
  ~AddrSpace();                    // 释放地址空间

  void InitRegisters(); // 初始化用户级 CPU 寄存器，
                        // 在跳转到用户代码之前

  void SaveState();    // 保存/恢复地址空间特定的
  void RestoreState(); // 上下文切换的信息

  void print();

  int FindPageOut();

  OpenFile *CreateSwapFile(int pageSize);
  OpenFile *GetSwapFile() { return swapFile; }
  void WriteToSwap(int outPage);
  void GetPageToMem(int needPage);

  int getSpaceId() { return spaceId; }

private:
  TranslationEntry *pageTable; // 目前假设线性页表转换
                               // 现在就这样！
  unsigned int numPages;       // 虚拟地址空间中的页数
  int spaceId;
  OpenFile *swapFile;

  int *pageInMem; // 在内存中的页
  int idx;        // 记录最先进入内存的页
};

#endif // ADDRSPACE_H
