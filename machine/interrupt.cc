// interrupt.cc
//	用于模拟硬件中断的例程。
//
//	硬件提供了一个例程（SetLevel）来启用或禁用
//	中断。
//
//	为了仿真硬件，我们需要跟踪所有
//	硬件设备可能引发的中断，以及它们
//	应该发生的时间。
//
//	该模块还跟踪模拟时间。时间的推进
//	仅在以下情况发生时：
//		中断被重新启用
//		执行用户指令
//		就绪队列中没有任何内容
//
//  请勿更改 -- 机器仿真的一部分
//

#include "interrupt.h"
#include "system.h"

// 调试消息的字符串定义

static const char *intLevelNames[] = {"关闭", "开启"};
static const char *intTypeNames[] = {"定时器", "磁盘", "控制台写入",
                                     "控制台读取", "网络发送", "网络接收"};

//----------------------------------------------------------------------
// PendingInterrupt::PendingInterrupt
// 	初始化一个即将发生的硬件设备中断。
//
//	"func" 是中断发生时调用的过程
//	"param" 是传递给过程的参数
//	"time" 是中断发生的时间（以模拟时间为单位）
//	"kind" 是生成中断的硬件设备
//----------------------------------------------------------------------

PendingInterrupt::PendingInterrupt(VoidFunctionPtr func, _int param, int time,
                                   IntType kind)
{
    handler = func;
    arg = param;
    when = time;
    type = kind;
}

//----------------------------------------------------------------------
// Interrupt::Interrupt
// 	初始化硬件设备中断的仿真。
//
//	中断开始时禁用，没有待处理的中断等。
//----------------------------------------------------------------------

Interrupt::Interrupt()
{
    level = IntOff;
    pending = new List();
    inHandler = FALSE;
    yieldOnReturn = FALSE;
    status = SystemMode;
}

//----------------------------------------------------------------------
// Interrupt::~Interrupt
// 	释放中断仿真所需的数据结构。
//----------------------------------------------------------------------

Interrupt::~Interrupt()
{
    while (!pending->IsEmpty())
        delete (ListElement *)(pending->Remove());
    delete pending;
}

//----------------------------------------------------------------------
// Interrupt::ChangeLevel
// 	改变中断的启用或禁用状态，而不推进
//	模拟时间（通常，启用中断会推进时间）。
//
//----------------------------------------------------------------------
// Interrupt::ChangeLevel
// 	改变中断的启用或禁用状态，而不推进
//	模拟时间（通常，启用中断会推进时间）。
//
//	内部使用。
//
//	"old" -- 旧的中断状态
//	"now" -- 新的中断状态
//----------------------------------------------------------------------
void Interrupt::ChangeLevel(IntStatus old, IntStatus now)
{
    level = now;
    DEBUG('i', "\t中断: %s -> %s\n", intLevelNames[old], intLevelNames[now]);
}

//----------------------------------------------------------------------
// Interrupt::SetLevel
// 	改变中断的启用或禁用状态，如果中断
//	被启用，则通过调用 OneTick() 来推进模拟时间。
//
// 返回:
//	旧的中断状态。
// 参数:
//	"now" -- 新的中断状态
//----------------------------------------------------------------------

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

//----------------------------------------------------------------------
// Interrupt::Enable
// 	启用中断。以前的状态无所谓。
//	在 ThreadRoot 中使用，以在首次启动
//	线程时启用中断。
//----------------------------------------------------------------------
void Interrupt::Enable()
{
    (void)SetLevel(IntOn);
}

//----------------------------------------------------------------------
// Interrupt::OneTick
// 	推进模拟时间并检查是否有任何待处理
//	中断需要被调用。
//
//	两件事情可以导致 OneTick 被调用：
//		中断被重新启用
//		执行用户指令
//----------------------------------------------------------------------
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

//----------------------------------------------------------------------
// Interrupt::YieldOnReturn
// 	在中断处理程序内部调用，以导致上下文切换
//	（例如，在时间片上）在被中断的线程中，
//	当处理程序返回时。
//
//	我们不能在这里进行上下文切换，因为这会切换
//	出中断处理程序，而我们想要切换出被中断的线程。
//----------------------------------------------------------------------

void Interrupt::YieldOnReturn()
{
    ASSERT(inHandler == TRUE);
    yieldOnReturn = TRUE;
}

//----------------------------------------------------------------------
// Interrupt::Idle
// 	当就绪队列中没有任何内容时调用的例程。
//
//	由于必须有某些东西在运行才能将线程
//	放入就绪队列，因此唯一要做的就是推进
//	模拟时间，直到下一个计划的硬件中断。
//
//	如果没有待处理的中断，则停止。我们没有
//	更多的事情要做。
//----------------------------------------------------------------------
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

    // 如果没有待处理的中断，并且就绪队列中没有任何内容，
    // 是时候停止了。如果控制台或网络正在运行，
    // 总是会有待处理的中断，因此不会到达此代码。
    // 相反，停止必须由用户程序调用。

    DEBUG('i', "机器空闲。没有中断要处理。\n");
    printf("没有线程准备或可运行，并且没有待处理的中断。\n");
    printf("假设程序已完成。\n");
    Halt();
}

//----------------------------------------------------------------------
// Interrupt::Halt
// 	干净地关闭 Nachos，打印出性能统计信息。
//----------------------------------------------------------------------
void Interrupt::Halt()
{
    printf("机器正在停止！\n\n");
    stats->Print();
    Cleanup(); // 永远不会返回。
}

//----------------------------------------------------------------------
// Interrupt::Schedule
// 	安排在模拟时间达到 "now + when" 时中断 CPU。
//
//	实现：只需将其放入排序列表中。
//
//	注意：Nachos 内核不应直接调用此例程。
//	相反，它仅由硬件设备模拟器调用。
//
//	"handler" 是中断发生时调用的过程
//	"arg" 是传递给过程的参数
//	"fromNow" 是多远的未来（以模拟时间为单位）
//		 中断将发生
//	"type" 是生成中断的硬件设备
//----------------------------------------------------------------------
void Interrupt::Schedule(VoidFunctionPtr handler, _int arg, int fromNow, IntType type)
{
    int when = stats->totalTicks + fromNow;
    PendingInterrupt *toOccur = new PendingInterrupt(handler, arg, when, type);

    DEBUG('i', "调度中断处理程序 %s 在时间 = %d\n",
          intTypeNames[type], when);
    ASSERT(fromNow > 0);

    pending->SortedInsert(toOccur, when);
}

//----------------------------------------------------------------------
// Interrupt::CheckIfDue
// 	检查是否有中断计划发生，如果有，则触发它。
//
// 返回:
//	TRUE，如果我们触发了任何中断处理程序
// 参数:
//	"advanceClock" -- 如果为 TRUE，就绪队列中没有任何内容，
//		因此我们应该简单地推进时钟到下一个
//		待处理的中断将发生的时间（如果有）。如果待处理的
//		中断只是时间片守护程序，那么我们就完成了！
//----------------------------------------------------------------------
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

//----------------------------------------------------------------------
// PrintPending
// 	打印有关计划发生的中断的信息。
//	何时，在哪里，为什么，等等。
//----------------------------------------------------------------------

static void
PrintPending(_int arg)
{
    PendingInterrupt *pend = (PendingInterrupt *)arg;

    printf("中断处理程序 %s，计划在 %d 发生\n",
           intTypeNames[pend->type], pend->when);
}

//----------------------------------------------------------------------
// DumpState
// 	打印完整的中断状态 - 状态，以及所有计划在未来发生的中断。
//----------------------------------------------------------------------

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
