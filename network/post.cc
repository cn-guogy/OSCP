// post.cc 
// 	处理传入网络消息并将其送达正确的
//	“地址”——一个邮箱，或一个用于存放传入消息的区域。
//	这个模块的工作方式就像美国邮政服务一样（换句话说，它能工作，但速度慢，你不能真的确定你的邮件是否真的送达！）。
//
//	请注意，一旦我们将 MailHdr 添加到发出的消息数据前面，
//	组合（MailHdr 加数据）在网络设备看来就像“数据”一样。
//
// 	该实现将传入消息与等待这些消息的线程进行同步。
//


#include "post.h"

//----------------------------------------------------------------------
// Mail::Mail
//      初始化单个邮件消息，通过将头部与
//	数据连接起来。
//
//	"pktH" -- 源，目标机器 ID
//	"mailH" -- 源，目标邮箱 ID
//	"data" -- 负载数据
//----------------------------------------------------------------------

Mail::Mail(PacketHeader pktH, MailHeader mailH, char *msgData)
{
    ASSERT(mailH.length <= MaxMailSize);

    pktHdr = pktH;
    mailHdr = mailH;
    bcopy(msgData, data, mailHdr.length);
}

//----------------------------------------------------------------------
// MailBox::MailBox
//      在邮局中初始化一个单独的邮箱，以便它
//	可以接收传入消息。
//
//	只需初始化一个消息列表，表示邮箱。
//----------------------------------------------------------------------


MailBox::MailBox()
{ 
    messages = new SynchList(); 
}

//----------------------------------------------------------------------
// MailBox::~MailBox
//      释放邮局中的单个邮箱。
//
//	只需删除邮箱，并丢弃邮箱中所有排队的消息。
//----------------------------------------------------------------------

MailBox::~MailBox()
{ 
    delete messages; 
}

//----------------------------------------------------------------------
// PrintHeader
// 	打印消息头——目标机器 ID 和邮箱
//	#，源机器 ID 和邮箱 #，以及消息长度。
//
//	"pktHdr" -- 源，目标机器 ID
//	"mailHdr" -- 源，目标邮箱 ID
//----------------------------------------------------------------------

static void 
PrintHeader(PacketHeader pktHdr, MailHeader mailHdr)
{
    printf("来自 (%d, %d) 到 (%d, %d) 字节 %d\n",
    	    pktHdr.from, mailHdr.from, pktHdr.to, mailHdr.to, mailHdr.length);
}

//----------------------------------------------------------------------
// MailBox::Put
// 	将消息添加到邮箱。如果有人在等待消息
//	到达，唤醒他们！
//
//	我们需要重建邮件消息（通过将头部与
//	数据连接起来），以简化在 SynchList 上排队消息。
//
//	"pktHdr" -- 源，目标机器 ID
//	"mailHdr" -- 源，目标邮箱 ID
//	"data" -- 负载消息数据
//----------------------------------------------------------------------

void 
MailBox::Put(PacketHeader pktHdr, MailHeader mailHdr, char *data)
{ 
    Mail *mail = new Mail(pktHdr, mailHdr, data); 

    messages->Append((void *)mail);	// 将其放在
					// 到达消息列表的末尾，并唤醒
					// 任何等待者
}

//----------------------------------------------------------------------
// MailBox::Get
// 	从邮箱中获取消息，将其解析为数据包头、
//	邮箱头和数据。
//	
//	如果邮箱中没有消息，调用线程将等待。
//
//	"pktHdr" -- 放置地址：源，目标机器 ID
//	"mailHdr" -- 放置地址：源，目标邮箱 ID
//	"data" -- 放置地址：负载消息数据
//----------------------------------------------------------------------

