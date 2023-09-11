#include "lib.h"

char path[MAXPATHLEN];

char *strcat(char *dest, const char *src) {
	char *tmp = dest;

	while (*dest)
		dest++;
	while ((*dest++ = *src++) != '\0')
		;

	return tmp;
}

void tree(char *curdir, int indent) {
	int fd, n;
	struct File f;
	char buf[MAXNAMELEN];

	if ((fd = open(curdir, O_RDONLY)) < 0)
		user_panic("open %s: %e", curdir, fd);

	while ((n = readn(fd, &f, sizeof f)) == sizeof f) {
		if (f.f_name[0]) {
			for (int i = 0; i < indent; ++i) {
				fprintf(1, "|   ");
			}
			if (f.f_type == FTYPE_DIR) {
				fprintf(1, "|-- \x1b[36m\x1b[1m%s\x1b[0m\n", f.f_name);
				strcpy(buf, curdir);
				strcat(buf, f.f_name);
				strcat(buf, "/");
				tree(buf, indent + 1);
			} else {
				fprintf(1, "|-- \x1b[31m%s\x1b[0m\n", f.f_name);
			}
		}
	}

	close(fd);
	if (n)
		user_panic("error in directory %s: %e", curdir, n);
}

int main(int argc, char **argv) {
	int r;
	if (argc > 2) {
		fprintf(1, "usage: tree [path]\n");
		return 0;
	}
	struct Stat st;
	if (argc == 1) {
		char path[128];
		if ((r = getcwd(path)) < 0) {
			printf("cd failed: %d\n", r);
			exit();
		}
		if (strcmp(path, "/") != 0) {
			path[strlen(path)] = '/';
			path[strlen(path) + 1] = 0;
		}
		tree(path, 0);
	} else {
		strcpy(path, argv[1]);
		if (stat(path, &st) < 0) {
			fprintf(1, "cannot open directory\n");
			return 0;
		}
		if (!st.st_isdir) {
			fprintf(1, "specified path should be directory\n");
			return 0;
		}
		if (path[strlen(path) - 1] != '/')
			strcat(path, "/");
		tree(path, 0);
	}
	return 0;
}
