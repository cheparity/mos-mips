#include "lib.h"

int chdir(char *path) {
	return syscall_set_path(path);
}

int getcwd(char *buf) {
	return syscall_get_path(buf);
}

/// @brief 把相对路径拼接到进程所在路径下，得到绝对路径
/// @param path 进程所在路径
/// @param suffix 相对路径
void pathcat(char *path, const char *suffix) {
	int pre_len = strlen(path);

	// 如果是..或者../
	// if (suffix[0] == '.' && suffix[1] == '.') {
	// 	suffix += (suffix[2] == '/') ? 3 : 2;
	// 	// 把path的上一个/删掉,如path='/abc/efg
	// 	for (int i = pre_len; path[i] != '/'; i--)
	// 		;
	// }

	if (suffix[0] == '.' && suffix[1] == '/') {
		suffix += 2;
	}
	int suf_len = strlen(suffix);

	if (pre_len != 1) {
		path[pre_len++] = '/';
	}
	// 拼接
	for (int i = 0; i < suf_len; i++) {
		path[pre_len + i] = suffix[i];
	}

	path[pre_len + suf_len] = 0;
}