#include <lib.h>

int main(int argc, char **argv) {
	int r;
	char buf[128];

	if (argc != 1) {
		printf("usage: pwd\n");
		exit();
	} else {
		if ((r = getcwd(buf)) < 0) {
			printf("get path failed: %d\n", r);
			exit();
		}
		printf("%s\n", buf);
	}

	return 0;
}