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

static struct {
	size_t capacity;
	size_t count;
	char **items;
} history;

uint64_t hist_srch_idx = 0;
char *hist_srch_str = "";

void history_append(char *l)
{
	nob_da_append(&history, l);
}

char *history_prev()
{
	if (hist_srch_idx > 0) {
		hist_srch_idx--;
	}
	if (hist_srch_idx == 0)
		return "";
	return history.items[history.count - hist_srch_idx];
}

char *history_next()
{
	if (hist_srch_idx < history.count) {
		hist_srch_idx++;
	}
	if (hist_srch_idx == 0)
		return "";
	return history.items[history.count - hist_srch_idx];
}

const char *default_prompt = "> ";

char *parse_env(char *str)
{
	struct CharArray newname = { 0 };
	int reading_env = 0;
	char *curenv = 0;
	for (int i = 0; i < strlen(str); i++) {
		char *p = str + i;
		if (reading_env) {
			if (*p == '}') {
				char *e;
				if ((e = getenv(curenv))) {
					nob_da_append_many(&newname, e,
							   strlen(e));
				} else {
					printf("Couldn't find environment "
					       "variable %s\n",
					       curenv);
				}
				reading_env = 0;
			}
		} else {
			if (*(p) == '$' && *(p + 1) == '{') {
				p++;
				reading_env = 1;
			} else {
				nob_da_append(&newname, *p);
			}
		}
	}
	return newname.items;
}

static struct {
	size_t capacity;
	size_t count;
	char **items;
} repls;

char *parse_repl(char *str)
{
	struct CharArray newname = { 0 };
	nob_da_append_many(&newname, str, strlen(str));
	for (int i = 0; i < repls.count; i++) {
		char *sp = strstr(newname.items, repls.items[i]);
		if (sp != NULL) {
			int ns = newname.items - sp;
			struct CharArray t_newname = { 0 };
			nob_da_append_many(&t_newname, newname.items, ns);
			nob_da_append_many(&t_newname, sp,
					   strlen(repls.items[i]));
			nob_da_append_many(
				&t_newname,
				newname.items + strlen(repls.items[i]),
				newname.count - (ns + strlen(repls.items[i])));
			newname.count = 0;
			nob_da_append_many(&newname, t_newname.items,
					   t_newname.count);
		}
	}
	return newname.items;
}

char *get_prompt()
{
	char *custom = getenv("PROMPT");
	if (custom)
		return custom;
	return (char *)default_prompt;
}

char *read_line(char *prompt)
{
	printf("%s", prompt);
	size_t cap = 32;
	size_t sz = 0;
	char *line = malloc(sizeof(char) * cap);
	char c = getc_unlocked(stdin);
	while (1) {
		if (c == EOF) {
			free(line);
			return NULL;
		}
		if (c == '\n' || c == '\r') {
			break;
		}
		line[sz] = c;
		sz++;
		if (sz >= cap) {
			cap *= 2;
			line = realloc(line, sizeof(char) * cap);
		}
		c = getc_unlocked(stdin);
	}
	line[sz] = '\0';
	return line;
}

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
		exit(EXIT_SUCCESS);
	struct Command tokens = { 0 };
	char *token;
	token = strtok(line, " \t\r\n\a"); // Delimiters: space, tab, newline
	struct Command atoken = { 0 };
	if (unalias_rec(line, &atoken)) {
		nob_da_append_many(&tokens, atoken.items, atoken.count);
		token = strtok(NULL, " \t\r\n\a");
	}
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

	// int i = 0;
	// while(args[i] != NULL) {
	// 	printf("%s\n", args[i]);
	// 	i++;
	// }

	if (strcmp(args->items[0], "exit") == 0) {
		exit(EXIT_SUCCESS);
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
		printf("\n%s", get_prompt());
		fflush(stdout);
		break;
	}
}

