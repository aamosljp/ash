#define NOB_IMPLEMENTATION
#include "nob.h"

#define CC "clang"
#define EXEC "ash"
#define INCL "-Iinclude"
#define CFLAGS "-ggdb", "-DDEBUG"
#define LIBS "-lreadline"

bool debug = false;
bool test = false;

char *ext(const char *str)
{
	return strchr(str, '.');
}

char *no_ext(char *str)
{
	return strtok(str, ".");
}

void build_c(Nob_Cmd *c, char *path)
{
	if (nob_get_file_type(path) == NOB_FILE_DIRECTORY)
		return;
	char *t = strdup(path);
	if (strcmp(ext(t), ".c"))
		return;
	char *out = strdup(path);
	out = strcat(no_ext(out), ".o");
	nob_cmd_append(c, out);
	if (!nob_needs_rebuild1(out, path))
		return;
	Nob_Cmd cmd = { 0 };
	nob_cmd_append(&cmd, CC);
#ifdef CFLAGS
	nob_cmd_append(&cmd, CFLAGS);
#endif
	nob_cmd_append(&cmd, "-c");
	nob_cmd_append(&cmd, INCL);
	nob_cmd_append(&cmd, "-Wall", "-Wextra");
	nob_cmd_append(&cmd, "-o", out);
	nob_cmd_append(&cmd, path);
	nob_cmd_run_sync(cmd);
	nob_cmd_free(cmd);
}

void build_s(Nob_Cmd *c, char *path)
{
	if (nob_get_file_type(path) == NOB_FILE_DIRECTORY)
		return;
	char *t = strdup(path);
	if (strcmp(ext(t), ".s"))
		return;
	char *out = strdup(path);
	out = strcat(no_ext(out), ".o");
	nob_cmd_append(c, out);
	if (!nob_needs_rebuild1(out, path))
		return;
	Nob_Cmd cmd = { 0 };
	nob_cmd_append(&cmd, "as");
	nob_cmd_append(&cmd, "-g");
	nob_cmd_append(&cmd, "-o", out);
	nob_cmd_append(&cmd, path);
	nob_cmd_run_sync(cmd);
	nob_cmd_free(cmd);
}

int main(int argc, char **argv)
{
	NOB_GO_REBUILD_URSELF(argc, argv);

	nob_shift_args(&argc, &argv);
	Nob_Cmd cmd = { 0 };
	nob_cmd_append(&cmd, CC);
	nob_cmd_append(&cmd, "-o", EXEC);
	Nob_File_Paths ch = { 0 };
	nob_read_entire_dir("src", &ch);
	Nob_Cmd x = { 0 };
	for (int i = 0; i < ch.count; i++) {
		if (ch.items[i][0] == '.')
			continue;
		if (strcmp(ext(ch.items[i]), ".c"))
			continue;
		char *f = malloc(sizeof(ch.items[i]) + 5);
		strcpy(f, "src/");
		strcpy(f + 4, ch.items[i]);
		build_c(&x, f);
	}
	if (nob_needs_rebuild(EXEC, x.items, x.count)) {
		for (int i = 0; i < x.count; i++) {
			nob_cmd_append(&cmd, x.items[i]);
		}
#ifdef LIBS
		nob_cmd_append(&cmd, LIBS);
#endif
#ifdef LDFLAGS
		nob_cmd_append(&cmd, LDFLAGS);
#endif
		if (!nob_cmd_run_sync(cmd))
			return 1;
	}
	while (argc > 0) {
		char *arg = nob_shift_args(&argc, &argv);
		if (strcmp(arg, "run") == 0) {
			cmd.count = 0;
			nob_cmd_append(&cmd, "./"EXEC);
			nob_da_append_many(&cmd, argv, argc);
			if (!nob_cmd_run_sync(cmd))
				return 1;
		}
	}
	return 0;
}
