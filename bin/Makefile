# 使用普通的 make 来构建这个 Makefile
#
# 这个 Makefile 用于：
#	coff2noff -- 将一个普通的 MIPS 可执行文件转换为 Nachos 可执行文件
#	disassemble -- 反汇编一个普通的 MIPS 可执行文件 
#

ifndef MAKEFILE_BIN
define MAKEFILE_BIN
yes
endef

include ../Makefile.dep

CFILES = coff2noff.c coff2flat.c

# 定义目标。这必须在 Makefile.common 之前，因为
# 它将定义目标 nachos，我们不希望它成为默认目标

# 根据我们所处的架构定义目标。反汇编
# 程序尚不支持 BIG_ENDIAN，例如 SPARC。

ifeq (,$(findstring HOST_MIPS,$(HOST)))
targets = $(bin_dir)/coff2noff $(bin_dir)/coff2flat
else
targets = $(bin_dir)/coff2noff $(bin_dir)/coff2flat $(bin_dir)/disassemble 
CFILES += out.c opstrings.c
endif

all : $(targets)

# Makefile.common 包含所有的规则。

include ../Makefile.common

INCPATH += -I. -I../threads
CFLAGS = $(GCCOPT32) $(INCPATH) $(DEFINES) $(HOST)
CC=gcc
LD=gcc

# 将 COFF 文件转换为 Nachos 对象格式
$(bin_dir)/coff2noff: $(obj_dir)/coff2noff.o

# 将 COFF 文件转换为平面对象格式
$(bin_dir)/coff2flat: $(obj_dir)/coff2flat.o

# 反汇编一个 COFF 文件
$(bin_dir)/disassemble: $(obj_dir)/out.o $(obj_dir)/opstrings.o

endif # MAKEFILE_BIN
