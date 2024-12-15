// ring++.h
//	用于生产者和消费者问题的环形缓冲区的数据结构
//

// 以下定义了环形缓冲区类。函数在文件 ring.cc 中实现。
//
// 环形缓冲区的构造函数（初始化器）传入一个整数，表示缓冲区的大小（槽的数量）。

// 环形缓冲区中槽的类

#include "synch.h"

class slot
{
public:
  slot(int id, int number);
  slot()
  {
    thread_id = 0;
    value = 0;
  };

  int thread_id;
  int value;
};

class Ring
{
public:
  Ring(int sz); // 构造函数：初始化变量，分配空间。
  ~Ring();      // 析构函数：释放上面分配的空间。

  void Put(slot *message); // 将消息放入下一个空槽。

  void Get(slot *message); // 从下一个满槽获取消息。

  int Full();  // 如果环形缓冲区满则返回非0， 否则返回0。
  int Empty(); // 如果环形缓冲区空则返回非0， 否则返回0。

private:
  int size;     // 环形缓冲区的大小。
  int in, out;  // Put 和 Get 的索引
  slot *buffer; // 指向环形缓冲区数组的指针。
  int current;  // 缓冲区中当前满槽的数量

  Condition_H *notfull;  // 条件变量，等待直到不满
  Condition_H *notempty; // 条件变量，等待直到不空

  Semaphore *mutex; // 互斥信号量
  Semaphore *next;  // "next" 队列的信号量
  int next_count;   // "next" 队列中线程的数量
};
