// thread.cc
//	线程管理的例程。主要有四个操作：
//
//	Fork -- 创建一个线程以与调用者并发运行
//		（这分两步进行 -- 首先分配线程对象，然后调用Fork）
//	Finish -- 当分叉的过程完成时调用，以进行清理
//	Yield -- 将CPU控制权让给另一个就绪线程
//	Sleep -- relinquish control over the CPU, but thread is now blocked.
//		换句话说，它不会再次运行，直到被显式地
//		放回就绪队列。

#include "thread.h"
#include "switch.h"
#include "synch.h"
#include "system.h"

#define STACK_FENCEPOST 0xdeadbeef // 这个值放在执行栈的顶部，用于检测
                                   // 栈溢出

//----------------------------------------------------------------------
// Thread::Thread
// 	初始化线程控制块，以便我们可以调用
//	Thread::Fork.
//
//	"threadName" 是一个任意字符串，便于调试。
//----------------------------------------------------------------------

Thread::Thread(const char *threadName)
{
    name = (char *)threadName;
    stackTop = NULL;
    stack = NULL;
    status = JUST_CREATED;
#ifdef USER_PROGRAM
    space = NULL;
#endif
}

//----------------------------------------------------------------------
// Thread::~Thread
// 	释放一个线程的资源。
//
// 	注意：当前线程 *不能* 直接删除自己，
//	因为它仍在运行我们需要删除的栈上。
//
//      注意：如果这是主线程，我们不能删除栈
//      因为我们没有分配它 -- 我们是自动获得的
//      作为启动Nachos的一部分。
//----------------------------------------------------------------------

Thread::~Thread()
{
    DEBUG('t', "正在删除线程 \"%s\"\n", name);

    ASSERT(this != currentThread);
    if (stack != NULL)
        DeallocBoundedArray((char *)stack, StackSize * sizeof(_int));
}

//----------------------------------------------------------------------
// Thread::Println
// 	打印线程名称并换行
//----------------------------------------------------------------------

void Thread::Println(void)
{
    printf("%s\n", name);
}

//----------------------------------------------------------------------
// Thread::Fork
// 	调用 (*func)(arg)，允许调用者和被调用者并发执行。
//
//	注意：虽然我们的定义只允许传递一个整数参数
//	给过程，但可以通过将多个参数放入结构体的字段中，
//	并将指向结构体的指针作为 "arg" 传递来实现。
//
// 	实现步骤如下：
//		1. 分配一个栈
//		2. 初始化栈，以便调用SWITCH时
//		可以运行该过程
//		3. 将线程放入就绪队列
//
//	"func" 是要并发运行的过程。
//	"arg" 是要传递给过程的单个参数。
//----------------------------------------------------------------------

