#include <args.h>
#include <lib.h>

#define WHITESPACE " \t\r\n"
#define SYMBOLS "<|>&;()"

/* Overview:
 *   Parse the next token from the string at s.
 *
 * Post-Condition:
 *   Set '*p1' to the beginning of the token and '*p2' to just past the token.
 *   Return:
 *     - 0 if the end of string is reached.
 *     - '<' for < (stdin redirection).
 *     - '>' for > (stdout redirection).
 *     - '|' for | (pipe).
 *     - 'w' for a word (command, argument, or file name).
 *
 *   The buffer is modified to turn the spaces after words into zero bytes
 * ('\0'), so that the returned token is a null-terminated string.
 */
int _gettoken(char *s, char **p1, char **p2) {
	*p1 = 0;
	*p2 = 0;
	if (s == 0) {
		return 0;
	}

	while (strchr(WHITESPACE, *s)) {
		*s++ = 0;
	}
	// 引号支持
	if (*s == '"') {
		*p1 = s + 1;
		s++;
		while (*s && *s != '"')
			s++;
		if (*s == 0) {
			debugf("\" does not match\n");
			return -1;
		}
		*s = 0;
		s++;
		while (*s && !strchr(WHITESPACE SYMBOLS, *s))
			s++;
		*p2 = s;
		return 'w';
	}

	if (*s == 0) {
		return 0;
	}

	if (strchr(SYMBOLS, *s)) {
		int t = *s;
		*p1 = s;
		*s++ = 0;
		*p2 = s;
		return t;
	}

	*p1 = s;
	while (*s && !strchr(WHITESPACE SYMBOLS, *s)) {
		s++;
	}
	*p2 = s;
	return 'w';
}

int gettoken(char *s, char **p1) {
	static int c, nc;
	static char *np1, *np2;

	if (s) {
		nc = _gettoken(s, &np1, &np2);
		return 0;
	}
	c = nc;
	*p1 = np1;
	nc = _gettoken(np2, &np1, &np2);
	return c;
}

#define MAXARGS 128

int parsecmd(char **argv, int *rightpipe) {
	int argc = 0;
	int pid, buf_len;
	while (1) {
		char *t;
		int fd;
		int c = gettoken(0, &t);
		switch (c) {
		case 0:
			return argc;
		case 'w':
			if (argc >= MAXARGS) {
				debugf("too many arguments\n");
				exit();
			}
			argv[argc++] = t;
			break;
		case ';':
			/*在 fork() 之后，子进程运行 ;之前的指令
			父进程等待子进程结束之后运行剩下部分的指令。*/
			if ((pid = fork()) == 0) {
				return argc; // 子进程退出
			}
			wait(pid); // 父进程等待
			argc = 0;  // 做一些清空
			buf_len = 0;
			rightpipe = 0;

			break;
		case '&':
			/*在 fork() 之后，子进程运行 ;之前的指令
			父进程等待子进程结束之后运行剩下部分的指令。*/
			if ((pid = fork()) == 0) {
				return argc; // 子进程退出
			}
			// wait(pid); // 父进程无需等待
			argc = 0; // 做一些清空
			buf_len = 0;
			rightpipe = 0;

			break;
		case '<':
			if (gettoken(0, &t) != 'w') {
				debugf("syntax error: < not followed by word\n");
				exit();
			}
			// Open 't' for reading, dup it onto fd 0, and then close the original fd.
			/* Exercise 6.5: Your code here. (1/3) */
			fd = open(t, O_RDONLY);

			break;
		case '>':
			if (gettoken(0, &t) != 'w') {
				debugf("syntax error: > not followed by word\n");
				exit();
			}
			// Open 't' for writing, dup it onto fd 1, and then close the original fd.
			/* Exercise 6.5: Your code here. (2/3) */
			fd = open(t, O_WRONLY);
			if (fd < 0) {
				if (create(t, FTYPE_REG) < 0) {
					debugf("> failed");
					exit();
				}
				fd = open(t, O_RDONLY);
			}

			dup(fd, 1);
			close(fd);

			break;
		case '|':;
			/*
			 * First, allocate a pipe.
			 * Then fork, set '*rightpipe' to the returned child envid or zero.
			 * The child runs the right side of the pipe:
			 * - dup the read end of the pipe onto 0
			 * - close the read end of the pipe
			 * - close the write end of the pipe
			 * - and 'return parsecmd(argv, rightpipe)' again, to parse the rest of
			 * the command line. The parent runs the left side of the pipe:
			 * - dup the write end of the pipe onto 1
			 * - close the write end of the pipe
			 * - close the read end of the pipe
			 * - and 'return argc', to execute the left of the pipeline.
			 */
			int p[2];
			/* Exercise 6.5: Your code here. (3/3) */
			pipe(p);
			*rightpipe = fork();
			if (*rightpipe == 0) {
				dup(p[0], 0);
				close(p[0]);
				close(p[1]);
				return parsecmd(argv, rightpipe);
			} else if (*rightpipe > 0) {
				dup(p[1], 1);
				close(p[1]);
				close(p[0]);
				return argc;
			}
			break;
		}
	}

	return argc;
}

