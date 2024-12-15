// synchlist.cc
//	用于同步访问列表的例程。
//
//	通过用同步例程包围 List 抽象来实现。
//
// 	以“监视器”风格实现 -- 用锁的获取和释放对包围每个过程，
// 	使用条件信号和等待进行同步。
//


#include "synchlist.h"

//----------------------------------------------------------------------
// SynchList::SynchList
//	分配和初始化同步列表所需的数据结构，
//	初始为空。
//	现在可以向列表中添加元素。
//----------------------------------------------------------------------

SynchList::SynchList()
{
    list = new List();
    lock = new Lock("list lock"); 
    listEmpty = new Condition("list empty cond");
}

//----------------------------------------------------------------------
// SynchList::~SynchList
//	释放为同步列表创建的数据结构。 
//----------------------------------------------------------------------

SynchList::~SynchList()
{ 
    delete list; 
    delete lock;
    delete listEmpty;
}

//----------------------------------------------------------------------
// SynchList::Append
//      将“item”附加到列表的末尾。唤醒任何等待
//	元素被附加的线程。
//
//	“item”是要放在列表上的东西，可以是指向 
//		任何东西的指针。
//----------------------------------------------------------------------

void
SynchList::Append(void *item)
{
    lock->Acquire();		// 强制对列表的互斥访问 
    list->Append(item);
    listEmpty->Signal(lock);	// 唤醒一个等待者（如果有的话）
    lock->Release();
}

//----------------------------------------------------------------------
// SynchList::Remove
//      从列表的开头移除一个“item”。如果列表为空则等待。
// 返回：
//	被移除的项。 
//----------------------------------------------------------------------

void *
SynchList::Remove()
{
    void *item;

    lock->Acquire();			// 强制互斥
    while (list->IsEmpty())
	listEmpty->Wait(lock);		// 等待直到列表不为空
    item = list->Remove();
    ASSERT(item != NULL);
    lock->Release();
    return item;
}

//----------------------------------------------------------------------
// SynchList::Mapcar
//      对列表上的每个项应用函数。遵守互斥
//	约束。
//
//	“func”是要应用的过程。
//----------------------------------------------------------------------

void
SynchList::Mapcar(VoidFunctionPtr func)
{ 
    lock->Acquire(); 
    list->Mapcar(func);
    lock->Release(); 
}
