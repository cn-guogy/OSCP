// ring.cc
//	实现生产者和消费者问题的环形缓冲区的例程。
//
// 版权所有 (c) 1995 昆士兰大学的校长和董事会。
// 保留所有权利。请参阅 copyright.h 以获取版权声明和责任限制
// 及免责声明条款。

extern "C"
{
#include <stdio.h>
    extern int exit(int st);
}

#include "ring.h"

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
// 	Ring类的构造函数。请注意，它没有返回类型。
//
// 	"sz" -- 环形缓冲区中同时存在的最大元素数量
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
    buffer = new slot[size]; // 分配一个槽的数组。
}

//----------------------------------------------------------------------
// Ring::~Ring
// 	Ring类的析构函数。只需释放在构造函数中分配的数组。
//----------------------------------------------------------------------

Ring::~Ring()
{
    // 一些编译器和书籍告诉你这样写：
    //     delete [size] stack;
    // 但显然G++不喜欢这样。

    delete[] buffer;
}

//----------------------------------------------------------------------
// Ring::Put
// 	将消息放入下一个可用的空槽中。我们假设调用者已完成必要的同步。
//
//	"message" -- 要放入缓冲区的消息
//----------------------------------------------------------------------

void Ring::Put(slot *message)
{
    buffer[in].thread_id = message->thread_id;
    buffer[in].value = message->value;
    in = (in + 1) % size;
}

//----------------------------------------------------------------------
// Ring::Get
// 	从下一个满槽中获取消息。我们假设调用者已完成必要的同步。
//
//	"message" -- 来自缓冲区的消息
//----------------------------------------------------------------------

void Ring::Get(slot *message)
{
    message->thread_id = buffer[out].thread_id;
    message->value = buffer[out].value;
    out = (out + 1) % size;
}

int Ring::Empty()
{
    return in == out;
}

int Ring::Full()
{
    return ((in + 1) % size) == out;
}
