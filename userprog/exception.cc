// exception.cc
//	用户程序进入Nachos内核的入口点。
//	有两种情况会导致控制权从用户代码转移回这里：
//
//	系统调用 -- 用户代码显式请求调用Nachos内核中的一个过程。
//	目前，我们支持的唯一功能是
//	"Halt"。
//
//	异常 -- 用户代码执行了CPU无法处理的操作。
//	例如，访问不存在的内存、算术错误等。
//
//	中断（也可以导致控制权从用户代码转移到Nachos内核）在其他地方处理。
//
// 目前，这仅处理Halt()系统调用。
// 其他所有情况都会导致核心转储。
//


#include "console.h"

#include "syscall.h"
#include "system.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	进入Nachos内核的入口点。当用户程序
//	正在执行时，发生系统调用或生成地址
//	或算术异常时调用。
//
// 	对于系统调用，以下是调用约定：
//
// 	系统调用代码 -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	系统调用的结果（如果有）必须放回r2中。
//
// 别忘了在返回之前递增pc。（否则你会
// 无限循环执行相同的系统调用！）
//
//	"which"是异常的类型。可能的异常列表
//	在machine.h中。
//----------------------------------------------------------------------

void ExceptionHandler(ExceptionType which) {
  int type = machine->ReadRegister(2);

  if ((which == SyscallException) && (type == SC_Halt)) {
    DEBUG('a', "关机，由用户程序发起。\n");
    interrupt->Halt();
  } else {
    printf("意外的用户模式异常 %d %d\n", which, type);
    ASSERT(FALSE);
  }
}
