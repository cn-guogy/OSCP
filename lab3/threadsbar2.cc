// lab3 n线程屏障问题,模拟时间推进
// 修改自threadsbar1.cc
// 增加MakeTicks函数
// 使用模拟时间推进代替sleep

#include <unistd.h>
#include <stdio.h>

#include "system.h"
#include "synch.h"

#define N_THREADS 10 // 线程的数量
#define MAX_NAME 16  // 名称的最大长度
#define N_TICKS 1000 // 模拟时间前进的节拍数

Thread *threads[N_THREADS];             // 线程指针数组
char thread_names[N_THREADS][MAX_NAME]; // 线程名称的字符数组

Semaphore *barrier; // 屏障的信号量
Semaphore *mutex;   // 互斥的信号量
int nCount = 0;     // 等待线程的数量

void MakeTicks(int n) // 前进 n 个模拟时间节拍
{
    for (int i = 0; i < n; i++)
    {
        interrupt->OneTick(); // 推进一个模拟时间节拍
    }
}

void BarThread(int which)
{
    MakeTicks(N_TICKS);
    printf("线程 %d 会合\n", which);
    mutex->P();
    nCount++;
    int n = nCount;
    mutex->V();
    if (n == N_THREADS)
    {
        printf("线程 %d 是最后一个\n", which);
        for (int i = 0; i < N_THREADS; i++)
        {
            barrier->V(); // 解锁所有等待的线程
        }
    }
    else
    {
        barrier->P(); // 等待解锁
    }
    printf("线程 %d 关键点\n", which);
}

//----------------------------------------------------------------------
// ThreadsBarrier
// 	为 n 线程屏障问题设置信号量
//	创建并分叉线程
//----------------------------------------------------------------------

void ThreadsBarrier()
{
    printf("lab3 n线程屏障问题,模拟时间推进\n");
    DEBUG('t', "ThreadsBarrier");

    // 信号量
    barrier = new Semaphore("barrier", 0);
    mutex = new Semaphore("mutex", 1);

    // 创建并分叉 N_THREADS 线程
    for (int i = 0; i < N_THREADS; i++)
    {
        // 这个语句用于形成一个字符串，以用作线程 i 的名称。
        sprintf(thread_names[i], "thread_%d", i);

        // 使用 thread_names[i] 中的名称和整数 i 作为 "BarThread" 函数的参数创建并分叉一个新线程
        threads[i] = new Thread(thread_names[i]);
        threads[i]->Fork(BarThread, i);
    };
}
