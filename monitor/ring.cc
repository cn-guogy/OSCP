// ring.cc
//	实现生产者和消费者问题的环形缓冲区的例程。

extern "C"
{
#include <stdio.h>
#include <stdlib.h> // extern void exit(int st);
}

#include "ring.h"
#include "system.h"

//----------------------------------------------------------------------
// slot::slot
// 	slot类的构造函数。
//----------------------------------------------------------------------

slot::slot(int id, int number)
{
    thread_id = id;
    value = number;
}

//----------------------------------------------------------------------
// Ring::Ring
// 	Ring类的构造函数。注意它没有返回类型。
//
// 	"sz" -- 环形缓冲区中任何时候的最大元素数量
//----------------------------------------------------------------------

Ring::Ring(int sz)
{
    if (sz < 1)
    {
        fprintf(stderr, "错误: Ring: 大小 %d 太小\n", sz);
        exit(1);
    }

    // 初始化环形对象的数据成员。
    size = sz;
    in = 0;
    out = 0;
    current = 0;
    buffer = new slot[size]; // 分配一个槽的数组。

    // 初始化条件变量
    notfull = new Condition_H("notfull");
    notempty = new Condition_H("notempty");

    // 初始化监视器环的信号量
    mutex = new Semaphore("mutex", 1);
    next = new Semaphore("next", 0);
    next_count = 0;
}

//----------------------------------------------------------------------
// Ring::~Ring
// 	Ring类的析构函数。只需处理我们在构造函数中分配的数组。
//----------------------------------------------------------------------

Ring::~Ring()
{
    // 一些编译器和书籍告诉你这样写：
    //     delete [size] stack;
    // 但显然G++不喜欢这样。

    delete[] buffer;

    delete notfull;
    delete notempty;

    delete mutex;
    delete next;
}

//----------------------------------------------------------------------
// Ring::Put
// 	将消息放入下一个可用的空槽中。我们假设调用者已完成必要的同步。
//
//	"message" -- 要放入缓冲区的消息
//----------------------------------------------------------------------

void Ring::Put(slot *message)
{

    mutex->P();

    if (current == size)
    {

        notfull->Wait(mutex, next, &next_count);
    }

    buffer[in].thread_id = message->thread_id;
    buffer[in].value = message->value;
    current++;
    in = (in + 1) % size;

    notempty->Signal(next, &next_count);

    if (next_count > 0)
        next->V();
    else
        mutex->V();
}

//----------------------------------------------------------------------
// Ring::Get
// 	从下一个满槽中获取消息。我们假设调用者已完成必要的同步。
//
//	"message" -- 来自缓冲区的消息
//----------------------------------------------------------------------

void Ring::Get(slot *message)
{

    mutex->P();

    if (current == 0)
    {

        notempty->Wait(mutex, next, &next_count);
    }

    message->thread_id = buffer[out].thread_id;
    message->value = buffer[out].value;
    current--;
    out = (out + 1) % size;

    notfull->Signal(next, &next_count);

    if (next_count > 0)
        next->V();
    else
        mutex->V();
}

int Ring::Empty()
{
    return 0; // 待实现
}

int Ring::Full()
{
    return 0; // 待实现
}
