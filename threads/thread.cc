// thread.cc
//	线程管理的例程。主要有四个操作：
//
//	Fork -- 创建一个线程以与调用者并发运行一个过程
//		（这分为两个步骤 -- 首先分配线程对象，然后在其上调用Fork）
//	Finish -- 当分叉的过程完成时调用，以进行清理
//	Yield -- 将CPU控制权让给另一个就绪线程
//	Sleep -- 放弃CPU控制权，但线程现在被阻塞。
//		换句话说，它不会再次运行，直到被显式地
//		放回就绪队列。

#include "thread.h"
#include "switch.h"
#include "synch.h"
#include "system.h"

#define STACK_FENCEPOST 0xdeadbeef // 这放在执行栈的顶部，用于检测
                                   // 栈溢出

//----------------------------------------------------------------------
// Thread::Thread
// 	初始化线程控制块，以便我们可以调用
//	Thread::Fork。
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
// 	释放一个线程。
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
    DEBUG('t', "Deleting thread \"%s\"\n", name);

    ASSERT(this != currentThread);
    if (stack != NULL)
        DeallocBoundedArray((char *)stack, StackSize * sizeof(_int));
}

//----------------------------------------------------------------------
// Thread::Fork
// 	调用 (*func)(arg)，允许调用者和被调用者并发执行。
//
//	注意：尽管我们的定义只允许传递一个整数参数
//	到过程，但可以通过将多个参数作为结构的字段，
//	并将指向结构的指针作为 "arg" 进行传递。
//
// 	实现步骤如下：
//		1. 分配一个栈
//		2. 初始化栈，以便调用 SWITCH 将
//		导致它运行该过程
//		3. 将线程放入就绪队列
//
//	"func" 是要并发运行的过程。
//	"arg" 是要传递给过程的单个参数。
//----------------------------------------------------------------------

