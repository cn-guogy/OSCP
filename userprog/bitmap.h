// bitmap.h 
//	定义位图的数据结构 -- 一个每个位可以是开或关的位数组。
//
//	表示为一个无符号整数数组，我们对其进行
//	模运算以找到我们感兴趣的位。
//
//	位图可以根据管理的位数进行参数化。

#ifndef BITMAP_H
#define BITMAP_H


#include "utility.h"
#include "openfile.h"

// 有助于将位图表示为整数数组的定义
#define BitsInByte 	8
#define BitsInWord 	32

// 以下类定义了一个“位图” -- 一个位数组，
// 每个位可以独立设置、清除和测试。
//
// 最常用于管理数组元素的分配 --
// 例如，磁盘扇区或主内存页面。
// 每个位表示相应的扇区或页面是
// 正在使用还是空闲。

class BitMap {
  public:
    BitMap(int nitems);		// 初始化一个位图，具有“nitems”位
				// 初始时，所有位都被清除。
    ~BitMap();			// 释放位图
    
    void Mark(int which);   	// 设置“第n”位
    void Clear(int which);  	// 清除“第n”位
    bool Test(int which);   	// “第n”位是否被设置？
    int Find();            	// 返回一个清除位的编号，并作为副作用
				// 设置该位。
				// 如果没有清除位，返回-1。
    int NumClear();		// 返回清除位的数量

    void Print();		// 打印位图的内容
    
    // 这些在FILESYS之前不需要，当我们需要读取和 
    // 写入位图到文件时
    void FetchFrom(OpenFile *file); 	// 从磁盘获取内容 
    void WriteBack(OpenFile *file); 	// 将内容写回磁盘

  private:
    int numBits;			// 位图中的位数
    int numWords;			// 位图存储的字数
					// （如果numBits不是
					//  位数的倍数，则向上取整）
    unsigned int *map;			// 位存储
};

#endif // BITMAP_H
