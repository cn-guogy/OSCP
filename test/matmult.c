/* matmult.c 
 *    测试程序，用于对大数组进行矩阵乘法。
 *
 *    旨在测试虚拟内存系统。
 *
 *    理想情况下，我们可以从文件系统读取矩阵，
 *    并将结果存储回文件系统！
 */

#include "syscall.h"

#define Dim 	20	/* 数组的总和不适合物理内存 
			 */

int A[Dim][Dim];
int B[Dim][Dim];
int C[Dim][Dim];

int
main()
{
    int i, j, k;

    for (i = 0; i < Dim; i++)		/* 首先初始化矩阵 */
	for (j = 0; j < Dim; j++) {
	     A[i][j] = i;
	     B[i][j] = j;
	     C[i][j] = 0;
	}

    for (i = 0; i < Dim; i++)		/* 然后将它们相乘 */
	for (j = 0; j < Dim; j++)
            for (k = 0; k < Dim; k++)
		 C[i][j] += A[i][k] * B[k][j];

    Exit(C[Dim-1][Dim-1]);		/* 然后我们完成了 */
}