// Main loop
void shell_loop()
{
	char *line;
	struct Command args;
	int status;

	do {
		line = read_line(get_prompt()); // Read input
		history_append(line);
		args = split_line(line); // Parse input
		status = execute(&args); // Execute command

		free(line);
		nob_da_free(args);
	} while (status);
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

int load_config(char *filename)
{
	int pipefd[2];
	pid_t pid, wpid;
	int status;

	if (pipe(pipefd) == -1) {
		perror("pipe");
		exit(EXIT_FAILURE);
	}

	pid = fork(); // Create a child process
	if (pid == 0) {
		// Child process
		close(pipefd[0]);
		FILE *file = fopen(filename, "r");
		if (file == NULL) {
			perror(filename);
			return 1;
		}
		size_t read = 0;
		char* line = NULL;
		size_t len = 0;
		while ((read = getline(&line, &len, file)) != -1) {
			if (write(pipefd[1], line, read) == -1) {
				perror("write");
				exit(EXIT_FAILURE);
			}
		}
		fclose(file);
		close(pipefd[1]);
		exit(EXIT_SUCCESS);
	} else if (pid < 0) {
		// Error forking
		perror("shell");
	} else {
		// Parent process
		close(pipefd[1]);
		size_t len = 0;
		char *buf = malloc(1024);
		while((len = read(pipefd[0], buf, sizeof(buf)))) {
			char *line = strtok(buf, "\n\r");
			while(line != NULL) {
				char *var = strtok(line, "=");
				char *val = strtok(NULL, "=");
				setenv(var, val, 1);
				break;
				// line = strtok(buf, "\n\r");
			}
		}
		do {
			wpid = waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}
	return 0;
}

char *parse_filename(char *filename)
{
	struct CharArray newname = { 0 };
	int reading_env = 0;
	char *curenv = 0;
	for (int i = 0; i < strlen(filename); i++) {
		char *p = filename + i;
		if (reading_env) {
			if (*p == '}') {
				char *e;
				if ((e = getenv(curenv))) {
					nob_da_append_many(&newname, e,
							   strlen(e));
				} else {
					printf("Couldn't find environment "
					       "variable %s\n",
					       curenv);
				}
				reading_env = 0;
			}
		} else {
			if (*(p) == '$' && *(p + 1) == '{') {
				p++;
				reading_env = 1;
			} else if ((*p) == '~') {
				char *H = getenv("HOME");
				nob_da_append_many(&newname, H, strlen(H));
			} else {
				nob_da_append(&newname, *p);
			}
		}
	}
	return newname.items;
}

char *conf_file_name = "~/.ashrc";
int main(int argc, char **argv)
{
	char *program = nob_shift_args(&argc, &argv);
	while (argc > 0) {
		char *arg = nob_shift_args(&argc, &argv);
		if (strcmp(arg, "-c") == 0) {
			if (argc > 0) {
				char *f = nob_shift_args(&argc, &argv);
				load_config(f);
			} else {
				fprintf(stderr, "Error: Specify filename after "
						"'-c'\n");
				exit(EXIT_FAILURE);
			}
		}
	}

	// struct alias ls = { 0 };
	// ls.name = "ls";
	// ls.cmd = (struct Command){ 0 };
	// nob_da_append(&ls.cmd, "ls");
	// nob_da_append(&ls.cmd, "--color=tty");
	// nob_da_append(&aliases, ls);
	// struct alias la = { 0 };
	// la.name = "la";
	// la.cmd = (struct Command){ 0 };
	// nob_da_append(&la.cmd, "ls");
	// nob_da_append(&la.cmd, "-lAh");
	// nob_da_append(&aliases, la);

	if (load_config(parse_filename(conf_file_name)) != 0) {
		printf("Couldn't load config file, creating empty .ashrc...\n");
		FILE *_f = fopen(parse_filename("~/.ashrc"), "w+");
		fclose(_f);
	}

	setup_signals();

	// Run command loop
	shell_loop();

	return EXIT_SUCCESS;
}
