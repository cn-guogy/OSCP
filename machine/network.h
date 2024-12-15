// network.h 
//	用于模拟物理网络连接的数据结构。
//	网络提供了无序、不可靠、固定大小的数据包传递抽象，
//	可以传递给网络上的其他机器。
//
//	您可能会注意到，网络的接口与控制台设备相似 -- 
//	两者都是全双工通道。
//
//  请勿更改 -- 机器仿真的一部分
//

//
// 修改:
//
//   日期: 1995年7月
//   作者: K. Salem
//   描述: 添加数据包延迟，以便数据包传递不保证有序
//

#ifndef NETWORK_H
#define NETWORK_H


#include "utility.h"

// 网络地址 -- 唯一标识一台机器。该机器的ID 
//  在命令行中给出。
typedef int NetworkAddress;	 

// 以下类定义网络数据包头。
// 数据包头由网络驱动程序在数据有效负载之前添加， 
// 在数据包通过网络发送之前。网络上的格式是:  
//	数据包头 (PacketHeader)
//	数据 (包含来自邮局的MailHeader!)

class PacketHeader {
  public:
    NetworkAddress to;		// 目标机器ID
    NetworkAddress from;	// 源机器ID
    unsigned length;	 	// 数据包数据的字节数，不包括 
				// 数据包头（但包括邮局添加的 
				// MailHeader）
};

#define MaxWireSize 	64	// 可以通过网络发送的最大数据包
#define MaxPacketSize 	(MaxWireSize - sizeof(struct PacketHeader))	
				// 最大数据包的“有效负载”


/* 以下类定义物理网络设备。网络
   能够将固定大小的数据包传递给
   连接到网络的其他机器。
   数据包传递既不有序也不可靠。
   
   网络的“可靠性”可以在构造函数中指定。
   这个数字在0到1之间，是网络不会丢失
   数据包的概率。
   
   网络的“有序性”也可以在构造函数中指定。
   这个数字在0到1之间，是数据包在网络中不会被
   延迟的概率，*前提是它没有丢失*。一个延迟的
   数据包最终会被传递。然而，在延迟数据包之后
   发送到同一目标的其他数据包可能会在
   目标处到达延迟数据包之前。
   
   有序性参数仅用于未丢失的数据包。
   因此，如果可靠性设置为0.9且有序性设置为0.9，
   则任何数据包在没有丢失和没有延迟的情况下
   被发送的概率为81%。有10%的概率会丢失，
   以及9%的概率会延迟。
   
   请注意，您可以通过更改Initialize()中
   RandomInit()的参数来更改随机数生成器的种子。
   随机数生成器用于选择丢弃或延迟哪些数据包。 */

class Network {
  public:
    Network(NetworkAddress addr, double reliability, double orderability,
  	  VoidFunctionPtr readAvail, VoidFunctionPtr writeDone, _int callArg);
				// 分配并初始化网络驱动程序
    ~Network();			// 释放网络驱动程序数据

    void Send(PacketHeader hdr, char* data);
    				// 将数据包数据发送到远程机器，
				// 由“hdr”指定。成功发送后返回。
    				// “writeHandler”在下一个 
				// 数据包可以发送时被调用。请注意，writeHandler 
				// 无论数据包是否被丢弃都会被调用，并且请注意 
				// PacketHeader的“from”字段会自动 
				// 由Send()填充。

    PacketHeader Receive(char* data);
    				// 轮询网络以获取传入消息。  
				// 如果有数据包等待，复制数据包 
				// 到“data”并返回头部。
				// 如果没有数据包等待，返回一个长度为0的头部。

    void SendDone();		// 中断处理程序，当消息被 
				// 发送时调用
    void CheckPktAvail();	// 检查是否有传入的数据包

  private:
    NetworkAddress ident;	// 该机器的网络地址
    double chanceToWork;	// 数据包不会被丢弃的概率
    double chanceToNotDelay;       // 数据包不会被延迟的概率
    int sock;			// 用于传入数据包的UNIX套接字编号
    char sockName[32];		// 与UNIX套接字对应的文件名
    VoidFunctionPtr writeHandler; // 中断处理程序，指示下一个数据包 
				// 可以被发送。  
    VoidFunctionPtr readHandler;  // 中断处理程序，指示数据包已 
				// 到达。
    _int handlerArg;		// 传递给中断处理程序的参数
				//   （指向邮局的指针）
    bool sendBusy;		// 数据包正在发送中。
    bool packetAvail;		// 数据包已到达，可以从
				//   网络中提取
    PacketHeader inHdr;		// 有关到达数据包的信息
    char inbox[MaxPacketSize];  // 到达数据包的数据
    char delayBuf[MaxWireSize];  // 保存延迟数据包的地方
    char delayToName[32];       // 最终发送延迟数据包的地方
    bool delayBufFull;          // delayBuf是否在使用中？
};

#endif // NETWORK_H
