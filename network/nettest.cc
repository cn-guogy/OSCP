// nettest.cc 
//	测试两个 "Nachos" 机器之间的消息传递，
//	使用邮局协调交付。
//
//	一个注意事项：
//	  1. 必须运行两个 Nachos 副本，机器 ID 为 0 和 1：
//		./nachos -m 0 -o 1 &
//		./nachos -m 1 -o 0 &
//

#include "system.h"
#include "network.h"
#include "post.h"
#include "interrupt.h"

// 测试消息传递，通过以下步骤：
//	1. 向 ID 为 "farAddr" 的机器发送消息，邮筒 #0
//	2. 等待另一台机器的消息到达（在我们的邮筒 #0 中）
//	3. 发送对另一台机器消息的确认
//	4. 等待另一台机器对我们原始消息的确认

void
MailTest(int farAddr)
{
    PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char *data = (char*)"你好！";
    char *ack = (char*)"收到！";
    char buffer[MaxMailSize];

    // 构造原始消息的包和邮件头
    // 收件人：目标机器，邮筒 0
    // 发件人：我们的机器，回复到：邮筒 1
    outPktHdr.to = farAddr;		
    outMailHdr.to = 0;
    outMailHdr.from = 1;
    outMailHdr.length = strlen(data) + 1;

    // 发送第一条消息
    postOffice->Send(outPktHdr, outMailHdr, data); 

    // 等待来自另一台机器的第一条消息
    postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
    printf("从 %d, 邮箱 %d 收到 \"%s\"\n", inPktHdr.from, inMailHdr.from, buffer);
    fflush(stdout);

    // 向另一台机器发送确认（使用刚到达消息中的 "回复到" 邮箱）
    outPktHdr.to = inPktHdr.from;
    outMailHdr.to = inMailHdr.from;
    outMailHdr.length = strlen(ack) + 1;
    postOffice->Send(outPktHdr, outMailHdr, ack); 

    // 等待来自另一台机器的对我们发送的第一条消息的确认。
    postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer);
    printf("从 %d, 邮箱 %d 收到 \"%s\"\n", inPktHdr.from, inMailHdr.from, buffer);
    fflush(stdout);

    // 然后我们完成了！
    interrupt->Halt();
}
