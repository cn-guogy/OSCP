/* switch.s 
 *   	与机器相关的上下文切换例程。请勿修改这些！ 
 *
 *	上下文切换本质上是与机器相关的，因为
 *	要保存的寄存器、如何设置初始
 *	调用帧等，都是特定于处理器架构的。
 *
 * 	此文件当前支持以下架构：
 *	    DEC MIPS
 *	    SUN SPARC
 *	    HP PA-RISC
 *	    Intel 386
 *
 * 我们为每个架构定义两个例程：
 *
 * ThreadRoot(InitialPC, InitialArg, WhenDonePC, StartupPC)
 *	InitialPC  - 要在此线程中运行的过程的程序计数器。
 *      InitialArg - 线程的单个参数。
 *	WhenDonePC - 线程返回时要调用的例程。
 *	StartupPC  - 启动线程时要调用的例程。
 *
 *	ThreadRoot 从 SWITCH() 例程调用以首次启动
 *	一个线程。
 *
 * SWITCH(oldThread, newThread)
 * 	oldThread  - 当前正在运行的线程，CPU寄存器状态
 *		将被保存到此处。
 * 	newThread  - 要运行的新线程，CPU寄存器状态
 *		将从此处加载。
 */

/*
 版权所有 (c) 1992-1993 加州大学的摄政。
 保留所有权利。请参阅 以获取版权声明和责任限制
 和免责声明条款。
 */


#include "switch.h"

#ifdef HOST_i386

        .text
        .align  2

        .globl  ThreadRoot

/* void ThreadRoot( void )
**
** 期望以下寄存器被初始化：
**      eax     指向启动函数（中断使能）
**      edx     包含线程函数的初始参数
**      esi     指向线程函数
**      edi     指向 Thread::Finish()
*/
ThreadRoot:
        pushl   %ebp
        movl    %esp,%ebp
        pushl   InitialArg
        call    *StartupPC
        call    *InitialPC
        call    *WhenDonePC

        # 不会到达
        movl    %ebp,%esp
        popl    %ebp
        ret



/* void SWITCH( thread *t1, thread *t2 )
**
** 进入时，堆栈看起来像这样：
**      8(esp)  ->              thread *t2
**      4(esp)  ->              thread *t1
**       (esp)  ->              返回地址
**
**
**
**
*/
        .comm   _eax_save,4

        .globl  SWITCH
SWITCH:
        movl    %eax,_eax_save          # 保存 eax 的值
        movl    4(%esp),%eax            # 将指针移动到 t1 进入 eax
        movl    %ebx,_EBX(%eax)         # 保存寄存器
        movl    %ecx,_ECX(%eax)
        movl    %edx,_EDX(%eax)
        movl    %esi,_ESI(%eax)
        movl    %edi,_EDI(%eax)
        movl    %ebp,_EBP(%eax)
        movl    %esp,_ESP(%eax)         # 保存堆栈指针
        movl    _eax_save,%ebx          # 获取保存的 eax 值
        movl    %ebx,_EAX(%eax)         # 存储它
        movl    0(%esp),%ebx            # 从堆栈中获取返回地址到 ebx
        movl    %ebx,_PC(%eax)          # 将其保存到 pc 存储中

        movl    8(%esp),%eax            # 将指针移动到 t2 进入 eax

        movl    _EAX(%eax),%ebx         # 获取新值到 eax 进入 ebx
        movl    %ebx,_eax_save          # 保存它
        movl    _EBX(%eax),%ebx         # 恢复旧寄存器
        movl    _ECX(%eax),%ecx
        movl    _EDX(%eax),%edx
        movl    _ESI(%eax),%esi
        movl    _EDI(%eax),%edi
        movl    _EBP(%eax),%ebp
        movl    _ESP(%eax),%esp         # 恢复堆栈指针
        movl    _PC(%eax),%eax          # 恢复返回地址到 eax
#       movl    %eax,4(%esp)            # 将返回地址复制到堆栈上
        movl    %eax,0(%esp)    
	#这是一个错误，返回地址的偏移量应该是 0，而不是 4。
	# -- ptang, Sep/1/03, at UALR
	
        movl    _eax_save,%eax

        ret

#endif
