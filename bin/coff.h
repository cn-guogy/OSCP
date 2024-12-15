/* coff.h
 *   描述 MIPS COFF 格式的数据结构。
 */

#ifdef HOST_ALPHA /* 由于 gcc 使用 64 位长整型而需要 */
#define _long int /* DEC ALPHA 架构上的整数。 */
#else
#define _long long
#endif

struct filehdr
{
  unsigned short f_magic;  /* 魔数 */
  unsigned short f_nscns;  /* 节的数量 */
  _long f_timdat;          /* 时间和日期戳 */
  _long f_symptr;          /* 指向符号头的文件指针 */
  _long f_nsyms;           /* 符号头的大小 */
  unsigned short f_opthdr; /* 可选头的大小 */
  unsigned short f_flags;  /* 标志 */
};

#define MIPSELMAGIC 0x0162

#define OMAGIC 0407
#define SOMAGIC 0x0701

typedef struct aouthdr
{
  short magic;      /* 见上文                            */
  short vstamp;     /* 版本戳                        */
  _long tsize;      /* 文本大小（字节），填充到双字边界 */
  _long dsize;      /* 初始化数据大小                */
  _long bsize;      /* 未初始化数据大小             */
  _long entry;      /* 入口点                            */
  _long text_start; /* 此文件使用的文本基址      */
  _long data_start; /* 此文件使用的数据基址      */
  _long bss_start;  /* 此文件使用的 bss 基址       */
  _long gprmask;    /* 通用寄存器掩码        */
  _long cprmask[4]; /* 协处理器寄存器掩码          */
  _long gp_value;   /* 此对象使用的 gp 值    */
} AOUTHDR;
#define AOUTHSZ sizeof(AOUTHDR)

struct scnhdr
{
  char s_name[8];          /* 节名称 */
  _long s_paddr;           /* 物理地址，别名 s_nlib */
  _long s_vaddr;           /* 虚拟地址 */
  _long s_size;            /* 节大小 */
  _long s_scnptr;          /* 指向节的原始数据的文件指针 */
  _long s_relptr;          /* 指向重定位的文件指针 */
  _long s_lnnoptr;         /* 指向 gp 直方图的文件指针 */
  unsigned short s_nreloc; /* 重定位条目的数量 */
  unsigned short s_nlnno;  /* gp 直方图条目的数量 */
  _long s_flags;           /* 标志 */
};
