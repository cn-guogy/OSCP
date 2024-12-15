// 修改自lab4/fstest.cc

#include <sys/stat.h>

#include "utility.h"
#include "filesys.h"
#include "system.h"
#include "thread.h"
#include "disk.h"
#include "stats.h"

#include "directory.h"

#define TransferSize 10

// 设置修改时间
int SetCopyTime(char *from, OpenFile *openFile)
{
    struct stat st;

    if (stat(from, &st) == -1)
    {
        printf("%s: stat 错误\n", from);
        return 1;
    }
    openFile->SetModTime(st.st_mtime);
    openFile->WriteBack();
    return 0;
}

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

    // 创建一个与之相同长度的 Nachos 文件
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
    printf("复制数据成功\n");
    SetCopyTime(from, openFile);

    // 关闭 UNIX 和 Nachos 文件
    delete openFile;
    fclose(fp);
}

// 附加
void Append(char *from, char *to, int half)
{
    FILE *fp;
    OpenFile *openFile;
    int amountRead, fileLength;
    char *buffer;
    bool bDstExist = true;

    // 附加的起始位置
    int start;

    // 打开 UNIX 文件
    if ((fp = fopen(from, "r")) == NULL)
    {
        printf("附加: 无法打开输入文件 %s\n", from);
        return;
    }

    // 计算 UNIX 文件的长度
    fseek(fp, 0, 2);
    fileLength = ftell(fp);
    fseek(fp, 0, 0);

    if (fileLength == 0)
    {
        printf("附加: 文件 %s 没有内容可附加\n", from);
        return;
    }

    if ((openFile = fileSystem->Open(to)) == NULL)
    {
        bDstExist = false;
        // 文件 "to" 不存在，则创建一个
        if (!fileSystem->Create(to, 0))
        {
            printf("附加: 无法创建文件 %s 以附加\n", to);
            fclose(fp);
            return;
        }
        openFile = fileSystem->Open(to);
    }

    ASSERT(openFile != NULL);
    // 从 "start" 位置附加
    start = openFile->Length();
    if (half)
        start = start / 2;
    openFile->Seek(start);

    // 以 TransferSize 大小块附加数据
    buffer = new char[TransferSize];
    while ((amountRead = fread(buffer, sizeof(char), TransferSize, fp)) > 0)
    {
        int result;
        result = openFile->Write(buffer, amountRead);
        ASSERT(result == amountRead);
    }
    delete[] buffer;

    if (!bDstExist)
        SetCopyTime(from, openFile);

    // 将 inode 写回磁盘，因为我们已经更改了它
    openFile->WriteBack();

    // 关闭 UNIX 和 Nachos 文件
    delete openFile;
    fclose(fp);
}

void NAppend(char *from, char *to)
{
    OpenFile *openFileFrom;
    OpenFile *openFileTo;
    int amountRead, fileLength;
    char *buffer;
    bool bDstExist = true;

    // 附加的起始位置
    int start;

    if (!strncmp(from, to, FileNameMaxLen))
    {
        // "from" 应该与 "to" 不同
        printf("NAppend: 文件应不同\n");
        return;
    }

    if ((openFileFrom = fileSystem->Open(from)) == NULL)
    {
        // 文件 "from" 不存在，放弃
        printf("NAppend: 文件 %s 不存在\n", from);
        return;
    }

    fileLength = openFileFrom->Length();
    if (fileLength == 0)
    {
        printf("NAppend: 文件 %s 没有内容可附加\n", from);
        return;
    }

    if ((openFileTo = fileSystem->Open(to)) == NULL)
    {
        bDstExist = false;
        // 文件 "to" 不存在，则创建一个
        if (!fileSystem->Create(to, 0))
        {
            printf("附加: 无法创建文件 %s 以附加\n", to);
            delete openFileFrom;
            return;
        }
        openFileTo = fileSystem->Open(to);
    }

    ASSERT(openFileTo != NULL);
    // 从 "start" 位置附加
    start = openFileTo->Length();
    openFileTo->Seek(start);

    // 以 TransferSize 大小块附加数据
    buffer = new char[TransferSize];
    openFileFrom->Seek(0);
    while ((amountRead = openFileFrom->Read(buffer, TransferSize)) > 0)
    {
        int result;
        result = openFileTo->Write(buffer, amountRead);
        ASSERT(result == amountRead);
    }
    delete[] buffer;

    if (!bDstExist)
        openFileTo->SetModTime(openFileFrom->GetModTime());

    // 将 inode 写回磁盘，因为我们已经更改了它
    openFileTo->WriteBack();

    // 关闭两个 Nachos 文件
    delete openFileTo;
    delete openFileFrom;
}

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

#define FileName (char *)"TestFile"
#define Contents (char *)"1234567890"
#define ContentSize strlen(Contents)
#define FileSize ((int)(ContentSize * 100))

static void
FileWrite()
{
    OpenFile *openFile;
    int i, numBytes;

    printf("顺序写入 %d 字节文件，以 %d 字节块写入\n",
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

    // 将 inode 写回磁盘，因为我们已经更改了它
    openFile->WriteBack();

    delete openFile; // 关闭文件
}

static void
FileRead()
{
    OpenFile *openFile;
    char *buffer = new char[ContentSize];
    int i, numBytes;

    printf("顺序读取 %d 字节文件，以 %d 字节块读取\n",
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
