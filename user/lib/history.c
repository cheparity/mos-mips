#include <lib.h>
static int is_first_cmd;
// lines of .history
static int his_len;
// offset of lines in .history
static int hisoffsets[128];
void save_cmd_history(const char *s) {
	int r;
	int fd;

	if ((fd = open("/.history", O_WRONLY /*| O_APPEND*/)) < 0) {
		debugf("can't open file .history: %d\n", r);
		exit();
	}

	// if ((r = write(fd, s, strlen(s))) != strlen(s)) {
	// 	user_panic("write error .history: %d\n", r);
	// }

	// if ((r = write(fd, "\n", 1)) != 1) {
	// 	user_panic("write error .history: %d\n", r);
	// }

	if (is_first_cmd) {
		is_first_cmd = 0;
		if ((r = write(fd, s, strlen(s))) != strlen(s)) {
			user_panic("write error .history: %d\n", r);
		}
		if ((r = write(fd, "\n", 1)) != 1) {
			user_panic("write error .history: %d\n", r);
		}
		hisoffsets[his_len++] = strlen(s) + 1;
	} else {
		try(seek(fd, hisoffsets[his_len - 1]));
		if ((r = write(fd, s, strlen(s))) != strlen(s)) {
			user_panic("write error .history: %d\n", r);
		}
		if ((r = write(fd, "\n", 1)) != 1) {
			user_panic("write error .history: %d\n", r);
		}
		hisoffsets[his_len] = hisoffsets[his_len - 1] + strlen(s) + 1; // 递归加
		his_len++;
	}

	close(fd);
}

// 拿到第index条指令
int read_cmd_history(int cmd_offset, char *cmd) {
	int index;
	int r = 0;
	int fd;
	char buf[4096];
	int cmdlen = 0;
	if ((fd = open("/.history", O_RDONLY)) < 0) {
		user_panic("can't read file .history: %d", fd);
		exit();
	};
	index = his_len + cmd_offset;
	if (index < 0) {
		index = 0;
		cmd_offset = -his_len;
	}
	if (index > his_len) {
		index = his_len;
		cmd_offset = 0;
	}

	if (index != 0) {
		try(seek(fd, hisoffsets[index - 1]));
		cmdlen = hisoffsets[index] - hisoffsets[index - 1];
	} else {
		cmdlen = hisoffsets[index];
	}

	if ((r = readn(fd, buf, cmdlen)) != cmdlen) {
		user_panic("can't read file .history: %d", r);
		exit();
	}
	close(fd);
	buf[cmdlen - 1] = 0; // 增加'\0'
	// debugf("index:%d,buf:%s\n", index, buf);
	strcpy(cmd, buf);
	return cmd_offset;
}
