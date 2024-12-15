// interrupt.cc 修改自 lab6/interrupt.cc
// 增加了页错误处理
#include "interrupt.h"
#include "system.h"

extern int StartProcess(char *file);

static const char *intLevelNames[] = {"关闭", "开启"};
static const char *intTypeNames[] = {"定时器", "磁盘", "控制台写入",
                                     "控制台读取", "网络发送", "网络接收"};

// 页错误处理
void Interrupt::PageFault(int BadAddress)
{
    AddrSpace *space = currentThread->space;
    space->GetPageToMem(BadAddress / PageSize);
}

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

    ASSERT((now == IntOff) || (inHandler == FALSE));

    ChangeLevel(old, now);
    if ((now == IntOn) && (old == IntOff))
        OneTick();
    return old;
}

void Interrupt::Enable()
{
    (void)SetLevel(IntOn);
}

void Interrupt::OneTick()
{
    MachineStatus old = status;

    if (status == SystemMode)
    {
        stats->totalTicks += SystemTick;
        stats->systemTicks += SystemTick;
    }
    else
    {
        stats->totalTicks += UserTick;
        stats->userTicks += UserTick;
    }
    DEBUG('i', "\n== 时钟 %d ==\n", stats->totalTicks);

    ChangeLevel(IntOn, IntOff);
    while (CheckIfDue(FALSE))
        ;
    ChangeLevel(IntOff, IntOn);
    if (yieldOnReturn)
    {
        yieldOnReturn = FALSE;
        status = SystemMode;
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
    {
        while (CheckIfDue(FALSE))
            ;
        yieldOnReturn = FALSE;
        status = SystemMode;
        return;
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
    Cleanup();
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

    ASSERT(level == IntOff);
    if (DebugIsEnabled('i'))
        DumpState();
    PendingInterrupt *toOccur =
        (PendingInterrupt *)pending->SortedRemove(&when);

    if (toOccur == NULL)
        return FALSE;

    if (advanceClock && when > stats->totalTicks)
    {
        stats->idleTicks += (when - stats->totalTicks);
        stats->totalTicks = when;
    }
    else if (when > stats->totalTicks)
    {
        pending->SortedInsert(toOccur, when);
        return FALSE;
    }

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
    status = SystemMode;
    (*(toOccur->handler))(toOccur->arg);
    status = old;
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
