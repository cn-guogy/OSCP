// exception.cc 修改自 userprog/exception.cc
// 增加了 SC_Exec 入口
// 添加了 SC_PrintInt 入口
// 提供了AdvancePC函数

#include "console.h"

#include "syscall.h"
#include "system.h"

void AdvancePC(void)
{
  machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
  machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
  machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
}

int getInt(void){
  return machine->ReadRegister(4);
}

char *getName(void){
  int i = 0;
  int addr = machine->ReadRegister(4);
  char *name = new char[128];
  do
  {
    machine->ReadMem(addr + i, 1, (int *)&name[i]); // 从主内存读取文件名
  } while (name[i++] != '\0');
  return name;
}

void ExceptionHandler(ExceptionType which)
{
  int type = machine->ReadRegister(2);
  printf("which: %d, type: %d\n", which, type);
  if (which == SyscallException)
    switch (type)
    {
    case SC_Halt:
      DEBUG('a', "关机，由用户程序发起。\n");
      interrupt->Halt();
      break;
    case SC_PrintInt:
      printf("执行系统调用 PrintInt()\n");
      DEBUG('a', "执行，由用户程序发起。\n");
      interrupt->PrintInt(getInt());
      AdvancePC();
      break;
    case SC_Exec:
      printf("执行系统调用 Exec()\n");
      DEBUG('a', "执行，由用户程序发起。\n");
      interrupt->Exec(getName());
      AdvancePC();
      break;

    default:
      printf("系统调用 %d 未实现\n", type);
      AdvancePC();
      break;
    }
  else
  {
    printf("意外的用户模式异常 %d %d\n", which, type);
    ASSERT(FALSE);
  }
  
}
