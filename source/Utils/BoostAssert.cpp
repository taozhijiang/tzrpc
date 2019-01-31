#include <Utils/Log.h>

// 自定义assert，在触发的时候打印assert信息

namespace boost {
void assertion_failed(char const * expr, char const * function, char const * file, long line) {
    log_err("BAD!!! expr `%s` assert failed at %s(%ld): %s", expr, file, line, function);
}
} // 