void Thread::Fork(VoidFunctionPtr func, _int arg)
{
#ifdef HOST_ALPHA
    DEBUG('t', "正在分叉线程 \"%s\"，func = 0x%lx, arg = %ld\n",
          name, (long)func, arg);
#else
    DEBUG('t', "正在分叉线程 \"%s\"，func = 0x%x, arg = %d\n",
          name, (int)func, arg);
#endif

    StackAllocate(func, arg);

    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    scheduler->ReadyToRun(this); // ReadyToRun假设中断
                                 // 是禁用的！
    (void)interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Thread::CheckOverflow
// 	检查线程的栈以查看是否超出了为其分配的空间。
//	如果我们有一个更智能的编译器，
//	我们就不需要担心这个，但我们没有。
//
// 	注意：Nachos不会捕获所有栈溢出条件。
//	换句话说，您的程序可能仍会因为溢出而崩溃。
//
// 	如果您得到奇怪的结果（例如在没有代码的地方出现段错误）
// 	那么您 *可能* 需要增加栈大小。您可以通过不将大型数据结构放在栈上来避免栈溢出。
// 	不要这样做：void foo() { int bigArray[10000]; ... }
//----------------------------------------------------------------------

void Thread::CheckOverflow()
{
    if (stack != NULL)
#ifdef HOST_SNAKE // 栈在Snakes上向上增长
        ASSERT((unsigned int)stack[StackSize - 1] == STACK_FENCEPOST);
#else
        ASSERT((unsigned int)*stack == STACK_FENCEPOST);
#endif
}

//----------------------------------------------------------------------
// Thread::Finish
// 	当线程完成执行分叉的过程时由ThreadRoot调用。
//
// 	注意：我们不会立即释放线程数据结构
//	或执行栈，因为我们仍在该线程中运行
//	并且仍在栈上！相反，我们设置 "threadToBeDestroyed"，
//	以便Scheduler::Run()将在我们
//	在不同线程的上下文中运行时调用析构函数。
//
// 	注意：我们禁用中断，以便在设置threadToBeDestroyed和进入睡眠之间不会有时间片。
//----------------------------------------------------------------------

//
void Thread::Finish()
{
    (void)interrupt->SetLevel(IntOff);
    ASSERT(this == currentThread);

    DEBUG('t', "正在结束线程 \"%s\"\n", getName());

    threadToBeDestroyed = currentThread;
    Sleep(); // 调用SWITCH
    // 不会到达
}

//----------------------------------------------------------------------
// Thread::Yield
// 	如果有其他线程准备运行，则放弃CPU。
//	如果是，将线程放到就绪列表的末尾，以便
//	它最终会被重新调度。
//
//	注意：如果就绪队列中没有其他线程，则立即返回。
//	否则，当线程最终在就绪列表的前面并被重新调度时返回。
//
//	注意：我们禁用中断，以便查看就绪列表前面的线程
//	并切换到它可以原子地完成。返回时，我们重新设置中断级别为其
//	原始状态，以防我们在禁用中断的情况下被调用。
//
// 	类似于Thread::Sleep()，但略有不同。
//----------------------------------------------------------------------

void Thread::Yield()
{
    Thread *nextThread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    ASSERT(this == currentThread);

    DEBUG('t', "正在让出线程 \"%s\"\n", getName());

    nextThread = scheduler->FindNextToRun();
    if (nextThread != NULL)
    {
        scheduler->ReadyToRun(this);
        scheduler->Run(nextThread);
    }
    (void)interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Thread::Sleep
// 	放弃CPU，因为当前线程被阻塞
//	等待同步变量（信号量、锁或条件）。
//	最终，某个线程将唤醒此线程，并将其放回
//	就绪队列，以便它可以被重新调度。
//
//	注意：如果就绪队列中没有线程，这意味着
//	我们没有线程可以运行。调用 "Interrupt::Idle"
//	表示我们应该让CPU空闲，直到下一个I/O中断
//	发生（唯一可能导致线程变为就绪状态的事情）。
//
//	注意：我们假设中断已经被禁用，因为它
//	是从必须禁用中断以确保原子性的同步例程中调用的。
//	我们需要禁用中断，以便在从就绪列表中取出第一个线程和切换到它之间不会有时间片。
//----------------------------------------------------------------------
void Thread::Sleep()
{
    Thread *nextThread;

    ASSERT(this == currentThread);
    ASSERT(interrupt->getLevel() == IntOff);

    DEBUG('t', "正在睡眠的线程 \"%s\"\n", getName());

    status = BLOCKED;
    while ((nextThread = scheduler->FindNextToRun()) == NULL)
        interrupt->Idle(); // 没有线程可以运行，等待中断

    scheduler->Run(nextThread); // 在我们被信号时返回
}

//----------------------------------------------------------------------
// ThreadFinish, InterruptEnable, ThreadPrint
//	虚拟函数，因为C++不允许指向成员
//	函数的指针。因此，为了做到这一点，我们创建一个虚拟C函数
//	（我们可以传递指针），然后简单地调用
//	成员函数。
//----------------------------------------------------------------------

static void ThreadFinish() { currentThread->Finish(); }
static void InterruptEnable() { interrupt->Enable(); }
void ThreadPrint(_int arg)
{
    Thread *t = (Thread *)arg;
    t->Print();
}

//----------------------------------------------------------------------
// Thread::StackAllocate
//	分配并初始化执行栈。栈被
//	初始化为ThreadRoot的初始栈帧，它：
//	启用中断
//	调用 (*func)(arg)
//	调用 Thread::Finish
//
//	"func" 是要分叉的过程
//	"arg" 是要传递给过程的参数
//----------------------------------------------------------------------

void Thread::StackAllocate(VoidFunctionPtr func, _int arg)
{
    stack = (int *)AllocBoundedArray(StackSize * sizeof(_int));

#ifdef HOST_SNAKE
    // HP栈从低地址到高地址工作
    stackTop = stack + 16; // HP需要64字节的帧标记
    stack[StackSize - 1] = STACK_FENCEPOST;
#else
    // i386 & MIPS & SPARC & ALPHA栈从高地址到低地址工作
#ifdef HOST_SPARC
    // SPARC栈必须至少包含1个激活记录以开始。
    stackTop = stack + StackSize - 96;
#else // HOST_MIPS  || HOST_i386 || HOST_ALPHA
    stackTop = stack + StackSize - 4; // -4以确保安全！
#ifdef HOST_i386
    // 80386在栈上传递返回地址。为了使
    // SWITCH()在我们切换到此线程时转到ThreadRoot，
    // SWITCH中使用的返回地址必须是
    // ThreadRoot的起始地址。

    //    *(--stackTop) = (int)ThreadRoot;
    // 这个语句可以在i386的SWITCH函数中的一个错误修复后注释掉：
    // i386 SWITCH的当前最后三条指令如下：
    // movl    %eax,4(%esp)            # 将返回地址复制到栈上
    // movl    _eax_save,%eax
    // ret
    // 这里 "movl    %eax,4(%esp)" 应该是 "movl   %eax,0(%esp)"。
    // 在这个错误修复后，ThreadRoot的起始地址
    // 将在SWITCH函数 "返回" 到ThreadRoot时放入
    // %esp指向的位置。
    // 似乎这个语句是用来解决SWITCH中的错误。
    //
    // 然而，如果i386的SWITCH进一步简化，这个语句将是必要的。
    // 实际上，保存和恢复返回地址的代码都是多余的，因为
    // 返回地址已经在栈中（指向%esp）。
    // 也就是说，以下四条指令可以被移除：
    // ...
    // movl    0(%esp),%ebx            # 从栈中获取返回地址到ebx
    // movl    %ebx,_PC(%eax)          # 保存到pc存储中
    // ...
    // movl    _PC(%eax),%eax          # 将返回地址恢复到eax
    // movl    %eax,0(%esp)            # 将返回地址复制到栈上#

    // SWITCH函数可以如下：
    //         .comm   _eax_save,4

    //         .globl  SWITCH
    // SWITCH:
    //         movl    %eax,_eax_save          # 保存eax的值
    //         movl    4(%esp),%eax            # 将指向t1的指针移动到eax
    //         movl    %ebx,_EBX(%eax)         # 保存寄存器
    //         movl    %ecx,_ECX(%eax)
    //         movl    %edx,_EDX(%eax)
    //         movl    %esi,_ESI(%eax)
    //         movl    %edi,_EDI(%eax)
    //         movl    %ebp,_EBP(%eax)
    //         movl    %esp,_ESP(%eax)         # 保存栈指针
    //         movl    _eax_save,%ebx          # 获取保存的eax值
    //         movl    %ebx,_EAX(%eax)         # 存储它

    //         movl    8(%esp),%eax            # 将指向t2的指针移动到eax

    //         movl    _EAX(%eax),%ebx         # 获取新值到ebx
    //         movl    %ebx,_eax_save          # 保存它
    //         movl    _EBX(%eax),%ebx         # 恢复旧寄存器
    //         movl    _ECX(%eax),%ecx
    //         movl    _EDX(%eax),%edx
    //         movl    _ESI(%eax),%esi
    //         movl    _EDI(%eax),%edi
    //         movl    _EBP(%eax),%ebp
    //         movl    _ESP(%eax),%esp         # 恢复栈指针

    //         movl    _eax_save,%eax

    //         ret

    // 在这种情况下，上面的语句
    //     *(--stackTop) = (int)ThreadRoot;
    //  是必要的。但以下语句
    //     machineState[PCState] = (_int) ThreadRoot;
    //  变得多余。

    // Peiyi Tang, ptang@titus.compsci.ualr.edu
    // 计算机科学系
    // 阿肯色大学小石城分校
    // 2003年9月1日

#endif
#endif // HOST_SPARC
    *stack = STACK_FENCEPOST;
#endif // HOST_SNAKE

    machineState[PCState] = (_int)ThreadRoot;
    machineState[StartupPCState] = (_int)InterruptEnable;
    machineState[InitialPCState] = (_int)func;
    machineState[InitialArgState] = arg;
    machineState[WhenDonePCState] = (_int)ThreadFinish;
}

#ifdef USER_PROGRAM
#include "machine.h"

//----------------------------------------------------------------------
// Thread::SaveUserState
//	在上下文切换时保存用户程序的CPU状态。
//
//	注意，用户程序线程有 *两* 套CPU寄存器 --
//	一套用于执行用户代码时的状态，一套用于执行内核代码时的状态。
//	此例程保存前者。
//----------------------------------------------------------------------

void Thread::SaveUserState()
{
    for (int i = 0; i < NumTotalRegs; i++)
        userRegisters[i] = machine->ReadRegister(i);
}

//----------------------------------------------------------------------
// Thread::RestoreUserState
//	在上下文切换时恢复用户程序的CPU状态。
//
//	注意，用户程序线程有 *两* 套CPU寄存器 --
//	一套用于执行用户代码时的状态，一套用于执行内核代码时的状态。
//	此例程恢复前者。
//----------------------------------------------------------------------

void Thread::RestoreUserState()
{
    for (int i = 0; i < NumTotalRegs; i++)
        machine->WriteRegister(i, userRegisters[i]);
}
#endif
