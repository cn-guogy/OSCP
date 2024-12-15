// system.cc 修改自 threads/system.cc
// 增加了freePhys_Map的初始化

#include "system.h"

// 这定义了 *所有* 用于 Nachos 的全局数据结构。
// 这些都由此文件初始化和释放。

Thread *currentThread;       // 我们现在正在运行的线程
Thread *threadToBeDestroyed; // 刚刚完成的线程
Scheduler *scheduler;        // 就绪列表
Interrupt *interrupt;        // 中断状态
Statistics *stats;           // 性能指标
Timer *timer;                // 硬件定时器设备，
                             // 用于调用上下文切换

#ifdef FILESYS_NEEDED
FileSystem *fileSystem;
#endif

#ifdef FILESYS
SynchDisk *synchDisk;
#endif

#ifdef USER_PROGRAM // 需要 FILESYS 或 FILESYS_STUB
Machine *machine;   // 用户程序内存和寄存器
BitMap *freePhys_Map;
int space;
#endif

#ifdef NETWORK
PostOffice *postOffice;
#endif

// 外部定义，以允许我们获取指向此函数的指针
extern void Cleanup();

//----------------------------------------------------------------------
// TimerInterruptHandler
// 	定时器设备的中断处理程序。定时器设备被
//	设置为定期中断 CPU（每 TimerTicks 一次）。
//	每次发生定时器中断时调用此例程，
//	并禁用中断。
//
//	请注意，我们不是直接调用 Yield()（这将
//	挂起中断处理程序，而不是被中断的线程
//	这是我们想要上下文切换的），我们设置一个标志
//	以便在中断处理程序完成后，它将看起来像
//	被中断的线程在被中断的地方调用了 Yield。
//
//	“dummy”是因为每个中断处理程序都需要一个参数，
//		无论它是否需要。
//----------------------------------------------------------------------
static void TimerInterruptHandler(_int dummy)
{
  if (interrupt->getStatus() != IdleMode)
    interrupt->YieldOnReturn();
}

//----------------------------------------------------------------------
// Initialize
// 	初始化 Nachos 全局数据结构。解释命令行
//	参数以确定初始化的标志。
//
//	“argc”是命令行参数的数量（包括命令的名称）-- 例如：
//	“nachos -d +” -> argc = 3
//	“argv”是一个字符串数组，每个命令行参数一个
//		例如：“nachos -d +” -> argv = {"nachos", "-d", "+"}
//----------------------------------------------------------------------
void Initialize(int argc, char **argv)
{
  int argCount;
  char *debugArgs = (char *)"";
  bool randomYield = FALSE;

#ifdef USER_PROGRAM
  bool debugUserProg = FALSE; // 单步调试用户程序
#endif
#ifdef FILESYS_NEEDED
  bool format = FALSE; // 格式化磁盘
#endif
#ifdef NETWORK
  double rely = 1;  // 网络可靠性
  double order = 1; // 网络顺序性
  int netname = 0;  // UNIX 套接字名称
#endif

  for (argc--, argv++; argc > 0; argc -= argCount, argv += argCount)
  {
    argCount = 1;
    if (!strcmp(*argv, "-d"))
    {
      if (argc == 1)
        debugArgs = (char *)"+"; // 打开所有调试标志
      else
      {
        debugArgs = *(argv + 1);
        argCount = 2;
      }
    }
    else if (!strcmp(*argv, "-rs"))
    {
      ASSERT(argc > 1);
      RandomInit(atoi(*(argv + 1))); // 初始化伪随机
                                     // 数字生成器
      randomYield = TRUE;
      argCount = 2;
    }
#ifdef USER_PROGRAM
    if (!strcmp(*argv, "-s"))
      debugUserProg = TRUE;
#endif
#ifdef FILESYS_NEEDED
    if (!strcmp(*argv, "-f"))
      format = TRUE;
#endif
#ifdef NETWORK
    if (!strcmp(*argv, "-n"))
    {
      ASSERT(argc > 1);
      rely = atof(*(argv + 1));
      argCount = 2;
    }
    else if (!strcmp(*argv, "-e"))
    {
      ASSERT(argc > 1);
      order = atof(*(argv + 1));
      argCount = 2;
    }
    else if (!strcmp(*argv, "-m"))
    {
      ASSERT(argc > 1);
      netname = atoi(*(argv + 1));
      argCount = 2;
    }
#endif
  }

  DebugInit(debugArgs);        // 初始化调试消息
  stats = new Statistics();    // 收集统计信息
  interrupt = new Interrupt;   // 启动中断处理
  scheduler = new Scheduler(); // 初始化就绪队列
  if (randomYield)             // 启动定时器（如果需要）
    timer = new Timer(TimerInterruptHandler, 0, randomYield);

  threadToBeDestroyed = NULL;

  // 我们没有显式分配当前正在运行的线程。
  // 但如果它尝试放弃 CPU，我们最好有一个线程
  // 对象来保存它的状态。
  currentThread = new Thread("main");
  currentThread->setStatus(RUNNING);

  interrupt->Enable();
  CallOnUserAbort(Cleanup); // 如果用户按下 ctl-C

#ifdef USER_PROGRAM
  machine = new Machine(debugUserProg); // 这必须先执行
  freePhys_Map = new BitMap(NumPhysPages);
  space = 0;
#endif

#ifdef FILESYS
  synchDisk = new SynchDisk("DISK");
#endif

#ifdef FILESYS_NEEDED
  fileSystem = new FileSystem(format);
#endif

#ifdef NETWORK
  postOffice = new PostOffice(netname, rely, order, 10);
#endif
}

//----------------------------------------------------------------------
// Cleanup
// 	Nachos 正在停止。释放全局数据结构。
//----------------------------------------------------------------------
void Cleanup()
{
  printf("\n正在清理...\n");
#ifdef NETWORK
  delete postOffice;
#endif

#ifdef USER_PROGRAM
  delete machine;
#endif

#ifdef FILESYS_NEEDED
  delete fileSystem;
#endif

#ifdef FILESYS
  delete synchDisk;
#endif

  delete timer;
  delete scheduler;
  delete interrupt;

  Exit(0);
}
