Lab3 使用信号量解决N线程屏障问题
2022.5.8

本目录下的可执行程序n3为Lab3的一个参考实现。
有关N线程屏障问题请参阅："The Little Book of Semaphores V2.2.1 -Allen B. Downey 2016.pdf" 3.6.4。
本实现的一些具体参数及代码框架如下：


#define N_THREADS  10    // 线程数量
#define N_TICKS    1000  // 模拟时间前进的节拍数


void MakeTicks(int n)  // 前进n个模拟时钟节拍
{
   ...
}


void BarThread(_int which)
{
    MakeTicks(N_TICKS);
    printf("线程 %d 到达会合点\n", which);

    ...

    printf("线程 %d 关键点\n", which);
}


void ThreadsBarrier()
{
    ...

    // 创建并分叉 N_THREADS 线程 
    for(i = 0; i < N_THREADS; i++) {
        ...
        threads[i]->Fork(BarThread, i);
    };
}


可输入下面的命令行，进行测试：
./n3 -rs 1

对自己编写生成的程序，命令行为：
./nachos -rs 1
