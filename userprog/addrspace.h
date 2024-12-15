// addrspace.h
//	用于跟踪正在执行的用户程序的数据结构
//	（地址空间）。
//
//	目前，我们不保留任何关于地址空间的信息。
//	用户级 CPU 状态在执行用户程序的线程中保存和恢复
//	（见 thread.h）。
//

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

private:
  TranslationEntry *pageTable; // 目前假设线性页表转换
                               // 现在就这样！
  unsigned int numPages;       // 虚拟地址空间中的页数
};

#endif // ADDRSPACE_H
