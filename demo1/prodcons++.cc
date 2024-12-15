// prodcons++.cc
//	C++ 版本的生产者和消费者问题，使用环形缓冲区。
//
//	创建 N_PROD 个生产者线程和 N_CONS 个消费者线程。
//	生产者和消费者线程通过共享的环形缓冲区对象进行通信。
//	对共享环形缓冲区的操作通过信号量进行同步。
//
//
// 版权所有 (c) 1995 昆士兰大学的校长和董事会。
// 保留所有权利。请参阅 copyright.h 以获取版权声明和责任限制
// 及免责声明条款。

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "system.h"

#include "synch.h"
#include "ring.h"

#define BUFF_SIZE 3 // 环形缓冲区的大小
#define N_PROD 2    // 生产者的数量
#define N_CONS 2    // 消费者的数量
#define N_MESSG 5   // 每个生产者生成的消息数量
#define MAX_NAME 16 // 名称的最大长度

#define MAXLEN 48
#define LINELEN 24

Thread *producers[N_PROD]; // 指向生产者的指针数组
Thread *consumers[N_CONS]; // 指向消费者线程的指针数组

char prod_names[N_PROD][MAX_NAME]; // 生产者名称的字符数组
char cons_names[N_CONS][MAX_NAME]; // 消费者名称的字符数组

Semaphore *nempty, *nfull; // 用于空槽和满槽的两个信号量
Semaphore *mutex;          // 用于互斥的信号量

Ring *ring;

//----------------------------------------------------------------------
// Producer
// 	循环 N_MESSG 次，每次生成一条消息并放入共享的
//      环形缓冲区。
//	"which" 只是一个标识生产者线程的数字。
//
//----------------------------------------------------------------------

void Producer(_int which)
{
    int num;
    slot *message = new slot(0, 0);

    //  此循环用于生成 N_MESSG 条消息并放入环形缓冲区
    //   通过调用 ring->Put(message)。每条消息携带一个消息 ID
    //   由整数 "num" 表示。该消息 ID 应放入槽的 "value" 字段。
    //   它还应携带生产者线程的 ID 存储在 "thread_id" 字段中，以便
    //   消费者线程可以知道哪个生产者生成了该消息。
    //   你需要在调用 ring->Put(message) 之前和之后放置同步代码。
    //   请参见教科书第 182 页的算法。

    for (num = 0; num < N_MESSG; num++)
    {
        message->thread_id = which;
        message->value = num;

        nempty->P();
        mutex->P();
        ring->Put(message);
        mutex->V();
        nfull->V();
    }
}

//----------------------------------------------------------------------
// Consumer
// 	无限循环从环形缓冲区获取消息并
//      将这些消息记录到相应的文件中。
//
//----------------------------------------------------------------------

void Consumer(_int which)
{
    char str[MAXLEN];
    char fname[LINELEN];
    int fd;

    slot *message = new slot(0, 0);

    // 为此消费者线程形成一个输出文件名。
    // 所有被此消费者接收的消息将记录在
    // 此文件中。
    sprintf(fname, "tmp_%d", which);

    // 创建一个文件。请注意，这是一个 UNIX 系统调用。
    if ((fd = creat(fname, 0600)) == -1)
    {
        perror("creat: 文件创建失败");
        exit(1);
    }

    for (;;)
    {
        nfull->P();
        mutex->P();
        ring->Get(message);
        mutex->V();
        nempty->V();

        // 形成一个字符串以记录消息
        sprintf(str, "生产者 ID --> %d; 消息编号 --> %d;\n", message->thread_id, message->value);
        // 将此字符串写入此消费者的输出文件中。
        // 请注意，这是另一个 UNIX 系统调用。
        if (write(fd, str, strlen(str)) == -1)
        {
            perror("write: 写入失败");
            exit(1);
        }
    }
}

//----------------------------------------------------------------------
// ProdCons
// 	为共享环形缓冲区设置信号量并
//	创建和分叉生产者和消费者线程
//----------------------------------------------------------------------

void ProdCons()
{
    int i;
    DEBUG('t', "进入 ProdCons");

    nempty = new Semaphore("nempty", BUFF_SIZE);
    nfull = new Semaphore("nfull", 0);
    mutex = new Semaphore("mutex", 1);

    ring = new Ring(BUFF_SIZE);

    // 创建和分叉 N_PROD 个生产者线程
    for (i = 0; i < N_PROD; i++)
    {
        // 此语句用于形成一个字符串，用作生产者 i 的名称。
        sprintf(prod_names[i], "producer_%d", i);

        // 放置代码以使用
        //     prod_names[i] 中的名称创建并分叉一个新的生产者线程
        //     整数 i 作为函数 "Producer" 的参数
        //  ...
        producers[i] = new Thread(prod_names[i]);
        producers[i]->Fork(Producer, i);
    }

    // 创建和分叉 N_CONS 个消费者线程
    for (i = 0; i < N_CONS; i++)
    {
        // 此语句用于形成一个字符串，用作消费者 i 的名称。
        sprintf(cons_names[i], "consumer_%d", i);
        // 放置代码以使用
        //     cons_names[i] 中的名称创建并分叉一个新的消费者线程
        //     整数 i 作为函数 "Consumer" 的参数
        //  ...
        consumers[i] = new Thread(cons_names[i]);
        consumers[i]->Fork(Consumer, i);
    };
}
