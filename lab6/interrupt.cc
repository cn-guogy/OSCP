// interrupt.cc 修改自 threads/interrupt.cc
// 实现了 Exec 方法
// 实现了 PrintInt 方法

#include "interrupt.h"
#include "system.h"
// 调试消息的字符串定义

extern int StartProcess(char *file);

static const char *intLevelNames[] = {"关闭", "开启"};
static const char *intTypeNames[] = {"定时器", "磁盘", "控制台写入",
                                     "控制台读取", "网络发送", "网络接收"};

int Interrupt::Exec(char *name)
{
    int spaceId = StartProcess(name);
    return spaceId;
}

void Interrupt::PrintInt(int num)
{
    printf("%d\n", num);
    return;
}

PendingInterrupt::PendingInterrupt(VoidFunctionPtr func, _int param, int time,
                                   IntType kind)
{
    handler = func;
    arg = param;
    when = time;
    type = kind;
}

Interrupt::Interrupt()
{
    level = IntOff;
    pending = new List();
    inHandler = FALSE;
    yieldOnReturn = FALSE;
    status = SystemMode;
}

Interrupt::~Interrupt()
{
    while (!pending->IsEmpty())
        delete (ListElement *)(pending->Remove());
    delete pending;
}

void Interrupt::ChangeLevel(IntStatus old, IntStatus now)
{
    level = now;
    DEBUG('i', "\t中断: %s -> %s\n", intLevelNames[old], intLevelNames[now]);
}

IntStatus
Interrupt::SetLevel(IntStatus now)
{
    IntStatus old = level;

    ASSERT((now == IntOff) || (inHandler == FALSE)); // 中断处理程序
                                                     // 不允许启用
                                                     // 中断

    ChangeLevel(old, now); // 改变为新状态
    if ((now == IntOn) && (old == IntOff))
        OneTick(); // 推进模拟时间
    return old;
}

void Interrupt::Enable()
{
    (void)SetLevel(IntOn);
}

void Interrupt::OneTick()
{
    MachineStatus old = status;

    // 推进模拟时间
    if (status == SystemMode)
    {
        stats->totalTicks += SystemTick;
        stats->systemTicks += SystemTick;
    }
    else
    { // USER_PROGRAM
        stats->totalTicks += UserTick;
        stats->userTicks += UserTick;
    }
    DEBUG('i', "\n== 时钟 %d ==\n", stats->totalTicks);

    // 检查任何待处理的中断是否现在准备好触发
    ChangeLevel(IntOn, IntOff); // 首先，关闭中断
                                // （中断处理程序在
                                // 禁用中断的情况下运行）
    while (CheckIfDue(FALSE))   // 检查待处理的中断
        ;
    ChangeLevel(IntOff, IntOn); // 重新启用中断
    if (yieldOnReturn)
    { // 如果定时器设备处理程序请求
        // 进行上下文切换，现在可以执行
        yieldOnReturn = FALSE;
        status = SystemMode; // yield 是一个内核例程
        currentThread->Yield();
        status = old;
    }
}

void Interrupt::YieldOnReturn()
{
    ASSERT(inHandler == TRUE);
    yieldOnReturn = TRUE;
}

void Interrupt::Idle()
{
    DEBUG('i', "机器空闲；检查中断。\n");
    status = IdleMode;
    if (CheckIfDue(TRUE))
    {                             // 检查是否有任何待处理的中断
        while (CheckIfDue(FALSE)) // 检查是否有其他待处理的
            ;                     // 中断
        yieldOnReturn = FALSE;    // 由于就绪队列中没有任何内容，
                                  // yield 是自动的
        status = SystemMode;
        return; // 返回以防现在有
                // 可运行的线程
    }

    DEBUG('i', "机器空闲。没有中断要处理。\n");
    printf("没有线程准备或可运行，并且没有待处理的中断。\n");
    printf("假设程序已完成。\n");
    Halt();
}

void Interrupt::Halt()
{
    printf("机器正在停止！\n\n");
    stats->Print();
    Cleanup(); // 永远不会返回。
}

void Interrupt::Schedule(VoidFunctionPtr handler, _int arg, int fromNow, IntType type)
{
    int when = stats->totalTicks + fromNow;
    PendingInterrupt *toOccur = new PendingInterrupt(handler, arg, when, type);

    DEBUG('i', "调度中断处理程序 %s 在时间 = %d\n",
          intTypeNames[type], when);
    ASSERT(fromNow > 0);

    pending->SortedInsert(toOccur, when);
}

bool Interrupt::CheckIfDue(bool advanceClock)
{
    MachineStatus old = status;
    int when;

    ASSERT(level == IntOff); // 中断需要被禁用，
                             // 才能调用中断处理程序
    if (DebugIsEnabled('i'))
        DumpState();
    PendingInterrupt *toOccur =
        (PendingInterrupt *)pending->SortedRemove(&when);

    if (toOccur == NULL) // 没有待处理的中断
        return FALSE;

    if (advanceClock && when > stats->totalTicks)
    { // 推进时钟
        stats->idleTicks += (when - stats->totalTicks);
        stats->totalTicks = when;
    }
    else if (when > stats->totalTicks)
    { // 还没到时间，放回去
        pending->SortedInsert(toOccur, when);
        return FALSE;
    }

    // 检查是否没有更多的事情要做，如果是，则退出
    if ((status == IdleMode) && (toOccur->type == TimerInt) && pending->IsEmpty())
    {
        pending->SortedInsert(toOccur, when);
        return FALSE;
    }

    DEBUG('i', "在时间 %d 调用 %s 的中断处理程序\n",
          intTypeNames[toOccur->type], toOccur->when);
#ifdef USER_PROGRAM
    if (machine != NULL)
        machine->DelayedLoad(0, 0);
#endif
    inHandler = TRUE;
    status = SystemMode;                 // 无论我们在做什么，
                                         // 现在我们将
                                         // 在内核中运行
    (*(toOccur->handler))(toOccur->arg); // 调用中断处理程序
    status = old;                        // 恢复机器状态
    inHandler = FALSE;
    delete toOccur;
    return TRUE;
}

static void
PrintPending(_int arg)
{
    PendingInterrupt *pend = (PendingInterrupt *)arg;

    printf("中断处理程序 %s，计划在 %d 发生\n",
           intTypeNames[pend->type], pend->when);
}

void Interrupt::DumpState()
{
    printf("时间: %d, 中断 %s\n", stats->totalTicks,
           intLevelNames[level]);
    printf("待处理的中断:\n");
    fflush(stdout);
    pending->Mapcar(PrintPending);
    printf("待处理的中断结束\n");
    fflush(stdout);
}
