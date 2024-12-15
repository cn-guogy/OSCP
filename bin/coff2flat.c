/* 该程序读取 COFF 格式文件，并输出一个平面文件 --
 * 平面文件可以直接复制到虚拟内存并执行。
 * 换句话说，目标代码的各个部分被加载到
 * 平面文件中的适当偏移量。
 *
 * 假设 coff 文件使用 -N -T 0 编译，以确保它不是共享文本。
 */

/*
#include <filehdr.h>
#include <aouthdr.h>
#include <scnhdr.h>
#include <reloc.h>
#include <syms.h>
*/

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include "coff.h"
#include "noff.h"

/* 注意 -- 一旦你实现了大文件，可以将其增大！ */
#define StackSize      		1024      /* 以字节为单位 */
#define ReadStruct(f,s) 	Read(f,(char *)&s,sizeof(s))

// extern char *malloc();

unsigned int
WordToHost(unsigned int word) {
#ifdef HOST_IS_BIG_ENDIAN
	 register unsigned long result;
	 result = (word >> 24) & 0x000000ff;
	 result |= (word >> 8) & 0x0000ff00;
	 result |= (word << 8) & 0x00ff0000;
	 result |= (word << 24) & 0xff000000;
	 return result;
#else 
	 return word;
#endif /* HOST_IS_BIG_ENDIAN */
}

unsigned short
ShortToHost(unsigned short shortword) {
#if HOST_IS_BIG_ENDIAN
	 register unsigned short result;
	 result = (shortword << 8) & 0xff00;
	 result |= (shortword >> 8) & 0x00ff;
	 return result;
#else 
	 return shortword;
#endif /* HOST_IS_BIG_ENDIAN */
}

/* 读取并检查错误 */
void Read(int fd, char *buf, int nBytes)
{
    if (read(fd, buf, nBytes) != nBytes) {
	fprintf(stderr, "文件太短\n");
	exit(1);
    }
}

/* 写入并检查错误 */
void Write(int fd, char *buf, int nBytes)
{
    if (write(fd, buf, nBytes) != nBytes) {
	fprintf(stderr, "无法写入文件\n");
	exit(1);
    }
}

/* 执行实际工作 */
int main (int argc, char **argv)
{
    int fdIn, fdOut, numsections, i, top, tmp;
    struct filehdr fileh;
    struct aouthdr systemh;
    struct scnhdr *sections;
    char *buffer;

    if (argc < 2) {
	fprintf(stderr, "用法: %s <coff文件名> <平面文件名>\n", argv[0]);
	exit(1);
    }
    
/* 打开 COFF 文件（输入） */
    fdIn = open(argv[1], O_RDONLY, 0);
    if (fdIn == -1) {
	perror(argv[1]);
	exit(1);
    }

/* 打开 NOFF 文件（输出） */
    fdOut = open(argv[2], O_WRONLY|O_CREAT|O_TRUNC , 0666);
    if (fdOut == -1) {
	perror(argv[2]);
	exit(1);
    }
    
/* 读取文件头并检查魔数。 */
    ReadStruct(fdIn,fileh);
    fileh.f_magic = ShortToHost(fileh.f_magic);
    fileh.f_nscns = ShortToHost(fileh.f_nscns); 
    if (fileh.f_magic != MIPSELMAGIC) {
	fprintf(stderr, "文件不是 MIPSEL COFF 文件\n");
	exit(1);
    }
    
/* 读取系统头并检查魔数 */
    ReadStruct(fdIn,systemh);
    systemh.magic = ShortToHost(systemh.magic);
    if (systemh.magic != OMAGIC) {
	fprintf(stderr, "文件不是 OMAGIC 文件\n");
	exit(1);
    }
    
/* 读取节头。 */
    numsections = fileh.f_nscns;
    sections = (struct scnhdr *)malloc(fileh.f_nscns * sizeof(struct scnhdr));
    Read(fdIn, (char *) sections, fileh.f_nscns * sizeof(struct scnhdr));

 /* 复制段 */
    printf("加载 %d 个节:\n", fileh.f_nscns);
    for (top = 0, i = 0; i < fileh.f_nscns; i++) {
	printf("\t\"%s\", 文件位置 0x%lx, 内存位置 0x%lx, 大小 0x%lx\n",
	      sections[i].s_name, sections[i].s_scnptr,
	      sections[i].s_paddr, sections[i].s_size);
	if ((sections[i].s_paddr + sections[i].s_size) > top)
	    top = sections[i].s_paddr + sections[i].s_size;
	if (strcmp(sections[i].s_name, ".bss") && /* 如果是 .bss 则无需复制 */
	    	strcmp(sections[i].s_name, ".sbss")) {
	    lseek(fdIn, sections[i].s_scnptr, 0);
	    buffer = malloc(sections[i].s_size);
	    Read(fdIn, buffer, sections[i].s_size);
	    Write(fdOut, buffer, sections[i].s_size);
	    free(buffer);
	}
    }
/* 在末尾放一个空字，以便我们知道结束的位置！ */
    printf("添加大小为: %d 的栈\n", StackSize);
    lseek(fdOut, top + StackSize - 4, 0);
    tmp = 0;
    Write(fdOut, (char *)&tmp, 4);

    close(fdIn);
    close(fdOut);
}
