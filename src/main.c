#include <readline/history.h>
#include <readline/readline.h>
#include <stdint.h>
#include <sys/types.h>
#define NOB_IMPLEMENTATION
#include "../nob.h"
#include <linux/limits.h>
#include <readline/chardefs.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

extern void ash_setup();
extern char *ash_readline();
extern void ash_exit(int err);
extern char* ash_prompt;

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

struct aliases aliases = { 0 };

int unalias(char *cmd, struct Command *out)
{
	struct Command oc = { 0 };
	int sz = 0;
	int cap = 2;
	for (int i = 0; i < aliases.count; i++) {
		if (strcmp(cmd, aliases.items[i].name) == 0) {
			for (int j = 0; j < aliases.items[i].cmd.count; j++) {
				nob_da_append(&oc,
					      aliases.items[i].cmd.items[j]);
			}
			(*out) = oc;
			return i;
		}
	}
	nob_da_append(&oc, cmd);
	(*out) = oc;
	return -1;
}

bool unalias_rec(char *cmd, struct Command *out)
{
	int *used_aliases = malloc(sizeof(int) * aliases.count);
	int uasz = 0;
	struct Command cmd_tmp = { 0 };
	struct Command outf = { 0 };
	int amt = 0;
	bool ret = false;
	int is_alias = unalias(cmd, &outf);
	while (is_alias != -1) {
		ret = true;
		int br = 0;
		for (int i = 0; i < uasz; i++) {
			if (is_alias == used_aliases[i])
				br = 1;
		}
		if (br)
			break;
		used_aliases[uasz++] = is_alias;
		struct Command outfn = { 0 };
		char *c = strdup(outf.items[0]);
		int is_alias2 = unalias(c, &outfn);
		br = 0;
		for (int i = 0; i < uasz; i++) {
			if (is_alias2 == used_aliases[i])
				br = 1;
		}
		if (!br) {
			nob_da_append_many(&cmd_tmp, outfn.items, outfn.count);
			nob_da_append_many(&cmd_tmp, outf.items + 1,
					   outf.count - 1);
		} else {
			nob_da_append_many(&cmd_tmp, outf.items, outf.count);
		}
		outf = cmd_tmp;
	}
	(*out) = (struct Command){ 0 };
	nob_da_append_many(out, outf.items, outf.count);
	return ret;
}

// Function to split the line into arguments (tokenizing)
struct Command split_line(char *line)
{
	if (line == NULL)
		ash_exit(EXIT_SUCCESS);
	struct Command tokens = { 0 };
	char *token;
	token = strtok(line, " \t\r\n\a"); // Delimiters: space, tab, newline
	while (token != NULL) {
		nob_da_append(&tokens, token);
		token = strtok(NULL, " \t\r\n\a");
	}
	return tokens;
}

// Function to execute a command
int execute(struct Command *args)
{
	if (args->count == 0) {
		// Command doesn't exist
		return 1;
	}
	struct Command alias = {0};
	int isalias = unalias_rec(args->items[0], &alias);
	if (isalias) {
		struct Command newargs = {0};
		nob_da_append_many(&newargs, alias.items, alias.count);
		nob_da_append_many(&newargs, args->items+1, args->count-1);
		args->count = 0;
		nob_da_append_many(args, newargs.items, newargs.count);
	}
	// int i = 0;
	// while(args[i] != NULL) {
	// 	printf("%s\n", args[i]);
	// 	i++;
	// }

	if (strcmp(args->items[0], "exit") == 0) {
		ash_exit(EXIT_SUCCESS);
	}

	pid_t pid, wpid;
	int status;

	pid = fork(); // Create a child process
	if (pid == 0) {
		// Child process
		if (execvp(args->items[0], args->items) == -1) {
			perror(args->items[0]);
		}
		exit(EXIT_FAILURE);
	} else if (pid < 0) {
		// Error forking
		perror("shell");
	} else {
		// Parent process
		do {
			wpid = waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}

	return 1;
}

volatile sig_atomic_t sigstat = 0;

static void signal_handler(int sig)
{
	switch (sig) {
	case SIGINT:
		rl_replace_line("", 1);
		rl_on_new_line_with_prompt();
		printf("\n%s", ash_prompt);
		break;
	}
}

void setup_signals()
{
	struct sigaction new_action, old_action;
	new_action.sa_handler = signal_handler;
	sigemptyset(&new_action.sa_mask);
	new_action.sa_flags = SA_RESTART;

	sigaction(SIGINT, NULL, &old_action);
	if (old_action.sa_handler != SIG_IGN)
		sigaction(SIGINT, &new_action, NULL);
}

// Main loop
void shell_loop()
{
	char *line;
	struct Command args;
	int status;

	do {
		line = ash_readline(); // Read input
		args = split_line(line); // Parse input
		status = execute(&args); // Execute command
		nob_da_free(args);
	} while (status);
}

char *conf_file_name = "~/.ashrc";
int main(int argc, char **argv)
{
	char *program = nob_shift_args(&argc, &argv);
	while (argc > 0) {
		char *arg = nob_shift_args(&argc, &argv);
	}

	struct alias ls = {0};
	ls.name = "ls";
	ls.cmd = (struct Command){0};
	nob_da_append(&ls.cmd, "ls");
	nob_da_append(&ls.cmd, "--color=tty");
	nob_da_append(&aliases, ls);
	struct alias la = {0};
	la.name = "la";
	la.cmd = (struct Command){0};
	nob_da_append(&la.cmd, "ls");
	nob_da_append(&la.cmd, "-lAh");
	nob_da_append(&aliases, la);

	setup_signals();
	ash_setup();

	// Run command loop
	shell_loop();

	ash_exit(EXIT_SUCCESS);
}
