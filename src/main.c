#include <env.h>
#include <utils.h>
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
#include <shell.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

extern void ash_setup(char *ash_config);
extern char *ash_readline();
extern void ash_exit(int err);
extern char *get_prompt();
extern int unalias_rec(char *cmd, struct Command *out);
extern char *get_dir(char *npath, char *path, int tilde);

int cmd_status;

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
	if (tokens.count != 0) {
		struct Command alias = { 0 };
		int isalias = unalias_rec(tokens.items[0], &alias);
		if (isalias) {
			struct Command newtoks = { 0 };
			nob_da_append_many(&newtoks, alias.items, alias.count);
			nob_da_append_many(&newtoks, tokens.items + 1,
					   tokens.count - 1);
			tokens.count = 0;
			nob_da_append_many(&tokens, newtoks.items,
					   newtoks.count);
		}
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

	args->items[args->count] = NULL;

	if (strcmp(args->items[0], "exit") == 0) {
		ash_exit(EXIT_SUCCESS);
	}

	pid_t pid;
	pid_t wpid __attribute__((unused));

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
			wpid = waitpid(pid, &cmd_status, WUNTRACED);
		} while (!WIFEXITED(cmd_status) && !WIFSIGNALED(cmd_status));
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
		printf("\n%s", get_prompt());
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
	char __attribute__((unused)) *program = nob_shift_args(&argc, &argv);
	while (argc > 0) {
		char *arg = nob_shift_args(&argc, &argv);
		if (strcmp(arg, "-c") == 0) {
			printf("%d\n", argc);
			if (argc > 0) {
				conf_file_name = nob_shift_args(&argc, &argv);
				PERR("Filename expected after -c");
			}
		}
	}

	setup_signals();
	ash_setup(conf_file_name);

	// Run command loop
	shell_loop();

	ash_exit(EXIT_SUCCESS);
}
