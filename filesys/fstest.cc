// fstest.cc
//	简单的文件系统测试例程。
//
//	我们实现了：
//	   复制 -- 从 UNIX 复制文件到 Nachos
//	   打印 -- 显示 Nachos 文件的内容
//	   性能测试 -- 对 Nachos 文件系统的压力测试
//		以小块读取和写入一个非常大的文件
//		（在基线系统上无法工作！）
//

#include "utility.h"
#include "filesys.h"
#include "system.h"
#include "thread.h"
#include "disk.h"
#include "stats.h"

#define TransferSize 10 // 设置为小值，以增加难度

//----------------------------------------------------------------------
// 复制
// 	将 UNIX 文件 "from" 的内容复制到 Nachos 文件 "to"
//----------------------------------------------------------------------

void Copy(char *from, char *to)
{
    FILE *fp;
    OpenFile *openFile;
    int amountRead, fileLength;
    char *buffer;

    // 打开 UNIX 文件
    if ((fp = fopen(from, "r")) == NULL)
    {
        printf("复制: 无法打开输入文件 %s\n", from);
        return;
    }

    // 计算 UNIX 文件的长度
    fseek(fp, 0, 2);
    fileLength = ftell(fp);
    fseek(fp, 0, 0);

    // 创建一个相同长度的 Nachos 文件
    DEBUG('f', "正在复制文件 %s, 大小 %d, 到文件 %s\n", from, fileLength, to);
    if (!fileSystem->Create(to, fileLength))
    { // 创建 Nachos 文件
        printf("复制: 无法创建输出文件 %s\n", to);
        fclose(fp);
        return;
    }

    openFile = fileSystem->Open(to);
    ASSERT(openFile != NULL);

    // 以 TransferSize 大小块复制数据
    buffer = new char[TransferSize];
    while ((amountRead = fread(buffer, sizeof(char), TransferSize, fp)) > 0)
        openFile->Write(buffer, amountRead);
    delete[] buffer;

    // 关闭 UNIX 和 Nachos 文件
    delete openFile;
    fclose(fp);
}

//----------------------------------------------------------------------
// 打印
// 	打印 Nachos 文件 "name" 的内容。
//----------------------------------------------------------------------

void Print(char *name)
{
    OpenFile *openFile;
    int i, amountRead;
    char *buffer;

    if ((openFile = fileSystem->Open(name)) == NULL)
    {
        printf("打印: 无法打开文件 %s\n", name);
        return;
    }

    buffer = new char[TransferSize];
    while ((amountRead = openFile->Read(buffer, TransferSize)) > 0)
        for (i = 0; i < amountRead; i++)
            printf("%c", buffer[i]);
    delete[] buffer;

    delete openFile; // 关闭 Nachos 文件
    return;
}

//----------------------------------------------------------------------
// 性能测试
// 	通过创建一个大文件，逐步写入，逐步读取，然后删除文件来测试 Nachos 文件系统的压力。
//
//	实现为三个独立的例程：
//	  FileWrite -- 写入文件
//	  FileRead -- 读取文件
//	  PerformanceTest -- 总体控制，并打印性能数据
//----------------------------------------------------------------------

#define FileName (char *)"TestFile"
#define Contents (char *)"1234567890"
#define ContentSize (int)strlen(Contents)
#define FileSize ((int)(ContentSize * 5000))

static void
FileWrite()
{
    OpenFile *openFile;
    int i, numBytes;

    printf("顺序写入 %d 字节文件，以 %d 字节块进行\n",
           FileSize, ContentSize);
    if (!fileSystem->Create(FileName, 0))
    {
        printf("性能测试: 无法创建 %s\n", FileName);
        return;
    }
    openFile = fileSystem->Open(FileName);
    if (openFile == NULL)
    {
        printf("性能测试: 无法打开 %s\n", FileName);
        return;
    }
    for (i = 0; i < FileSize; i += ContentSize)
    {
        numBytes = openFile->Write(Contents, ContentSize);
        if (numBytes < 10)
        {
            printf("性能测试: 无法写入 %s\n", FileName);
            delete openFile;
            return;
        }
    }
    delete openFile; // 关闭文件
}

static void
FileRead()
{
    OpenFile *openFile;
    char *buffer = new char[ContentSize];
    int i, numBytes;

    printf("顺序读取 %d 字节文件，以 %d 字节块进行\n",
           FileSize, ContentSize);

    if ((openFile = fileSystem->Open(FileName)) == NULL)
    {
        printf("性能测试: 无法打开文件 %s\n", FileName);
        delete[] buffer;
        return;
    }
    for (i = 0; i < FileSize; i += ContentSize)
    {
        numBytes = openFile->Read(buffer, ContentSize);
        if ((numBytes < 10) || strncmp(buffer, Contents, ContentSize))
        {
            printf("性能测试: 无法读取 %s\n", FileName);
            delete openFile;
            delete[] buffer;
            return;
        }
    }
    delete[] buffer;
    delete openFile; // 关闭文件
}

void PerformanceTest()
{
    printf("开始文件系统性能测试:\n");
    stats->Print();
    FileWrite();
    FileRead();
    if (!fileSystem->Remove(FileName))
    {
        printf("性能测试: 无法删除 %s\n", FileName);
        return;
    }
    stats->Print();
}
