// post.h 
//	数据结构，用于提供不可靠、无序、固定大小的消息传递抽象，
//	将消息发送到其他（直接连接的）机器上的邮箱。消息可能会被网络丢弃或延迟，
//	但永远不会被损坏。
//
// 	美国邮政局将邮件投递到指定的邮箱。
// 	类比而言，我们的邮局将数据包投递到特定的缓冲区
// 	（MailBox），基于存储在数据包头中的邮箱号码。
// 	邮件在邮箱中等待，直到线程请求它；如果邮箱为空，
// 	线程可以等待邮件到达。
//
// 	因此，我们的邮局提供的服务是对传入的数据包进行解复用，
// 	将它们传递给适当的线程。
//
//      每条消息都有一个返回地址，包括一个“发件地址”，
// 	这是发送消息的机器的ID，以及一个“发件箱”，
// 	这是发送机器上一个邮箱的号码，如果你的协议需要，
// 	你可以向该邮箱发送确认。
//

#ifndef POST_H
#define POST_H

#include "network.h"
#include "synchlist.h"

// 邮箱地址 -- 唯一标识给定机器上的一个邮箱。
// 邮箱只是消息的临时存储位置。
typedef int MailBoxAddress;

// 以下类定义了消息头的一部分。
// 这是由邮局在消息发送到网络之前添加到消息中的。

class MailHeader {
  public:
    MailBoxAddress to;		// 目标邮箱
    MailBoxAddress from;	// 回复的邮箱
    unsigned length;		// 消息数据的字节数（不包括
				// 邮件头）
};

// 单个消息中可以包含的最大“有效载荷” -- 实际数据
// 不包括MailHeader和PacketHeader

#define MaxMailSize 	(MaxPacketSize - sizeof(MailHeader))


// 以下类定义了进出“邮件”消息的格式。
// 消息格式是分层的：
//	网络头（PacketHeader）
//	邮局头（MailHeader）
//	数据

class Mail {
  public:
     Mail(PacketHeader pktH, MailHeader mailH, char *msgData);
				// 通过将头部与数据连接来初始化邮件消息

     PacketHeader pktHdr;	// 网络附加的头部
     MailHeader mailHdr;	// 邮局附加的头部
     char data[MaxMailSize];	// 有效载荷 -- 消息数据
};

// 以下类定义了一个单一的邮箱，或消息的临时存储。
// 传入的消息由邮局放入适当的邮箱，这些消息可以被
// 该机器上的线程检索。

class MailBox {
  public: 
    MailBox();			// 分配并初始化邮箱
    ~MailBox();			// 释放邮箱

    void Put(PacketHeader pktHdr, MailHeader mailHdr, char *data);
   				// 原子性地将消息放入邮箱
    void Get(PacketHeader *pktHdr, MailHeader *mailHdr, char *data); 
   				// 原子性地从邮箱中获取消息
				// （如果没有消息可获取，则等待！）
  private:
    SynchList *messages;	// 邮箱只是到达消息的列表
};

// 以下类定义了一个“邮局”，或一组邮箱。
// 邮局是一个同步对象，提供两个主要操作：发送 -- 将消息发送到远程
// 机器上的邮箱，以及接收 -- 等待直到邮箱中有消息，
// 然后移除并返回它。
//
// 传入的消息由邮局放入适当的邮箱，唤醒任何在接收上等待的线程。

class PostOffice {
  public:
    PostOffice(NetworkAddress addr, double reliability,
	       double orderability, int nBoxes);
				// 分配并初始化邮局
				//   “可靠性”是底层网络丢弃的包的数量
    ~PostOffice();		// 释放邮局数据
    
    void Send(PacketHeader pktHdr, MailHeader mailHdr, char *data);
    				// 将消息发送到远程机器上的邮箱。
// 	MailHeader中的fromBox是确认的返回箱。
    
    void Receive(int box, PacketHeader *pktHdr, 
		MailHeader *mailHdr, char *data);
    				// 从“box”中检索消息。如果
				// 邮箱中没有消息，则等待。

    void PostalDelivery();	// 等待传入消息， 
				// 然后将它们放入正确的邮箱

    void PacketSent();		// 中断处理程序，当传出的 
				// 数据包已放入网络时调用；下一个 
				// 数据包现在可以发送
    void IncomingPacket();	// 中断处理程序，当传入的
   				// 数据包到达并可以被提取时调用
				// （即，是时候调用 
				// PostalDelivery）

  private:
    Network *network;		// 物理网络连接
    NetworkAddress netAddr;	// 该机器的网络地址
    MailBox *boxes;		// 存放传入邮件的邮箱表
    int numBoxes;		// 邮箱数量
    Semaphore *messageAvailable;// 当消息已从网络到达时被V
    Semaphore *messageSent;	// 当下一个消息可以发送到网络时被V
    Lock *sendLock;		// 每次只能有一个传出的消息
};

#endif
