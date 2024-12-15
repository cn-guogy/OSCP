// syscall.h 复制自 lab6/syscall.h
// 无修改

#ifndef SYSCALLS_H
#define SYSCALLS_H

#define SC_Halt 0
#define SC_Exit 1
#define SC_Exec 2
#define SC_Join 3
#define SC_Create 4
#define SC_Open 5
#define SC_Read 6
#define SC_Write 7
#define SC_Close 8
#define SC_Fork 9
#define SC_Yield 10
#define SC_PrintInt 11

#ifndef IN_ASM

void Halt();
void Exit(int status);
int Exec(char *name);
int Join(int id);
typedef int OpenFileId;
#define ConsoleInput 0
#define ConsoleOutput 1
void Create(char *name);
OpenFileId Open(char *name);
void Write(char *buffer, int size, OpenFileId id);
int Read(char *buffer, int size, OpenFileId id);
void Close(OpenFileId id);
void Fork(void (*func)());
void Yield();
void PrintInt(int num);

#endif /* IN_ASM */

#endif /* SYSCALL_H */
