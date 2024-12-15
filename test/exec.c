/* exec.c
 *	简单程序，用于运行另一个用户程序。
 */

#include "syscall.h"

int main()
{
    int pid;
    PrintInt(12345);
    pid = Exec("../test/halt2.noff");
    Halt();
    /* 不会到达 */
}
