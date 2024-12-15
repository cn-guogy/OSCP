// prodcons++.cc
//	C++ 版本的生产者和消费者问题，使用环形缓冲区。
//
//	创建 N_PROD 个生产者线程和 N_CONS 个消费者线程。
//	生产者和消费者线程通过共享的
//      环形缓冲区对象进行通信。对共享环形缓冲区的操作
//      通过信号量进行同步。
//
//

#include <stdio.h>

// 添加这些包含以满足新的 Linux 环境
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include "system.h"

#include "synch.h"
#include "ring.h"

#define BUFF_SIZE 2 // 环形缓冲区的大小
#define N_PROD 2    // 生产者的数量
#define N_CONS 2    // 消费者的数量
#define N_MESSG 3   // 每个生产者生成的消息数量
#define MAX_NAME 16 // 名称的最大长度

#define MAXLEN 48
#define LINELEN 24

Thread *producers[N_PROD]; // 指向生产者的指针数组
Thread *consumers[N_CONS]; // 和消费者线程的指针数组;

char prod_names[N_PROD][MAX_NAME]; // 生产者名称的字符数组
char cons_names[N_CONS][MAX_NAME]; // 消费者名称的字符数组

Semaphore *nempty, *nfull; // 两个信号量用于空槽和满槽
Semaphore *mutex;          // 互斥信号量

Ring *ring;

//----------------------------------------------------------------------
// 生产者
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
    // 在这里放置准备消息的代码。
    // ...
    message->thread_id = which;
    message->value = num;

    ring->Put(message);

    currentThread->Yield();
  }
}

//----------------------------------------------------------------------
// 消费者
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
  // 这个文件中。
  sprintf(fname, "tmp_%d", which);

  // 创建一个文件。请注意，这是一个 UNIX 系统调用。
  if ((fd = creat(fname, 0600)) == -1)
  {
    perror("creat: 文件创建失败");
    exit(1);
  }

  for (;;)
  {

    ring->Get(message);

    // 形成一个字符串以记录消息
    sprintf(str, "生产者 ID --> %d; 消息编号 --> %d;\n",
            message->thread_id,
            message->value);
    // 将这个字符串写入此消费者的输出文件中。
    // 请注意，这是另一个 UNIX 系统调用。
    if (write(fd, str, strlen(str)) == -1)
    {
      perror("write: 写入失败");
      exit(1);
    }
    currentThread->Yield();
  }
}

//----------------------------------------------------------------------
// ProdCons
// 	为共享环形缓冲区设置信号量，并
//	创建和分叉生产者和消费者线程
//----------------------------------------------------------------------

void ProdCons()
{
  int i;
  DEBUG('t', "进入 ProdCons");

  // 在这里放置构造所有信号量的代码。
  // ....
  nempty = new Semaphore("nempty", BUFF_SIZE);
  nfull = new Semaphore("nfull", 0);
  mutex = new Semaphore("mutex", 1);

  // 在这里放置构造大小为 BUFF_SIZE 的环形缓冲区对象的代码。
  // ...
  ring = new Ring(BUFF_SIZE);

  // 创建和分叉 N_PROD 个生产者线程
  for (i = 0; i < N_PROD; i++)
  {
    // 此语句用于形成一个字符串，以用作
    // 生产者 i 的名称。
    sprintf(prod_names[i], "生产者_%d", i);

    // 在这里放置使用 prod_names[i] 中的名称和
    // 整数 i 作为 "Producer" 函数参数的代码
    //  ...
    producers[i] = new Thread(prod_names[i]);
    producers[i]->Fork(Producer, i);
  };

  // 创建和分叉 N_CONS 个消费者线程
  for (i = 0; i < N_CONS; i++)
  {
    // 此语句用于形成一个字符串，以用作
    // 消费者 i 的名称。
    sprintf(cons_names[i], "消费者_%d", i);
    // 在这里放置使用 cons_names[i] 中的名称和
    // 整数 i 作为 "Consumer" 函数参数的代码
    //  ...
    consumers[i] = new Thread(cons_names[i]);
    consumers[i]->Fork(Consumer, i);
  };
}
