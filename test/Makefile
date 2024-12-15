#
# Makefile 用于构建在 Nachos 上运行的用户程序
#
# 有几个需要注意的事项：
#
#    Nachos 假设程序启动例程的位置（内核在程序初始启动时跳转到的位置）
#       在位置 0。这意味着：start.o 必须是传递给 ld 的第一个 .o，
# 以便例程 "Start" 被加载到位置 0
#

# 在滑铁卢的更改：
#     o 交叉编译现在由 Makefile 自动处理。
#         - 在必要时，它假设 /software/gcc_nachos 存在，
#           这在 cayley、napier、descartes 上是正确的。
#
# 此文件已更改为 USQ 使用，以添加链接到 arch/bin
# 目录并允许从 .c 文件创建 .s 文件

ifndef MAKEFILE_TEST
define MAKEFILE_TEST
yes
endef

# 机器相关的内容
include ../Makefile.dep

real_bin_dir := $(bin_dir)

# 如果在不同平台上生成的用户程序是可互换的，
# 您可以取消注释以下行，然后只有一组用户程序文件将
# 在架构之间共享。这应该是这种情况（？）。但是，
# 似乎至少在 SPARC 上生成的用户程序在 MIPS 版本的 nachos 上
# 无法正确运行...

#arch = $(mips_arch)

# GCCDIR 将指向交叉编译器的位置（如果适用）。

CC = $(GCCDIR)gcc 
AS = $(GCCDIR)as 
LD = $(GCCDIR)ld

# 用户程序。在这里添加您自己的内容。
# 
# 注意：约定是每个目标恰好有一个 .c 文件。
#        目标是通过编译 .c 文件并将相应的 .o 与 start.o 链接来构建的。
#        如果您想要每个目标有多个 .c 文件，您需要更改下面的内容。

targets = halt halt2 shell matmult sort exec exit

# 目标放在特定架构的 'bin' 目录中。

all = $(targets:%=$(bin_dir)/%) 

all_coff = $(targets:%=$(obj_dir)/%.coff)
all_noff = $(all:%=%.noff)
all_flat = $(all:%=%.flat)

all: $(all_noff) $(all_flat)

$(targets): % : $(bin_dir)/%
	ln -sf $(bin_dir)/$@ $@

CFILES = $(targets:%=%.c)

SFILES = start.s

include ../Makefile.common

INCDIR =-I../userprog -I../threads
CFLAGS = -G 0 -c $(INCDIR)

coff2noff = ../bin/$(real_bin_dir)/coff2noff
coff2flat = ../bin/$(real_bin_dir)/coff2flat

$(all_coff): $(obj_dir)/%.coff: $(obj_dir)/start.o $(obj_dir)/%.o
	@echo ">>> 正在链接" $(obj_dir)/$(notdir $@) "<<<"
	$(LD) $(LDFLAGS) $^ -o $(obj_dir)/$(notdir $@)

$(all_noff): $(bin_dir)/%.noff: $(obj_dir)/%.coff
	@echo ">>> 正在转换为 noff 文件:" $@ "<<<"
	$(coff2noff) $^ $@
	ln -sf $@ $(notdir $@)

$(all_flat): $(bin_dir)/%.flat: $(obj_dir)/%.coff
	@echo ">>> 正在转换为平面文件:" $@ "<<<"
	$(coff2flat) $^ $@
	ln -sf $@ $(notdir $@)

%.s: %.c
	@echo ">>> 正在编译 .s 文件：" $< "<<<"
	$(CC) $(CFLAGS) -S -c -o $@ $<
endif # MAKEFILE_TEST
