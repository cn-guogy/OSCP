// console.h 
//	用于模拟终端的行为的数据结构
//	I/O 设备。终端有两个部分——键盘输入，
//	和显示输出，每个部分依次产生/接受 
//	字符。
//
//	控制台硬件设备是异步的。当一个字符被
//	写入设备时，例程立即返回，并且在 I/O 完成时
//	会调用一个中断处理程序。
//	对于读取，当一个字符到达时会调用一个中断处理程序。
//
//	设备的用户可以指定在读取/写入中断发生时
//	要调用的例程。读取和写入有各自的中断，
//	设备是“全双工”的——一个字符
//	可以同时是输出和输入。
//
//  不要更改——机器仿真的一部分
//
#ifndef CONSOLE_H
#define CONSOLE_H


#include "utility.h"

// 以下类定义了一个硬件控制台设备。
// 对设备的输入和输出通过读取 
// 和写入 UNIX 文件（“readFile”和“writeFile”）来模拟。
//
// 由于设备是异步的，当一个字符到达时，
// 中断处理程序“readAvail”会被调用，准备读取。
// 当输出字符被“放置”时，
// 中断处理程序“writeDone”会被调用，以便下一个字符可以被写入。

class Console {
  public:
    Console(char *readFile, char *writeFile, VoidFunctionPtr readAvail, 
	VoidFunctionPtr writeDone, _int callArg);
				// 初始化硬件控制台设备
    ~Console();			// 清理控制台仿真

// 外部接口——Nachos 内核代码可以调用这些
    void PutChar(char ch);	// 将“ch”写入控制台显示， 
				// 并立即返回。“writeHandler” 
				// 在 I/O 完成时被调用。

    char GetChar();	   	// 轮询控制台输入。如果有字符 
				// 可用，返回它。否则，返回 EOF。
    				// “readHandler”在有字符可获取时被调用

// 内部仿真例程——不要调用这些。 
    void WriteDone();	 	// 内部例程以信号 I/O 完成
    void CheckCharAvail();

  private:
    int readFileNo;			// 模拟键盘的 UNIX 文件 
    int writeFileNo;			// 模拟显示的 UNIX 文件
    VoidFunctionPtr writeHandler; 	// 在 PutChar I/O 完成时调用的中断处理程序
    VoidFunctionPtr readHandler; 	// 在键盘上到达字符时调用的中断处理程序
    _int handlerArg;			// 传递给中断处理程序的参数
    bool putBusy;    			// PutChar 操作是否正在进行中？
					// 如果是，你不能再进行另一个操作！
    char incoming;    			// 包含要读取的字符，
					// 如果有可用字符。
					// 否则包含 EOF。
};

#endif // CONSOLE_H
