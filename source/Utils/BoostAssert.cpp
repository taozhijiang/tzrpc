/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#include <cstdio>
#include <Utils/Log.h>

// 自定义assert，在触发的时候打印assert信息

namespace boost {
void assertion_failed(char const * expr, char const * function, char const * file, long line) {
	fprintf(stderr, "BAD!!! expr `%s` assert failed at %s(%ld): %s", expr, file, line, function);
    log_err("BAD!!! expr `%s` assert failed at %s(%ld): %s", expr, file, line, function);
}

} // end namespace boost 
