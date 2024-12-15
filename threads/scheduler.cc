// scheduler.cc
//	选择下一个要运行的线程，并调度到该线程的例程。
//
// 	这些例程假设中断已经被禁用。
//	如果中断被禁用，我们可以假设互斥
//	（因为我们在单处理器上）。
//
// 	注意：我们不能在这里使用锁来提供互斥，因为
// 	如果我们需要等待一个锁，而锁正在被占用，我们将
//	最终调用 FindNextToRun()，这将使我们陷入一个
//	无限循环。
//
// 	非常简单的实现——没有优先级，直接的 FIFO。
//	可能需要在以后的作业中改进。

#include "scheduler.h"
#include "system.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	初始化准备但未运行的线程列表为空。
//----------------------------------------------------------------------

Scheduler::Scheduler() { readyList = new List; }

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	释放准备线程的列表。
//----------------------------------------------------------------------

Scheduler::~Scheduler() { delete readyList; }

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	将线程标记为准备好，但未运行。
//	将其放入准备列表，以便稍后调度到 CPU。
//
//	"thread" 是要放入准备列表的线程。
//----------------------------------------------------------------------

void Scheduler::ReadyToRun(Thread *thread)
{
  DEBUG('t', "将线程 %s 放入准备列表。\n", thread->getName());

  thread->setStatus(READY);
  readyList->Append((void *)thread);
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	返回下一个要调度到 CPU 的线程。
//	如果没有准备好的线程，则返回 NULL。
// 副作用：
//	线程从准备列表中移除。
//----------------------------------------------------------------------

Thread *Scheduler::FindNextToRun() { return (Thread *)readyList->Remove(); }

//----------------------------------------------------------------------
// Scheduler::Run
// 	将 CPU 调度到 nextThread。保存旧线程的状态，
//	并通过调用机器相关的上下文切换例程 SWITCH 加载新线程的状态。
//
//      注意：我们假设之前运行的线程的状态已经
//	从运行状态更改为阻塞或准备状态（视情况而定）。
// 副作用：
//	全局变量 currentThread 变为 nextThread。
//
//	"nextThread" 是要放入 CPU 的线程。
//----------------------------------------------------------------------

void Scheduler::Run(Thread *nextThread)
{
  Thread *oldThread = currentThread;

#ifdef USER_PROGRAM // 忽略，直到运行用户程序
  if (currentThread->space != NULL)
  {                                 // 如果该线程是用户程序，
    currentThread->SaveUserState(); // 保存用户的 CPU 寄存器
    currentThread->space->SaveState();
  }
#endif

  oldThread->CheckOverflow(); // 检查旧线程是否
                              // 存在未检测到的栈溢出

  currentThread = nextThread;        // 切换到下一个线程
  currentThread->setStatus(RUNNING); // nextThread 现在正在运行

  DEBUG('t', "从线程 \"%s\" 切换到线程 \"%s\"\n",
        oldThread->getName(), nextThread->getName());

  // 这是一个机器相关的汇编语言例程，在 switch.s 中定义。
  // 你可能需要思考一下
  // 发生了什么，从线程的角度和“外部世界”的角度来看。

  SWITCH(oldThread, nextThread);

  DEBUG('t', "现在在线程 \"%s\"\n", currentThread->getName());

  // 如果旧线程因为完成而放弃了处理器，
  // 我们需要删除它的尸体。注意，我们不能在现在之前删除线程
  // （例如，在 Thread::Finish() 中），因为到目前为止，
  // 我们仍然在旧线程的栈上运行！
  if (threadToBeDestroyed != NULL)
  {
    delete threadToBeDestroyed;
    threadToBeDestroyed = NULL;
  }

#ifdef USER_PROGRAM
  if (currentThread->space != NULL)
  {                                    // 如果存在地址空间
    currentThread->RestoreUserState(); // 恢复时，执行此操作。
    currentThread->space->RestoreState();
  }
#endif
}

//----------------------------------------------------------------------
// Scheduler::Print
// 	打印调度器状态——换句话说，就是准备列表的内容。
//	用于调试。
//----------------------------------------------------------------------
void Scheduler::Print()
{
  printf("准备列表内容：\n");
  readyList->Mapcar((VoidFunctionPtr)ThreadPrint);
}