void change_dir(char **argv) {
	int r;
	if (argv[1][0] == '/') {
		if ((r = chdir(argv[1])) < 0) {
			printf("cd failed: %d\n", r);
			exit();
		}
	} else {
		// Get curent direct path.
		char path[128];
		if ((r = getcwd(path)) < 0) {
			printf("cd failed: %d\n", r);
			exit();
		}

		pathcat(path, argv[1]);

		if ((r = open(path, O_RDONLY)) < 0) {
			printf("path %s doesn't exist: %d\n", path, r);
			exit();
		}
		close(r);
		struct Stat st;
		if ((r = stat(path, &st)) < 0) {
			user_panic("stat %s: %d", path, r);
		}
		if (!st.st_isdir) {
			printf("path %s is not a directory\n", path);
			exit();
		}

		if ((r = chdir(path)) < 0) {
			printf("cd failed: %d\n", r);
			exit();
		}
	}
}

void runcmd(char *s) {
	int r;
	gettoken(s, 0);
	char *argv[MAXARGS];
	int rightpipe = 0;
	int argc = parsecmd(argv, &rightpipe);
	if (argc == 0)
		return;
	argv[argc] = 0;
	// 如果是cd,则不创建子进程,特判
	if (strcmp(argv[0], "cd") == 0) {
		if (argc != 2) {
			printf("Usage: cd [path]\n");
			exit();
		} else {
			change_dir(argv);
			exit();
		}
	}
	char absolute_path[128];
	absolute_path[0] = '/';
	strcpy(absolute_path + 1, argv[0]);
	// pathcat(absolute_path, argv[0]);

	int child = spawn(absolute_path, argv);
	close_all();
	if (child >= 0)
		wait(child);
	else
		debugf("spawn %s: %d\n", argv[0], child);

	if (rightpipe)
		wait(rightpipe);
	exit();
}