void 
MailBox::Get(PacketHeader *pktHdr, MailHeader *mailHdr, char *data) 
{ 
    DEBUG('n', "等待邮箱中的邮件\n");
    Mail *mail = (Mail *) messages->Remove();	// 从列表中移除消息；
						// 如果列表为空，将等待

    *pktHdr = mail->pktHdr;
    *mailHdr = mail->mailHdr;
    if (DebugIsEnabled('n')) {
	printf("从邮箱中获取邮件：");
	PrintHeader(*pktHdr, *mailHdr);
    }
    bcopy(mail->data, data, mail->mailHdr.length);
					// 将消息数据复制到
					// 调用者的缓冲区中
    delete mail;			// 我们已经复制了所需的内容，
					// 现在可以丢弃该消息
}

//----------------------------------------------------------------------
// PostalHelper, ReadAvail, WriteDone
// 	虚拟函数，因为 C++ 不能间接调用成员函数
//	第一个作为“邮政工作线程”的一部分被分叉；
//	后两个由网络中断处理程序调用。
//
//	"arg" -- 指向管理网络的邮局的指针
//----------------------------------------------------------------------

static void PostalHelper(_int arg)
{ PostOffice* po = (PostOffice *) arg; po->PostalDelivery(); }
static void ReadAvail(_int arg)
{ PostOffice* po = (PostOffice *) arg; po->IncomingPacket(); }
static void WriteDone(_int arg)
{ PostOffice* po = (PostOffice *) arg; po->PacketSent(); }

//----------------------------------------------------------------------
// PostOffice::PostOffice
// 	将邮局初始化为邮箱的集合。
//	还初始化网络设备，以允许不同机器上的邮局
//	相互传递消息。
//
//      我们使用一个单独的线程“邮政工作者”来等待消息
//	到达，并将其送到正确的邮箱。请注意，
//	将消息送到邮箱不能直接由中断处理程序完成，
//	因为这需要一个锁。
//
//	"addr" 是该机器的网络 ID 
//	"reliability" 是网络数据包被
//	  传递的概率（例如，reliability = 1 意味着网络从不丢弃任何数据包；reliability = 0 意味着网络从不传递任何数据包）
//      "orderability" 是网络数据包被传递而不延迟的概率（例如，orderability = 1
//        意味着传递的数据包从不延迟）
//	"nBoxes" 是此邮局中的邮箱数量
//----------------------------------------------------------------------

PostOffice::PostOffice(NetworkAddress addr, double reliability,
		       double orderability, int nBoxes)
{
// 首先，初始化与中断处理程序的同步
    messageAvailable = new Semaphore("消息可用", 0);
    messageSent = new Semaphore("消息已发送", 0);
    sendLock = new Lock("消息发送锁");

// 其次，初始化邮箱
    netAddr = addr; 
    numBoxes = nBoxes;
    boxes = new MailBox[nBoxes];

// 第三，初始化网络；告诉它要调用哪些中断处理程序
    network = new Network(addr, reliability, orderability,
			  ReadAvail, WriteDone, (_int) this);


// 最后，创建一个线程，其唯一工作是等待传入消息，
//   并将其放入正确的邮箱。 
    Thread *t = new Thread("邮政工作者");

    t->Fork(PostalHelper, (_int) this);
}

//----------------------------------------------------------------------
// PostOffice::~PostOffice
// 	释放邮局数据结构。
//----------------------------------------------------------------------

PostOffice::~PostOffice()
{
    delete network;
    delete [] boxes;
    delete messageAvailable;
    delete messageSent;
    delete sendLock;
}

//----------------------------------------------------------------------
// PostOffice::PostalDelivery
// 	等待传入消息，并将其放入正确的邮箱。
//
//      传入消息已经去掉了 PacketHeader，
//	但 MailHeader 仍然附加在数据的前面。
//----------------------------------------------------------------------

