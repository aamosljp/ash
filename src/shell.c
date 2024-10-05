#include "../nob.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <utils.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <shell.h>

struct aliases aliases = { 0 };

int unalias(char *cmd, struct Command *out)
{
	struct Command oc = { 0 };
	int ret = -1;
	for (size_t i = 0; i < aliases.count; i++) {
		if (strcmp(cmd, aliases.items[i].name) == 0) {
			for (size_t j = 0; j < aliases.items[i].cmd.count;
			     j++) {
				nob_da_append(
					&oc,
					strdup(aliases.items[i].cmd.items[j]));
			}
			ret = i;
		}
	}
	if (oc.count == 0) {
		nob_da_append(&oc, cmd);
	}
	nob_da_append_many(out, oc.items, oc.count);
	return ret;
}

bool unalias_rec(char *cmd, struct Command *out)
{
	int *used_aliases = malloc(sizeof(int) * aliases.count);
	int uasz = 0;
	struct Command cmd_tmp = { 0 };
	struct Command outf = { 0 };
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
	free(used_aliases);
	nob_da_free(cmd_tmp);
	(*out) = (struct Command){ 0 };
	nob_da_append_many(out, outf.items, outf.count);
	nob_da_free(outf);
	return ret;
}

char *histfile;

const char *default_prompt = "> ";

char *ash_prompt;

char *get_prompt()
{
	char *custom = getenv("PROMPT");
	if (custom != NULL)
		return strdup(custom);
	return strdup(default_prompt);
}

char *parse_filename(char *str)
{
	struct CharArray out = { 0 };
	char *p = str;
	while ((*p) != 0) {
		if ((*p) == '~') {
			char *H = strdup(getenv("HOME"));
			nob_da_append_many(&out, H, strlen(H));
		} else {
			nob_da_append(&out, *p);
		}
		p++;
	}
	nob_da_append(&out, '\0');
	return out.items;
}

char *strip_quotes(char *s)
{
	char *r = strdup(s);
	if (STARTS_WITH(s, "\"") && ENDS_WITH(s, "\"")) {
		r[strlen(s) - 1] = '\0';
		r++;
	}
	return r;
}

void parse_configline(char *line)
{
	if (STARTS_WITH(line, "alias ")) {
		strtok(line, " ");
		char *name = strtok(NULL, " ");
		struct Command cmd = { 0 };
		char *a = NULL;
		while ((a = strtok(NULL, " ")) != NULL) {
			nob_da_append(&cmd, a);
		}
		struct alias r = { 0 };
		r.name = strdup(name);
		r.cmd = (struct Command){ 0 };
		nob_da_append_many(&r.cmd, cmd.items, cmd.count);
		nob_da_append(&aliases, r);
	} else if (strstr(line, "+=")) {
		char *env = strtok(line, "+=");
		if (env == NULL) {
			PERR("Error parsing config");
			return;
		}
		char *v = strtok(NULL, "+=");
		if (v == NULL) {
			PERR("Error parsing config");
			return;
		}
		v = strip_quotes(v);
		char *_v = strdup(getenv(env));
		strcat(_v, v);
		setenv(env, _v, 1);
	} else if (strstr(line, "=")) {
		char *env = strtok(line, "=");
		if (env == NULL) {
			PERR("Error parsing config");
			return;
		}
		char *v = strtok(NULL, "=");
		if (v == NULL) {
			PERR("Error parsing config");
			return;
		}
		v = strip_quotes(v);
		setenv(env, v, 1);
	}
}

void parse_config(char *ash_config)
{
	char *filename = parse_filename(ash_config);
	FILE *fp = fopen(filename, "r");
	if (fp == NULL) {
		PERR("%s: %s", filename, strerror(errno));
		return;
	}
	fseek(fp, 0, SEEK_END);
	size_t fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char *buf = malloc(fsize + 1);
	fread(buf, fsize, 1, fp);
	struct StrArray lines = { 0 };
	char *line = strtok(buf, "\n\r");
	while (line != NULL) {
		/* We need to strdup() here to avoid changing the pointer
		 * after passing it to parse_configline() */
		nob_da_append(&lines, strdup(line));
		line = strtok(NULL, "\n\r");
	}
	fclose(fp);
	if (line)
		free(line);
	if (buf)
		free(buf);
	for (size_t i = 0; i < lines.count; i++) {
		parse_configline(lines.items[i]);
	}
}

void ash_setup(char *ash_config)
{
	parse_config(ash_config);
	rl_initialize();
	rl_editing_mode = 0;
	rl_catch_signals = 1;
	rl_set_signals();
	using_history();
	histfile = strdup(getenv("HOME"));
	strcat(histfile, "/.ashhistory");
	read_history(histfile);
	rl_cleanup_after_signal();
	ash_prompt = get_prompt();
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
}

static char *line_read = NULL;

char *ash_readline()
{
	if (line_read) {
		free(line_read);
		line_read = NULL;
	}
	line_read = readline(ash_prompt);
	if (line_read && *line_read)
		add_history(line_read);
	return line_read;
}

void ash_exit(int err)
{
	write_history(histfile);
	exit(err);
}