void readline(char *buf, u_int n) {
	int cmd_offset = 0;
	int r;
	int cursor = 0;
	for (int i = 0; i < n; i++) {
		if ((r = read(0, buf + i, 1)) != 1) {
			if (r < 0)
				debugf("read error: %d\n", r);
			exit();
		}
		if (buf[i] == 126) {
			buf[i] = 0;
			for (int j = cursor + 1; j <= i; j++)
				buf[j - 1] = buf[j];
			for (int j = cursor; j < i; j++)
				printf("%c", buf[j]);
			printf(" \b");
			if (cursor < i - 1)
				MOVELEFT(i - cursor - 1);
			if (cursor == i)
				i--;
			else
				i -= 2;
		} else if (buf[i] == 127) {
			if (cursor == 0) {
				i--;
				continue;
			}

			buf[i] = 0;
			for (int j = cursor; j <= i; j++)
				buf[j - 1] = buf[j];
			MOVELEFT(1);
			cursor--;
			for (int j = cursor; j < i; j++)
				printf("%c", buf[j]);
			printf(" \b");

			if (cursor < i - 1)
				MOVELEFT(i - cursor - 1);
			if (cursor == i)
				i--;
			else
				i -= 2;
		} else if (buf[i] == '\x1b') {
			char tmp;
			read(0, &tmp, 1);
			if (tmp != '[')
				user_panic("\\x1b is not followed by '['");
			read(0, &tmp, 1);
			if (tmp == 'A') {
				// 上
				cmd_offset--;
				if (i)
					printf("\x1b[1B\x1b[%dD\x1b[K", i);
				else
					printf("\x1b[1B");
				cmd_offset = read_cmd_history(cmd_offset, buf);
				printf("%s", buf);
				cursor = i = strlen(buf);
			} else if (tmp == 'B') {
				// 下
				cmd_offset++;
				if (i)
					printf("\x1b[%dD\x1b[K", i);
				cmd_offset = read_cmd_history(cmd_offset, buf);
				printf("%s", buf);
				cursor = i = strlen(buf);
				// i = hist(buf, 0);
			} else if (tmp == 'C') {
				// 右
				if (cursor != i)
					cursor++;
			} else if (tmp == 'D') {
				// 左
				if (cursor != 0)
					cursor--;
			}
			i--; // 抵消i++
		} else if (buf[i] == '\r' || buf[i] == '\n') {
			buf[i] = 0;
			return;
		} else {
			char last = buf[i];
			for (int j = i; j > cursor; j--)
				buf[j] = buf[j - 1];
			buf[cursor] = last;
			// printf("%s", buf + cursor + 1);
			for (int j = cursor + 1; j <= i; j++)
				printf("%c", buf[j]);
			if (cursor < i)
				MOVELEFT(i - cursor);
			cursor++;
		}
	}
	debugf("line too long\n");
	while ((r = read(0, buf, 1)) == 1 && buf[0] != '\r' && buf[0] != '\n') {
		;
	}
	buf[0] = 0;
}

char buf[1024];

void usage(void) {
	debugf("usage: sh [-dix] [command-file]\n");
	exit();
}

int main(int argc, char **argv) {
	int r;
	int interactive = iscons(0);
	int echocmds = 0;
	debugf("\n:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
	debugf("::                                                         ::\n");
	debugf("::                     MOS Shell 2023                      ::\n");
	debugf("::                                                         ::\n");
	debugf(":::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
	ARGBEGIN {
	// 解析了命令中的选项,对本实验来说就是-i和-x
	case 'i': // 忽略大小写
		interactive = 1;
		break;
	case 'x': // 打印命令
		echocmds = 1;
		break;
	default:
		usage();
	}
	ARGEND

	// 文件系统
	if (create("/.history", FTYPE_REG) < 0) {
		debugf("create .history failed\n");
		// return 0;
		exit();
	}

	// 考虑了执行shell脚本的情况
	if (argc > 1)
		usage();
	// 如果需要执行脚本，则关闭标准输入，改为文件作为输入。
	if (argc == 1) {
		close(0);
		if ((r = open(argv[1], O_RDONLY)) < 0) {
			user_panic("open %s: %d", argv[1], r);
		}
		user_assert(r == 0);
	}
	// 在循环中不断读入命令行并进行处理。对于交互界面，会首先输出提示符
	// $。随后读入一行命令。
	for (;;) {
		if (interactive)
			printf("\n$ ");
		readline(buf, sizeof buf);
		save_cmd_history(buf); // 保存历史记录
		if (buf[0] == '#')
			continue;
		if (echocmds)
			printf("# %s\n", buf);
		if ((r = fork()) < 0)
			user_panic("fork: %d", r);
		if (r == 0) {
			runcmd(buf);
			exit();
		} else
			wait(r);
	}
	return 0;
}
