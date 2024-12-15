/* noff.h
 *     定义 Nachos 对象代码格式的数据结构
 *
 *     基本上，我们只知道三种类型的段：
 *	代码（只读）、初始化数据和未初始化数据
 */

#define NOFFMAGIC 0xbadfad /* 魔数，表示 Nachos \
                            * 对象代码文件     \
                            */

typedef struct segment
{
  int virtualAddr; /* 段在虚拟地址空间中的位置 */
  int inFileAddr;  /* 段在此文件中的位置 */
  int size;        /* 段的大小 */
} Segment;

typedef struct noffHeader
{
  int noffMagic;      /* 应该是 NOFFMAGIC */
  Segment code;       /* 可执行代码段 */
  Segment initData;   /* 初始化数据段 */
  Segment uninitData; /* 未初始化数据段 --
                       * 使用前应清零
                       */
} NoffHeader;