void Thread::Fork(VoidFunctionPtr func, _int arg)
{
#ifdef HOST_ALPHA
    DEBUG('t', "Forking thread \"%s\" with func = 0x%lx, arg = %ld\n",
          name, (long)func, arg);
#else
    DEBUG('t', "Forking thread \"%s\" with func = 0x%x, arg = %d\n",
          name, (int)func, arg);
#endif

    StackAllocate(func, arg);

    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    scheduler->ReadyToRun(this); // ReadyToRun 假设中断
                                 // 是禁用的！
    (void)interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Thread::CheckOverflow
// 	检查线程的栈以查看它是否超出了为其分配的空间。
//	如果我们有一个更聪明的编译器，
//	我们就不需要担心这个，但我们没有。
//
// 	注意：Nachos 不会捕获所有栈溢出条件。
//	换句话说，您的程序仍然可能因为溢出而崩溃。
//
// 	如果您得到奇怪的结果（例如在没有代码的地方出现段错误）
// 	那么您 *可能* 需要增加栈大小。您可以通过不将大数据结构放在栈上来避免栈溢出。
// 	不要这样做：void foo() { int bigArray[10000]; ... }
//----------------------------------------------------------------------

void Thread::CheckOverflow()
{
    if (stack != NULL)
#ifdef HOST_SNAKE // 栈在 Snakes 上向上增长
        ASSERT((unsigned int)stack[StackSize - 1] == STACK_FENCEPOST);
#else
        ASSERT((unsigned int)*stack == STACK_FENCEPOST);
#endif
}

//----------------------------------------------------------------------
// Thread::Finish
// 	当线程完成执行分叉的过程时由 ThreadRoot 调用。
//
// 	注意：我们不会立即释放线程数据结构
//	或执行栈，因为我们仍在该线程中运行
//	并且仍在栈上！相反，我们设置 "threadToBeDestroyed"，
//	以便 Scheduler::Run() 会在我们
//	在不同线程的上下文中运行时调用析构函数。
//
// 	注意：我们禁用中断，以便我们不会在设置 threadToBeDestroyed 和进入睡眠之间获得时间片。
//----------------------------------------------------------------------

//
void Thread::Finish()
{
    (void)interrupt->SetLevel(IntOff);
    ASSERT(this == currentThread);

    DEBUG('t', "Finishing thread \"%s\"\n", getName());

    threadToBeDestroyed = currentThread;
    Sleep(); // 调用 SWITCH
    // 不会到达
}

//----------------------------------------------------------------------
// Thread::Yield
// 	如果有其他线程准备运行，则放弃 CPU。
//	如果是，将线程放入就绪列表的末尾，以便
//	它最终会被重新调度。
//
//	注意：如果就绪队列中没有其他线程，则立即返回。
//	否则，当线程最终工作到就绪列表的前面并被重新调度时返回。
//
//	注意：我们禁用中断，以便查看就绪列表前面的线程，
//	并切换到它，可以原子地完成。
//	返回时，我们重新设置中断级别为其
//	原始状态，以防我们在禁用中断的情况下被调用。
//
// 	类似于 Thread::Sleep()，但略有不同。
//----------------------------------------------------------------------

void Thread::Yield()
{
    Thread *nextThread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    ASSERT(this == currentThread);

    DEBUG('t', "Yielding thread \"%s\"\n", getName());

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
// 	放弃 CPU，因为当前线程被阻塞
//	等待同步变量（信号量、锁或条件）。
//	最终，某个线程将唤醒此线程，并将其放回
//	就绪队列，以便它可以被重新调度。
//
//	注意：如果就绪队列中没有线程，这意味着
//	我们没有线程可以运行。调用 "Interrupt::Idle"
//	表示我们应该让 CPU 处于空闲状态，直到下一个 I/O 中断
//	发生（唯一可能导致线程变为就绪状态的事情）。
//
//	注意：我们假设中断已经被禁用，因为它
//	是从必须
//	禁用中断以实现原子性的同步例程中调用的。
//	我们需要禁用中断，以便在从就绪列表中取出第一个线程和切换到它之间不会有时间片。
//----------------------------------------------------------------------
void Thread::Sleep()
{
    Thread *nextThread;

    ASSERT(this == currentThread);
    ASSERT(interrupt->getLevel() == IntOff);

    DEBUG('t', "Sleeping thread \"%s\"\n", getName());

    status = BLOCKED;
    while ((nextThread = scheduler->FindNextToRun()) == NULL)
        interrupt->Idle(); // 没有线程可以运行，等待中断

    scheduler->Run(nextThread); // 在我们被信号唤醒时返回
}

//----------------------------------------------------------------------
// ThreadFinish, InterruptEnable, ThreadPrint
//	虚拟函数，因为 C++ 不允许指向成员
//	函数的指针。因此，为了做到这一点，我们创建一个虚拟 C 函数
//	（我们可以传递指针），然后简单地调用该
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
//	Allocate and initialize an execution stack.  The stack is
//	initialized with an initial stack frame for ThreadRoot, which:
//		enables interrupts
//		calls (*func)(arg)
//		calls Thread::Finish
//
//	"func" is the procedure to be forked
//	"arg" is the parameter to be passed to the procedure
//----------------------------------------------------------------------

void Thread::StackAllocate(VoidFunctionPtr func, _int arg)
{
    stack = (int *)AllocBoundedArray(StackSize * sizeof(_int));

#ifdef HOST_SNAKE
    // HP stack works from low addresses to high addresses
    stackTop = stack + 16; // HP requires 64-byte frame marker
    stack[StackSize - 1] = STACK_FENCEPOST;
#else
    // i386 & MIPS & SPARC & ALPHA stack works from high addresses to low addresses
#ifdef HOST_SPARC
    // SPARC stack must contains at least 1 activation record to start with.
    stackTop = stack + StackSize - 96;
#else // HOST_MIPS  || HOST_i386 || HOST_ALPHA
    stackTop = stack + StackSize - 4; // -4 to be on the safe side!
#ifdef HOST_i386
    // the 80386 passes the return address on the stack.  In order for
    // SWITCH() to go to ThreadRoot when we switch to this thread, the
    // return addres used in SWITCH() must be the starting address of
    // ThreadRoot.

    //    *(--stackTop) = (int)ThreadRoot;
    // This statement can be commented out after a bug in SWITCH function
    // of i386 has been fixed: The current last three instruction of
    // i386 SWITCH is as follows:
    // movl    %eax,4(%esp)            # copy over the ret address on the stack
    // movl    _eax_save,%eax
    // ret
    // Here "movl    %eax,4(%esp)" should be "movl   %eax,0(%esp)".
    // After this bug is fixed, the starting address of ThreadRoot,
    // which is stored in machineState[PCState] by the next stament,
    // will be put to the location pointed by %esp when the SWITCH function
    // "return" to ThreadRoot.
    // It seems that this statement was used to get around that bug in SWITCH.
    //
    // However, this statement will be needed, if SWITCH for i386 is
    // further simplified. In fact, the code to save and
    // retore the return address are all redundent, because the
    // return address is already in the stack (pointed by %esp).
    // That is, the following four instructions can be removed:
    // ...
    // movl    0(%esp),%ebx            # get return address from stack into ebx
    // movl    %ebx,_PC(%eax)          # save it into the pc storage
    // ...
    // movl    _PC(%eax),%eax          # restore return address into eax
    // movl    %eax,0(%esp)            # copy over the ret address on the stack#

    // The SWITCH function can be as follows:
    //         .comm   _eax_save,4

    //         .globl  SWITCH
    // SWITCH:
    //         movl    %eax,_eax_save          # save the value of eax
    //         movl    4(%esp),%eax            # move pointer to t1 into eax
    //         movl    %ebx,_EBX(%eax)         # save registers
    //         movl    %ecx,_ECX(%eax)
    //         movl    %edx,_EDX(%eax)
    //         movl    %esi,_ESI(%eax)
    //         movl    %edi,_EDI(%eax)
    //         movl    %ebp,_EBP(%eax)
    //         movl    %esp,_ESP(%eax)         # save stack pointer
    //         movl    _eax_save,%ebx          # get the saved value of eax
    //         movl    %ebx,_EAX(%eax)         # store it

    //         movl    8(%esp),%eax            # move pointer to t2 into eax

    //         movl    _EAX(%eax),%ebx         # get new value for eax into ebx
    //         movl    %ebx,_eax_save          # save it
    //         movl    _EBX(%eax),%ebx         # retore old registers
    //         movl    _ECX(%eax),%ecx
    //         movl    _EDX(%eax),%edx
    //         movl    _ESI(%eax),%esi
    //         movl    _EDI(%eax),%edi
    //         movl    _EBP(%eax),%ebp
    //         movl    _ESP(%eax),%esp         # restore stack pointer

    //         movl    _eax_save,%eax

    //         ret

    // In this case the above statement
    //     *(--stackTop) = (int)ThreadRoot;
    //  is necesssary. But, the following statement
    //     machineState[PCState] = (_int) ThreadRoot;
    //  becomes redundant.

    // Peiyi Tang, ptang@titus.compsci.ualr.edu
    // Department of Computer Science
    // University of Arkansas at Little Rock
    // Sep 1, 2003

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
//	Save the CPU state of a user program on a context switch.
//
//	Note that a user program thread has *two* sets of CPU registers --
//	one for its state while executing user code, one for its state
//	while executing kernel code.  This routine saves the former.
//----------------------------------------------------------------------

void Thread::SaveUserState()
{
    for (int i = 0; i < NumTotalRegs; i++)
        userRegisters[i] = machine->ReadRegister(i);
}

//----------------------------------------------------------------------
// Thread::RestoreUserState
//	Restore the CPU state of a user program on a context switch.
//
//	Note that a user program thread has *two* sets of CPU registers --
//	one for its state while executing user code, one for its state
//	while executing kernel code.  This routine restores the former.
//----------------------------------------------------------------------

void Thread::RestoreUserState()
{
    for (int i = 0; i < NumTotalRegs; i++)
        machine->WriteRegister(i, userRegisters[i]);
}
#endif
