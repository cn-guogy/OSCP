// utility.cc
//	调试例程。允许用户控制是否打印 DEBUG 语句，
//	基于命令行参数。
//

#include "utility.h"

// 这似乎取决于编译器的配置。
// 如果您在 va_start 上遇到问题，请尝试这两种替代方案
#include <stdarg.h>

static char *enableFlags = NULL; // 控制打印哪些 DEBUG 消息

//----------------------------------------------------------------------
// DebugInit
//      初始化，以便只有在 flagList 中有标志的 DEBUG 消息
//	会被打印。
//
//	如果标志是 "+"，我们启用所有 DEBUG 消息。
//
// 	"flagList" 是一个字符字符串，用于启用其 DEBUG 消息。
//----------------------------------------------------------------------

void DebugInit(char *flagList)
{
    enableFlags = flagList;
}

//----------------------------------------------------------------------
// DebugIsEnabled
//      如果 DEBUG 消息的 "flag" 应该被打印，则返回 TRUE。
//----------------------------------------------------------------------

bool DebugIsEnabled(char flag)
{
    if (enableFlags != NULL)
        return (bool)((strchr(enableFlags, flag) != 0) || (strchr(enableFlags, '+') != 0));
    else
        return FALSE;
}

//----------------------------------------------------------------------
// DEBUG
//      如果标志被启用，则打印调试消息。像 printf，
//	只是前面多了一个额外的参数。
//----------------------------------------------------------------------

void DEBUG(char flag, const char *format, ...)
{
    if (DebugIsEnabled(flag))
    {
        va_list ap;
        // 您会在这里收到一个未使用变量的消息 -- 忽略它。
        va_start(ap, format);
        vfprintf(stdout, format, ap);
        va_end(ap);
        fflush(stdout);
    }
}
