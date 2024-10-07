#ifndef _ASH_SHELL_H
#define _ASH_SHELL_H

#include <stdlib.h>

struct CharArray {
	size_t capacity;
	size_t count;
	char *items;
};

struct Command {
	size_t capacity;
	size_t count;
	char **items;
};

struct alias {
	char *name;
	struct Command cmd;
};

struct aliases {
	size_t capacity;
	size_t count;
	struct alias *items;
};

struct StrArray {
	size_t capacity;
	size_t count;
	char **items;
};

#endif
