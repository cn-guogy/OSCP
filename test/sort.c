/* sort.c
 *    测试程序，用于对大量整数进行排序。
 *
 *    目的是测试虚拟内存系统。
 *
 *    理想情况下，我们可以从文件系统读取未排序的数组，
 *    并将结果存储回文件系统！
 */

#include "syscall.h"

/* 物理内存的大小；有了代码，我们将用完空间！*/
#define ARRAYSIZE 100

int A[ARRAYSIZE];

int main()
{
    int i, j, tmp;

    /* 首先以逆序初始化数组 */
    for (i = 0; i < ARRAYSIZE; i++)
        A[i] = ARRAYSIZE - i - 1;

    /* 然后进行排序！ */
    for (i = 0; i < (ARRAYSIZE - 1); i++)
        for (j = 0; j < ((ARRAYSIZE - 1) - i); j++)
            if (A[j] > A[j + 1])
            { /* 顺序错误 -> 需要交换！ */
                tmp = A[j];
                A[j] = A[j + 1];
                A[j + 1] = tmp;
            }
    PrintInt(A[0]);             /* 然后我们完成了 -- 应该是 0！ */
    PrintInt(A[1]);             /* 应该是 1 */
    PrintInt(A[ARRAYSIZE - 2]); /* 应该是 ARRAYSIZE - 2 */
    PrintInt(A[ARRAYSIZE - 1]); /* 应该是 ARRAYSIZE - 1 */
    PrintInt(ARRAYSIZE);        /* 应该是 ARRAYSIZE */
    Halt();
}
