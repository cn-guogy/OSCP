/* coff2noff.c 
 *
 * 该程序读取 COFF 格式文件，并输出 NOFF 格式文件。
 * NOFF 格式本质上只是 COFF 文件的简化版本，
 * 记录每个段在 NOFF 文件中的位置，以及它在
 * 虚拟地址空间中的位置。
 * 
 * 假设 coff 文件是通过以下方式链接的：
 *	gld 使用 -N -Ttext 0 
 * 	ld 使用 -N -T 0
 * 以确保目标文件没有共享文本。
 *
 * 还假设 COFF 文件最多有 3 个段：
 *	.text	-- 只读可执行指令 
 *	.data	-- 已初始化数据
 *	.bss/.sbss -- 未初始化数据（在程序启动时应为零）
 */

#define MAIN
 
#undef MAIN
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include "coff.h"
#include "noff.h"
#include <string.h>
/* 用于将字和短字转换为模拟机器的小端格式的例程。
 * 当主机机器是小端时，这些最终成为 NOP。
 */

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

#define ReadStruct(f,s) 	Read(f,(char *)&s,sizeof(s))

// extern char *malloc();
char *noffFileName = NULL;

/* 读取并检查错误 */
void Read(int fd, char *buf, int nBytes)
{
    if (read(fd, buf, nBytes) != nBytes) {
        fprintf(stderr, "文件过短\n");
	unlink(noffFileName);
	exit(1);
    }
}

/* 写入并检查错误 */
void Write(int fd, char *buf, int nBytes)
{
    if (write(fd, buf, nBytes) != nBytes) {
	fprintf(stderr, "无法写入文件\n");
	unlink(noffFileName);
	exit(1);
    }
}

int main (int argc, char **argv)
{
    int fdIn, fdOut, numsections, i, inNoffFile;
    struct filehdr fileh;
    struct aouthdr systemh;
    struct scnhdr *sections;
    char *buffer;
    NoffHeader noffH;

    if (argc < 2) {
	fprintf(stderr, "用法: %s <coffFileName> <noffFileName>\n", argv[0]);
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
    if (fdIn == -1) {
	perror(argv[2]);
	exit(1);
    }
    noffFileName = argv[2];
    
/* 读取文件头并检查魔数。 */
    ReadStruct(fdIn,fileh);
    fileh.f_magic = ShortToHost(fileh.f_magic);
    fileh.f_nscns = ShortToHost(fileh.f_nscns); 
    if (fileh.f_magic != MIPSELMAGIC) {
	fprintf(stderr, "文件不是 MIPSEL COFF 文件\n");
        unlink(noffFileName);
	exit(1);
    }
    
/* 读取系统头并检查魔数 */
    ReadStruct(fdIn,systemh);
    systemh.magic = ShortToHost(systemh.magic);
    if (systemh.magic != OMAGIC) {
	fprintf(stderr, "文件不是 OMAGIC 文件\n");
        unlink(noffFileName);
	exit(1);
    }
    
/* 读取节头。 */
    numsections = fileh.f_nscns;
    printf("节的数量 %d \n",numsections);
    sections = (struct scnhdr *)malloc(numsections * sizeof(struct scnhdr));
    Read(fdIn, (char *) sections, numsections * sizeof(struct scnhdr));

   for (i = 0; i < numsections; i++) {
     sections[i].s_paddr =  WordToHost(sections[i].s_paddr);
     sections[i].s_size = WordToHost(sections[i].s_size);
     sections[i].s_scnptr = WordToHost(sections[i].s_scnptr);
   }

 /* 初始化 NOFF 头，以防 COFF 文件中未定义所有段 */
    noffH.noffMagic = NOFFMAGIC;
    noffH.code.size = 0;
    noffH.initData.size = 0;
    noffH.uninitData.size = 0;

 /* 复制段 */
    inNoffFile = sizeof(NoffHeader);
    lseek(fdOut, inNoffFile, 0);
    printf("加载 %d 个节:\n", numsections);
    for (i = 0; i < numsections; i++) {
	printf("\t\"%s\", 文件位置 0x%lx, 内存位置 0x%lx, 大小 0x%lx\n",
	      sections[i].s_name, sections[i].s_scnptr,
	      sections[i].s_paddr, sections[i].s_size);
	if (sections[i].s_size == 0) {
		/* 什么都不做！ */	
	} else if (!strcmp(sections[i].s_name, ".text")) {
	    noffH.code.virtualAddr = sections[i].s_paddr;
	    noffH.code.inFileAddr = inNoffFile;
	    noffH.code.size = sections[i].s_size;
    	    lseek(fdIn, sections[i].s_scnptr, 0);
    	    buffer = malloc(sections[i].s_size);
    	    Read(fdIn, buffer, sections[i].s_size);
    	    Write(fdOut, buffer, sections[i].s_size);
    	    free(buffer);
	    inNoffFile += sections[i].s_size;
 	} else if (!strcmp(sections[i].s_name, ".data")
	  		|| !strcmp(sections[i].s_name, ".rdata")) {
  	    /* 需要检查是否同时存在 .data 和 .rdata 
	     *  -- 确保其中一个是空的！ */ 
	    if (noffH.initData.size != 0) {
	        fprintf(stderr, "无法同时处理 data 和 rdata\n");
	        unlink(noffFileName);
	        exit(1);
	    }
	    noffH.initData.virtualAddr = sections[i].s_paddr;
	    noffH.initData.inFileAddr = inNoffFile;
	    noffH.initData.size = sections[i].s_size;
    	    lseek(fdIn, sections[i].s_scnptr, 0);
    	    buffer = malloc(sections[i].s_size);
    	    Read(fdIn, buffer, sections[i].s_size);
    	    Write(fdOut, buffer, sections[i].s_size);
    	    free(buffer);
	    inNoffFile += sections[i].s_size;
	} else if (!strcmp(sections[i].s_name, ".bss") ||
			!strcmp(sections[i].s_name, ".sbss")) {
  	    /* 需要检查是否同时存在 .bss 和 .sbss -- 确保它们 
	     * 是连续的
	     */
	    if (noffH.uninitData.size != 0) {
	        if (sections[i].s_paddr == (noffH.uninitData.virtualAddr +
	        				noffH.uninitData.size)) {
		    fprintf(stderr, "无法同时处理 bss 和 sbss\n");
		    unlink(noffFileName);
		    exit(1);
		}
	        noffH.uninitData.size += sections[i].s_size;
	    } else {
	        noffH.uninitData.virtualAddr = sections[i].s_paddr;
	        noffH.uninitData.size = sections[i].s_size;
	    }
	    /* 我们不需要复制未初始化的数据！ */
	} else {
	    fprintf(stderr, "未知段类型: %s\n", sections[i].s_name);
            unlink(noffFileName);
	    exit(1);
	}
    }
    lseek(fdOut, 0, 0);
    Write(fdOut, (char *)&noffH, sizeof(NoffHeader));
    close(fdIn);
    close(fdOut);
    exit(0);
}
