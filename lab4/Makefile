# 注意：这是一个GNU Makefile。您必须使用“gmake”而不是“make”。
#
# 文件系统作业的Makefile
#    定义假设首先完成多程序和虚拟内存的设置。
#    如果没有，请使用下面的“基本骨架”定义。


ifndef MAKEFILE_FILESYS
define MAKEFILE_FILESYS
yes
endef

# 您可以重新排序分配。如果filesys在userprog之前，
# 只需重新排序并根据需要注释下面的包含。

include ../threads/Makefile.local

# 选择lab4不同的版本
include ../lab4/Makefile.local



#include ../userprog/Makefile.local
include ../filesys/Makefile.local

include ../Makefile.common

endif # MAKEFILE_FILESYS
