/* MIPS 指令反汇编器 */

#include <stdio.h>
#include <filehdr.h>
#include <scnhdr.h>
#include <syms.h>
#include <ldfcn.h>
#include "int.h"

static FILE *fp;
static LDFILE *ldptr;
static SCNHDR texthead, rdatahead, datahead, sdatahead, sbsshead, bsshead;

static char filename[1000] = "a.out";	/* 默认 a.out 文件 */
static char self[256];			/* 调用程序的名称 */

char mem[MEMSIZE];		/* 主内存，稍后使用 malloc */
int TRACE, Traptrace, Regtrace;
int NROWS=64, ASSOC=1, LINESIZE=4, RAND=0, LRD=0;
int pc;

extern char *strcpy();

main(argc, argv)
int argc;
char *argv[];
{
	register char *s;
	char *fakeargv[3];

	strcpy(self, argv[0]);
	while  ( argc > 1  &&  argv[1][0] == '-' )
	{
		--argc; ++argv;
		for  ( s=argv[0]+1; *s != '\0'; ++s )
			switch  ( *s )
			{
			}
	}

	if (argc >= 2)
		strcpy(filename, argv[1]);
	fp = fopen(filename, "r");
	if (fp == NULL)
	{
		fprintf(stderr, "%s: 无法打开 '%s'\n", self, filename);
		exit(0);
	}
	fclose(fp);
	load_program(filename);
	if  ( argv[1] == NULL )
	{
		fakeargv[1] = "a.out";
		fakeargv[2] = NULL;
		argv = fakeargv;
		++argc;
	}
	disasm(memoffset, argc-1, argv+1); /* 程序正常开始的地方 */
}

#define LOADSECTION(head) load_section(&head);

load_section(hd)
register SCNHDR *hd;
{
  register int pc, i;
  if  ( hd->s_scnptr != 0 ) {
    /* printf("正在加载 %s\n", hd->s_name); */
    pc = hd->s_vaddr;
    FSEEK(ldptr, hd->s_scnptr, 0);
    for  ( i=0; i<hd->s_size; ++i ) {
      if (pc-memoffset >= MEMSIZE)
	{ printf("MEMSIZE 太小。请修复并重新编译。\n");
	  exit(1); }
      *(char *) ((mem-memoffset)+pc++) = getc(fp);
    }
  }
}

load_program(filename)
char *filename;
{
	ldptr = ldopen(filename, NULL);
	if  ( ldptr == NULL )
	{
		fprintf(stderr, "%s: 加载读取错误 %s\n", self, filename);
		exit(0);
	}
	if  ( TYPE(ldptr) != 0x162 )
	{
		fprintf(stderr,
			"大端对象文件（小端解释器）\n");
		exit(0);
	}

        if  ( ldnshread(ldptr, ".text", &texthead) != 1 )
                printf("文本节头缺失\n");
        else
                LOADSECTION(texthead)

        if  ( ldnshread(ldptr, ".rdata", &rdatahead) != 1 )
                printf("只读数据节头缺失\n");
        else
                LOADSECTION(rdatahead)

        if  ( ldnshread(ldptr, ".data", &datahead) != 1 )
                printf("数据节头缺失\n");
        else
                LOADSECTION(datahead)

        if  ( ldnshread(ldptr, ".sdata", &sdatahead) != 1 )
                printf("小数据节头缺失\n");
        else
                LOADSECTION(sdatahead)

        if  ( ldnshread(ldptr, ".sbss", &sbsshead) != 1 )
                printf("小未初始化数据节头缺失\n");
        else
                LOADSECTION(sbsshead)

        if  ( ldnshread(ldptr, ".bss", &bsshead) != 1 )
                printf("未初始化数据节头缺失\n");
        else
                LOADSECTION(bsshead)

	/* BSS 已经被清零（静态分配内存） */
	/* 此版本忽略重定位信息 */
}


int *m_alloc(n)
int n;
{
	extern char *malloc();

	return (int *) (int) malloc((unsigned) n);
}

disasm(startpc, argc, argv)
int startpc, argc;
char *argv[];
{
	int i;

	pc = memoffset;
	for  ( i=0; i<texthead.s_size; i += 4 ) 
	{
		dis1(pc);
		pc = pc + 4;
	}
}

dis1(xpc)
int xpc;
{
	register int instr;

	instr = fetch(pc);
	dump_ascii(instr, pc);
	printf("\n");
}
