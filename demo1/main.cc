// main.cc
//	引导代码以初始化操作系统内核。
//
//	允许直接调用内部操作系统函数，
//	以简化调试和测试。实际上，
//	引导代码只会初始化数据结构，
//	并启动一个用户程序以打印登录提示。
//
// 	此文件的大部分内容在后续作业中不需要。
//
// 用法: nachos -d <调试标志> -rs <随机种子 #>
//		-s -x <nachos 文件> -c <控制台输入> <控制台输出>
//		-f -cp <unix 文件> <nachos 文件>
//		-p <nachos 文件> -r <nachos 文件> -l -D -t
//              -n <网络可靠性> -m <机器 ID>
//              -o <其他机器 ID>
//              -z
//
//    -d 会导致打印某些调试消息 (参见 utility.h)
//    -rs 会导致 Yield 在随机（但可重复）的位置发生
//    -z 打印版权信息
//
//  用户程序
//    -s 会导致用户程序以单步模式执行
//    -x 运行一个用户程序
//    -c 测试控制台
//
//  文件系统
//    -f 会导致物理磁盘被格式化
//    -cp 从 UNIX 复制文件到 Nachos
//    -p 打印 Nachos 文件到标准输出
//    -r 从文件系统中删除 Nachos 文件
//    -l 列出 Nachos 目录的内容
//    -D 打印整个文件系统的内容
//    -t 测试 Nachos 文件系统的性能
//
//  网络
//    -n 设置网络可靠性
//    -m 设置此机器的主机 ID（网络所需）
//    -o 运行 Nachos 网络软件的简单测试
//
//  注意 -- 标志在相关作业之前被忽略。
//  一些标志在这里被解释；一些在 system.cc 中。

#define MAIN
#undef MAIN

#include "utility.h"
#include "system.h"

// 本文件使用的外部函数

extern void ProdCons(void), Copy(char *unixFile, char *nachosFile);
extern void Print(char *file), PerformanceTest(void);
extern void StartProcess(char *file), ConsoleTest(char *in, char *out);
extern void MailTest(int networkID);

//----------------------------------------------------------------------
// main
// 	引导操作系统内核。
//
//	检查命令行参数
//	初始化数据结构
//	（可选）调用测试过程
//
//	"argc" 是命令行参数的数量
//	（包括命令的名称） -- 例如: "nachos -d +" -> argc = 3
//	"argv" 是一个字符串数组，每个命令行参数对应一个字符串
//		例如: "nachos -d +" -> argv = {"nachos", "-d", "+"}
//----------------------------------------------------------------------

int main(int argc, char **argv)
{
	int argCount; // 参数的数量
				  // 对于特定命令

	DEBUG('t', "进入主函数");
	(void)Initialize(argc, argv);

#ifdef THREADS
	ProdCons();
#endif

	for (argc--, argv++; argc > 0; argc -= argCount, argv += argCount)
	{
		argCount = 1;
#ifdef USER_PROGRAM
		if (!strcmp(*argv, "-x"))
		{ // 运行一个用户程序
			ASSERT(argc > 1);
			StartProcess(*(argv + 1));
			argCount = 2;
		}
		else if (!strcmp(*argv, "-c"))
		{ // 测试控制台
			if (argc == 1)
				ConsoleTest(NULL, NULL);
			else
			{
				ASSERT(argc > 2);
				ConsoleTest(*(argv + 1), *(argv + 2));
				argCount = 3;
			}
			interrupt->Halt(); // 一旦我们启动控制台，Nachos 将
							   // 永远循环等待控制台输入
		}
#endif // USER_PROGRAM
#ifdef FILESYS
		if (!strcmp(*argv, "-cp"))
		{ // 从 UNIX 复制到 Nachos
			ASSERT(argc > 2);
			Copy(*(argv + 1), *(argv + 2));
			argCount = 3;
		}
		else if (!strcmp(*argv, "-p"))
		{ // 打印一个 Nachos 文件
			ASSERT(argc > 1);
			Print(*(argv + 1));
			argCount = 2;
		}
		else if (!strcmp(*argv, "-r"))
		{ // 删除 Nachos 文件
			ASSERT(argc > 1);
			fileSystem->Remove(*(argv + 1));
			argCount = 2;
		}
		else if (!strcmp(*argv, "-l"))
		{ // 列出 Nachos 目录
			fileSystem->List();
		}
		else if (!strcmp(*argv, "-D"))
		{ // 打印整个文件系统
			fileSystem->Print();
		}
		else if (!strcmp(*argv, "-t"))
		{ // 性能测试
			PerformanceTest();
		}
#endif // FILESYS
#ifdef NETWORK
		if (!strcmp(*argv, "-o"))
		{
			ASSERT(argc > 1);
			Delay(2); // 延迟 2 秒
					  // 以给用户时间启动另一个 nachos
			MailTest(atoi(*(argv + 1)));
			argCount = 2;
		}
#endif // NETWORK
	}

	currentThread->Finish(); // 注意: 如果过程 "main"
							 // 返回，则程序 "nachos"
							 // 将退出（就像任何其他正常程序
							 // 一样）。但可能还有其他
							 // 线程在就绪列表中。我们通过
							 // 声明 "main" 线程已完成来切换
							 // 到这些线程，防止它返回。
	return (0);
} // 不会到达...
