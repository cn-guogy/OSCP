// ring++.h
//	用于生产者和消费者问题的环形缓冲区的数据结构
//
// 版权所有 (c) 1995 昆士兰大学的校长和董事会。
// 保留所有权利。请参阅 copyright.h 以获取版权声明和责任限制 
// 及免责声明条款。


// 以下定义了环形缓冲区类。函数在文件 ring.cc 中实现。
//
// 环形缓冲区的构造函数（初始化器）传入一个整数，表示缓冲区的大小（槽的数量）。

// 环形缓冲区中槽的类
class slot {
    public:
    slot(int id, int number);
    slot() { thread_id = 0; value = 0;};
    
    int thread_id;
    int value;
    };


class Ring {
  public:
    Ring(int sz);    // 构造函数：初始化变量，分配空间。
    ~Ring();         // 析构函数：释放上述分配的空间。

    
    void Put(slot *message); // 将消息放入下一个空槽。
    
    void Get(slot *message); // 从下一个满槽中获取消息。
                                            
    int Full();       // 如果环形缓冲区满则返回非0， 否则返回0。
    int Empty();      // 如果环形缓冲区空则返回非0， 否则返回0。
    
  private:
    int size;         // 环形缓冲区的大小。
    int in, out;      // 索引
    slot *buffer;       // 指向环形缓冲区数组的指针。
};