void
PostOffice::PostalDelivery()
{
    PacketHeader pktHdr;
    MailHeader mailHdr;
    char *buffer = new char[MaxPacketSize];

    for (;;) {
        // 首先，等待消息
        messageAvailable->P();	
        pktHdr = network->Receive(buffer);

        mailHdr = *(MailHeader *)buffer;
        if (DebugIsEnabled('n')) {
	    printf("将邮件放入邮箱：");
	    PrintHeader(pktHdr, mailHdr);
        }

	// 检查到达的消息是否合法！
	ASSERT(0 <= mailHdr.to && mailHdr.to < numBoxes);
	ASSERT(mailHdr.length <= MaxMailSize);

	// 放入邮箱
        boxes[mailHdr.to].Put(pktHdr, mailHdr, buffer + sizeof(MailHeader));
    }
}

//----------------------------------------------------------------------
// PostOffice::Send
// 	将 MailHeader 连接到数据的前面，并将
//	结果传递给网络以传递到目标机器。
//
//	请注意，MailHeader + 数据在网络看来就像正常的负载
//	数据一样。
//
//	"pktHdr" -- 源，目标机器 ID
//	"mailHdr" -- 源，目标邮箱 ID
//	"data" -- 负载消息数据
//----------------------------------------------------------------------

void
PostOffice::Send(PacketHeader pktHdr, MailHeader mailHdr, char* data)
{
    char* buffer = new char[MaxPacketSize];	// 用于保存连接的
						// mailHdr + 数据的空间

    if (DebugIsEnabled('n')) {
	printf("邮政发送：");
	PrintHeader(pktHdr, mailHdr);
    }
    ASSERT(mailHdr.length <= MaxMailSize);
    ASSERT(0 <= mailHdr.to && mailHdr.to < numBoxes);
    
    // 填充 pktHdr，以便于网络层
    pktHdr.from = netAddr;
    pktHdr.length = mailHdr.length + sizeof(MailHeader);

    // 连接 MailHeader 和数据
#ifdef HOST_ALPHA
    bcopy((const char *)&mailHdr, buffer, sizeof(MailHeader));
#else
    bcopy(&mailHdr, buffer, sizeof(MailHeader));
#endif
    bcopy(data, buffer + sizeof(MailHeader), mailHdr.length);

    sendLock->Acquire();   		// 任何时候只能发送一条消息
					// 到网络
    network->Send(pktHdr, buffer);
    messageSent->P();			// 等待中断告诉我们
					// 可以发送下一条消息
    sendLock->Release();

    delete [] buffer;			// 我们已经发送了消息，因此
					// 可以删除我们的缓冲区
}

//----------------------------------------------------------------------
// PostOffice::Send
// 	从特定邮箱中检索消息，如果有可用的消息，
//	否则等待消息到达该邮箱。
//
//	请注意，MailHeader + 数据在网络看来就像正常的负载
//	数据一样。
//
//	"box" -- 要查找消息的邮箱 ID
//	"pktHdr" -- 放置地址：源，目标机器 ID
//	"mailHdr" -- 放置地址：源，目标邮箱 ID
//	"data" -- 放置地址：负载消息数据
//----------------------------------------------------------------------

void
PostOffice::Receive(int box, PacketHeader *pktHdr, 
				MailHeader *mailHdr, char* data)
{
    ASSERT((box >= 0) && (box < numBoxes));

    boxes[box].Get(pktHdr, mailHdr, data);
    ASSERT(mailHdr->length <= MaxMailSize);
}

//----------------------------------------------------------------------
// PostOffice::IncomingPacket
// 	中断处理程序，当网络中到达数据包时调用。
//
//	通知 PostalDelivery 例程是时候开始工作了！
//----------------------------------------------------------------------

void
PostOffice::IncomingPacket()
{ 
    messageAvailable->V(); 
}

//----------------------------------------------------------------------
// PostOffice::PacketSent
// 	中断处理程序，当下一个数据包可以放入
//	网络时调用。
//
//	这个例程的名称是个误称；如果“reliability < 1”，
//	数据包可能已被网络丢弃，因此它不会通过。
//----------------------------------------------------------------------

void 
PostOffice::PacketSent()
{ 
    messageSent->V();
}
