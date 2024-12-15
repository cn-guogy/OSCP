#ifndef LIST_H
#define LIST_H

#include "utility.h"

// 以下类定义了一个 "列表元素" -- 用于
// 跟踪列表中的一个项目。它相当于一个
// LISP 单元，"car" ("next") 指向列表中的下一个元素，
// "cdr" ("item") 指向列表中的项目。
//
// 内部数据结构保持公共，以便列表操作可以
// 直接访问它们。

class ListElement {
   public:
     ListElement(void *itemPtr, int sortKey);	// 初始化一个列表元素

     ListElement *next;		// 列表中的下一个元素， 
				// 如果这是最后一个则为 NULL
     int key;		    	// 优先级，用于排序列表
     void *item; 	    	// 指向列表中项目的指针
};

// 以下类定义了一个 "列表" -- 一个单链表，由
// 列表元素组成，每个元素指向列表中的单个项目。
//
// 通过使用 "Sorted" 函数，列表可以保持
// 按 "key" 在 ListElement 中递增排序。

class List {
  public:
    List();			// 初始化列表
    ~List();			// 释放列表

    void Prepend(void *item); 	// 将项目放在列表的开头
    void Append(void *item); 	// 将项目放在列表的末尾
    void *Remove(); 	 	// 从列表前面移除项目

    void Mapcar(VoidFunctionPtr func);	// 将 "func" 应用到每个元素 
					// 在列表上
    bool IsEmpty();		// 列表是否为空？ 
    

    // 按顺序放入/取出列表中的项目的例程（按键排序）
    void SortedInsert(void *item, int sortKey);	// 将项目放入列表
    void *SortedRemove(int *keyPtr); 	  	// 从列表中移除第一个项目

  private:
    ListElement *first;  	// 列表的头部，如果列表为空则为 NULL
    ListElement *last;		// 列表的最后一个元素
};

#endif // LIST_H
