// synchtest.cc
//      另一个线程的测试用例。使用锁和条件变量 
//      来实现一个桥（见问题 7）。
//     
//      SynchThread 只是将一辆车来回发送过桥。通过
//      分叉多个这样的线程，可以模拟桥上的交通！
//


#include "system.h"
#include "synch.h"

// 见问题 7。桥最多可以容纳 3 辆车。它是
// 单车道的，因此车辆只能朝一个方向通过——否则
// 会发生正面碰撞。
class Bridge {
  private:
    int numCars;
    int currentDirec;
    Condition *bridgeFull;
    Lock *lock;
  public:
    Bridge();
    ~Bridge();
    void Arrive(int direc);   // 当车辆可以通过时返回
    void Cross(int direc);    // 不执行任何操作，除了可能打印消息
    void Exit(int direc);     // 离开桥
};

//----------------------------------------------------------------------
// Bridge::Bridge
//     初始化桥的初始状态
//----------------------------------------------------------------------
Bridge::Bridge()
{
    numCars = 0;
    currentDirec = 0;
    bridgeFull = new Condition("bridge");
    lock = new Lock("bridge");
}

//----------------------------------------------------------------------
// Bridge::~Bridge
//----------------------------------------------------------------------
Bridge::~Bridge()
{
    delete bridgeFull;
    delete lock;
}

//----------------------------------------------------------------------
// Bridge::Arrive
//         车辆到达桥，想要朝着方向
//         direc 通过。  
//----------------------------------------------------------------------
void
Bridge::Arrive(int direc)
{
    DEBUG('t', "到达桥。方向 [%d]", direc);
    lock->Acquire();

    // 我们可以继续，如果：  
    //       a) 桥是空的
    //    或 b) 桥上少于 3 辆车且交通流向
    //          是我们想要的方向
    // 否则，等待
    while((numCars > 0) && ((numCars >= 3) || (direc != currentDirec))) {
	bridgeFull->Wait(lock);
    }
    numCars++;               // 预留一个桥上的位置
    currentDirec = direc;    // 确保方向匹配 
    lock->Release();
    DEBUG('t', "方向 [%d]，现在准备通过桥", direc);
}

//----------------------------------------------------------------------
// Bridge::Exit
//         车辆离开桥。
//----------------------------------------------------------------------
void
Bridge::Exit(int direc)
{
    lock->Acquire();
    numCars--;              // 释放我们在桥上的位置
    DEBUG('t', "方向 [%d]，桥出口", direc);
    bridgeFull->Broadcast(lock);  // 向所有等待桥的线程发送信号
    lock->Release();
}

//----------------------------------------------------------------------
// Bridge::Cross
//         车辆通过桥。
//----------------------------------------------------------------------
void
Bridge::Cross(int direc)
{
    DEBUG('t', "方向 [%d]，正在通过桥", direc);
}


//  基于桥类的测试用例。  
//
//
Bridge *bridge = new Bridge;

//----------------------------------------------------------------------
// SynchThread
//      模拟一辆车在桥上来回穿行
//      多次——一定要有个好视野！ :).
//
//----------------------------------------------------------------------
void
SynchThread(_int which)
{
    int num;
    int direc;
    
    for (num = 0; num < 5; num++) {
        direc = num % 2;  // 设置方向（交替）
// 以下 printf 需要修复以适应不同类型的 _int
	printf("方向 [%d]，车辆 [%d]，到达中...\n", direc, which);
	bridge->Arrive(direc);
	currentThread->Yield();
	printf("方向 [%d]，车辆 [%d]，正在通过...\n", direc, which);
	bridge->Cross(direc);
	currentThread->Yield();
        printf("方向 [%d]，车辆 [%d]，正在退出...\n", direc, which);
	bridge->Exit(direc);
	currentThread->Yield();
    }
}

void
SynchTest()
{
    const int maxCars = 7;  // 交通量有多少？
    int i = 0;
    Thread *ts[maxCars];

    for(i=0; i < maxCars; i++) {
	ts[i] = new Thread("forked thread");
	ts[i]->Fork(SynchThread, i);
    }
}
