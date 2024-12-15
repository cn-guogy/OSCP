/* 指令格式 */

#define rd(i)		(((i) >> 11) & 0x1f)
#define rt(i)		(((i) >> 16) & 0x1f)
#define rs(i)		(((i) >> 21) & 0x1f)
#define shamt(i)	(((i) >> 6) & 0x1f)
#define immed(i)	(((i) & 0x8000) ? (i)|(-0x8000) : (i)&0x7fff)

#define off26(i)        (((i)&((1<<26)-1))<<2)
#define top4(i)         (((i)&(~((1<<28)-1))))
#define off16(i)        (immed(i)<<2)

#define extend(i, hibitmask)	(((i)&(hibitmask)) ? ((i)|(-(hibitmask))) : (i))
