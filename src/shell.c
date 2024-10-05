#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>

char *histfile;

extern char* get_prompt();

void ash_setup()
{
	rl_initialize();
	rl_editing_mode = 0;
	rl_catch_signals = 1;
	rl_set_signals();
	using_history();
	histfile = strdup(getenv("HOME"));
	strcat(histfile, "/.ashhistory");
	read_history(histfile);
	rl_cleanup_after_signal();
}

static char* line_read = NULL;

char *ash_readline(char *prompt)
{
	if (line_read) {
		free(line_read);
		line_read = NULL;
	}
	line_read = readline(prompt);
	if (line_read && *line_read)
		add_history(line_read);
	return line_read;
}

void ash_exit(int err)
{
	write_history(histfile);
	exit(err);
}
