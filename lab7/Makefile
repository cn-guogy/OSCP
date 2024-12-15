# 注意：这是一个GNU Makefile。您必须使用“gmake”而不是“make”。
#
# 多程序设计作业的Makefile
#    定义假设多程序设计在文件系统之前完成的设置。
#    如果不是，请使用下面的“filesys first”定义。 
#
#

ifndef MAKEFILE_USERPROG
define MAKEFILE_USERPROG
yes
endef

# 如果文件系统作业在用户程序之前完成，则
# 取消注释下面的包含

include ../threads/Makefile.local
# include ../filesys/Makefile.local
include ../lab7/Makefile.local

include ../Makefile.common

endif # MAKEFILE_USERPROG
