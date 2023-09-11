#include "lib.h"

void main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(1, "usage: mkdir directory_name\n");
		return;
	}
	int fd;
	fd = open(argv[1], O_RDONLY);
	if (fd >= 0) {
		fprintf(1, "directory duplicated\n");
		return;
	}
	if (create(argv[1], FTYPE_DIR) < 0) {
		fprintf(1, "fail to create directory\n");
	}
}
