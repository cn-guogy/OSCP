// utility.h 
//	杂项有用的定义，包括调试例程。
//
//	调试例程允许用户打开选定的
//	调试消息，可以通过传递给 Nachos 的命令行参数控制 (-d)。你被鼓励添加自己的
//	调试标志。预定义的调试标志有：
//
//	'+' -- 打开所有调试消息
//   	't' -- 线程系统
//   	's' -- 信号量、锁和条件变量
//   	'i' -- 中断仿真
//   	'm' -- 机器仿真 (USER_PROGRAM)
//   	'd' -- 磁盘仿真 (FILESYS)
//   	'f' -- 文件系统 (FILESYS)
//   	'a' -- 地址空间 (USER_PROGRAM)
//   	'n' -- 网络仿真 (NETWORK)


#ifndef UTILITY_H
#define UTILITY_H

#ifdef HOST_ALPHA		// 需要因为 gcc 使用 64 位指针和
#define _int long		// DEC ALPHA 架构上的 32 位整数。
#else
#define _int int
#endif

// 杂项有用的例程

#include "bool.h"
					 	// 布尔值。  
						// 这与 g++ 库中的定义相同.
/*
#ifdef FALSE
#undef FALSE
#endif
#ifdef TRUE
#undef TRUE
#endif

#define FALSE 0
#define TRUE  1

#define bool int		// 需要避免如果 bool 类型
				// 已经定义并且布尔值
				// 被分配给整数变量时的问题。
*/

#define min(a,b)  (((a) < (b)) ? (a) : (b))
#define max(a,b)  (((a) > (b)) ? (a) : (b))

// 除法并向上或向下取整 
#define divRoundDown(n,s)  ((n) / (s))
#define divRoundUp(n,s)    (((n) / (s)) + ((((n) % (s)) > 0) ? 1 : 0))

// 这声明了类型 "VoidFunctionPtr" 为 "指向一个
// 接受整数参数并返回无的函数的指针"。使用
// 这样的函数指针（假设它是 "func"），我们可以像这样调用它：
//
//	(*func) (17);
//
// 这在 Thread::Fork 和中断处理程序中使用，以及
// 其他几个地方。

typedef void (*VoidFunctionPtr)(_int arg); 
typedef void (*VoidNoArgFunctionPtr)(); 


// 包含接口，将我们与主机机器系统库隔离。
// 需要定义 bool 和 VoidFunctionPtr
#include "sysdep.h"				

// 调试例程的接口。

extern void DebugInit(char* flags);	// 启用打印调试消息

extern bool DebugIsEnabled(char flag); 	// 这个调试标志是否启用？

extern void DEBUG (char flag, const char* format, ...);  	// 打印调试消息 
							// 如果标志被启用

//----------------------------------------------------------------------
// ASSERT
//      如果条件为假，打印消息并转储核心。
//	对文档化代码中的假设很有用。
//
//	注意：需要是一个 #define，以便能够打印错误发生的 
//	位置。
//----------------------------------------------------------------------
#define ASSERT(condition)                                                     \
    if (!(condition)) {                                                       \
        fprintf(stderr, "断言失败: 行 %d, 文件 \"%s\"\n",           \
                __LINE__, __FILE__);                                          \
	fflush(stderr);							      \
        Abort();                                                              \
    }


#endif // UTILITY_H
