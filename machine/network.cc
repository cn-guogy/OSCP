// network.cc 
//	用于模拟网络接口的例程，使用 UNIX 套接字
//	在多个 Nachos 实例之间传递数据包。
//
//  请勿更改 -- 机器仿真的一部分
//

// 修改:
//
//   日期: 1995年7月
//   作者: K. Salem
//   描述: 添加数据包延迟，以便数据包传递
//                  不保证有序
//


#include "system.h"

// 虚拟函数，因为 C++ 不能间接调用成员函数 
static void NetworkReadPoll(_int arg)
{ Network *net = (Network *)arg; net->CheckPktAvail(); }
static void NetworkSendDone(_int arg)
{ Network *net = (Network *)arg; net->SendDone(); }

// 初始化网络仿真
//   addr 用于生成套接字名称
//   reliability 表示我们是否丢弃数据包以模拟不可靠链接
//   readAvail, writeDone, callArg -- 类似于控制台
Network::Network(NetworkAddress addr, double reliability, double orderability,
	VoidFunctionPtr readAvail, VoidFunctionPtr writeDone, _int callArg)
{
    ident = addr;
    if (reliability < 0) chanceToWork = 0;
    else if (reliability > 1) chanceToWork = 1;
    else chanceToWork = reliability;

    if (orderability < 0) chanceToNotDelay = 0;
    else if (orderability > 1) chanceToNotDelay = 1;
    else chanceToNotDelay = orderability;

    // 设置以模拟异步中断的内容
    writeHandler = writeDone;
    readHandler = readAvail;
    handlerArg = callArg;
    sendBusy = FALSE;
    inHdr.length = 0;
    delayBufFull = FALSE;
    
    sock = OpenSocket();
    sprintf(sockName, "SOCKET_%d", (int)addr);
    AssignNameToSocket(sockName, sock);		 // 将套接字绑定到当前目录中的文件名

    // 开始轮询传入的数据包
    interrupt->Schedule(NetworkReadPoll, (_int)this, NetworkTime, NetworkRecvInt);
}

Network::~Network()
{
    CloseSocket(sock);
    DeAssignNameToSocket(sockName);
}

// 如果数据包已经缓冲，我们只需延迟读取 
// 传入的数据包。在现实中，如果我们无法及时读取，
// 数据包可能会被丢弃。
void
Network::CheckPktAvail()
{
    // 安排下次轮询数据包的时间
    interrupt->Schedule(NetworkReadPoll, (_int)this, NetworkTime, NetworkRecvInt);

    if (inHdr.length != 0) 	// 如果数据包已经缓冲，则不执行任何操作
	return;		
    if (!PollSocket(sock)) 	// 如果没有数据包可读，则不执行任何操作
	return;

    // 否则，读取数据包
    char *buffer = new char[MaxWireSize];
    ReadFromSocket(sock, buffer, MaxWireSize);

    // 将数据包分为头部和数据
    inHdr = *(PacketHeader *)buffer;
    ASSERT((inHdr.to == ident) && (inHdr.length <= MaxPacketSize));
    bcopy(buffer + sizeof(PacketHeader), inbox, inHdr.length);
    delete []buffer ;

    DEBUG('n', "网络接收到来自 %d 的数据包，长度 %d...\n",
	  				(int) inHdr.from, inHdr.length);
    stats->numPacketsRecvd++;

    // 通知邮局数据包已到达
    (*readHandler)(handlerArg);	
}

// 通知用户可以发送另一个数据包
void
Network::SendDone()
{
    sendBusy = FALSE;
    stats->numPacketsSent++;
    (*writeHandler)(handlerArg);
}

// 通过连接 hdr 和数据发送数据包，并安排
// 一个中断以通知用户何时可以发送下一个数据包 
//
// 注意，我们总是将数据包填充到 MaxWireSize，然后再放入
// 套接字，因为在接收端更简单。
void
Network::Send(PacketHeader hdr, char* data)
{
    char toName[32];

    ASSERT((sendBusy == FALSE) && (hdr.length > 0) 
		&& (hdr.length <= MaxPacketSize) && (hdr.from == ident));
    DEBUG('n', "发送到地址 %d，%d 字节... ", hdr.to, hdr.length);

    interrupt->Schedule(NetworkSendDone, (_int)this, NetworkTime, NetworkSendInt);

    if (Random() % 100 >= chanceToWork * 100) { // 模拟丢失的数据包
	DEBUG('n', "哎呀，丢失了！\n");
	return;
    }
    if (Random() % 100 >= chanceToNotDelay * 100) { // 模拟延迟
      // 为了延迟一个数据包，我们只需将其保存在缓冲区中
      // 它将保持在那里，直到另一个数据包被延迟，此时
      // 我们将其发送出去
      if (delayBufFull == TRUE) {
	SendToSocket(sock, delayBuf, MaxWireSize, delayToName);
      }
      sprintf(delayToName, "SOCKET_%d", (int)hdr.to);
      *(PacketHeader *)delayBuf = hdr;
      bcopy(data, delayBuf + sizeof(PacketHeader), hdr.length);
      delayBufFull = TRUE;
      return;
    }

    // 数据包既没有丢失也没有延迟 - 现在发送它

    sprintf(toName, "SOCKET_%d", (int)hdr.to);
    // 将 hdr 和数据连接到一个缓冲区中，并发送出去
    char *buffer = new char[MaxWireSize];
    *(PacketHeader *)buffer = hdr;
    bcopy(data, buffer + sizeof(PacketHeader), hdr.length);
    SendToSocket(sock, buffer, MaxWireSize, toName);
    delete []buffer;
}

// 读取一个数据包，如果有一个被缓冲
PacketHeader
Network::Receive(char* data)
{
    PacketHeader hdr = inHdr;

    inHdr.length = 0;
    if (hdr.length != 0)
    	bcopy(inbox, data, hdr.length);
    return hdr;
}
