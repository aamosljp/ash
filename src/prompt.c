#include "../nob.h"
#include <ctype.h>
#include <utils.h>
#include <bits/posix1_lim.h>
#include <stdio.h>
#include <stdlib.h>
#include <shell.h>
#include <file.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/utsname.h>

extern int cmd_status;

struct prompt_var {
	char *var_name;
};

struct prompt_cmd {
	char *cmd_name;
	struct prompt_elems *args;
};

enum prompt_etype {
	ETYPE_CHAR,
	ETYPE_VAR,
	ETYPE_CMD,
};

struct prompt_elem {
	struct prompt_elem *prev;
	enum prompt_etype etype;
	union {
		struct prompt_cmd cmd;
		struct prompt_var var;
		struct CharArray c;
	};
	struct prompt_elem *next;
};

struct prompt_elems {
	size_t capacity;
	size_t count;
	struct prompt_elem *items;
};

char *timefmt()
{
	char *tfenv = getenv("TIME_FORMAT");
	if (tfenv != NULL)
		return strdup(tfenv);
	return "%H:%M:%S";
}

char *datefmt()
{
	char *dfenv = getenv("DATE_FORMAT");
	if (dfenv != NULL)
		return strdup(dfenv);
	return "%Y-%m-%d";
}

char *vartime()
{
	time_t t = time(NULL);
	struct tm *tmi = localtime(&t);
	char *fmt = timefmt();
	char *buffer = malloc(strlen(fmt) + 1);
	strftime(buffer, strlen(fmt) + 1, fmt, tmi);
	return buffer;
}

char *varuser()
{
	struct passwd *pw = getpwuid(getuid());
	char *ue = strdup(getenv("USER"));
	return pw ? pw->pw_name : (ue != NULL) ? ue : "unknown";
}

char *varhostname()
{
	static char hostname[HOST_NAME_MAX + 1];
	if (gethostname(hostname, sizeof(hostname)) == 0) {
		return hostname;
	}
	char *he = strdup(getenv("HOSTNAME"));
	return (he != NULL) ? he : "unknown";
}

static char *promptpath(char *p, int tilde)
{
	char *np = NULL;
	np = get_dir(np, p, tilde);
	return np;
}

char *varworkingdir()
{
	static char cwd[PATH_MAX];
	if (getcwd(cwd, sizeof(cwd)) != NULL) {
		return promptpath(cwd, 1);
	}
	char *ce = strdup(getenv("PWD"));
	return (ce != NULL) ? promptpath(cwd, 1) : "unknown";
}

char *varworkingdir2()
{
	static char cwd[PATH_MAX];
	if (getcwd(cwd, sizeof(cwd)) != NULL) {
		return promptpath(cwd, 0);
	}
	char *ce = strdup(getenv("PWD"));
	return (ce != NULL) ? promptpath(cwd, 0) : "unknown";
}

char *vardate()
{
	time_t t = time(NULL);
	struct tm *tmi = localtime(&t);
	char *fmt = datefmt();
	char *buffer = malloc(strlen(fmt) + 4);
	strftime(buffer, strlen(fmt) + 4, fmt, tmi);
	return buffer;
}

char *varerrcode()
{
	char *r = NULL;
	sprintf(r, "%d", cmd_status);
	return r;
}

char *(*var_func[7])(void) = { varuser,	       varhostname, varworkingdir,
			       varworkingdir2, vartime,	    vardate,
			       varerrcode };
char *var_names[7] = { "u", "h", "w", "W", "t", "d", "?" };

#define VARCOUNT 7

int get_prompt_var(char *name)
{
	for (int i = 0; i < VARCOUNT; i++) {
		if (strcmp(name, var_names[i]) == 0)
			return i;
	}
	return -1;
}

#define COLORCOUNT 8
char *color_vals_bg[COLORCOUNT] = { COLOR_BG_BLACK, COLOR_BG_RED,
				    COLOR_BG_GREEN, COLOR_BG_YELLOW,
				    COLOR_BG_BLUE,  COLOR_BG_MAGENTA,
				    COLOR_BG_CYAN,  COLOR_BG_WHITE };
char *color_vals_fg[COLORCOUNT] = { COLOR_FG_BLACK, COLOR_FG_RED,
				    COLOR_FG_GREEN, COLOR_FG_YELLOW,
				    COLOR_FG_BLUE,  COLOR_FG_MAGENTA,
				    COLOR_FG_CYAN,  COLOR_FG_WHITE };
char *color_names[COLORCOUNT] = { "black", "red",     "green", "yellow",
				  "blue",  "magenta", "cyan",  "white" };

int parse_color(char *s)
{
	for (int i = 0; i < COLORCOUNT; i++) {
		if (strcmp(s, color_names[i]) == 0)
			return i;
	}
	return -1;
}

char *cmd_fg(struct prompt_elems *args)
{
	char *color = NULL;
	if (args->count == 1) {
		if (args->items[0].etype != ETYPE_CHAR) {
			fprintf(stderr, "Color has to be a constant\n");
			return NULL;
		}
		args->items[0].c.items[args->items[0].c.count] = 0;
		int cs = parse_color(args->items[0].c.items);
		if (cs == -1) {
			fprintf(stderr, "Invalid color constant %s\n",
				args->items[0].c.items);
			return NULL;
		}
		color = strdup(color_vals_fg[cs]);
	}
	return color;
}

char *cmd_fg_bold(struct prompt_elems *args)
{
	char *color = cmd_fg(args);
	char *ret = malloc(strlen(color) + strlen(STYLE_BOLD));
	strcpy(ret, color);
	strcpy(ret + strlen(color), STYLE_BOLD);
	return ret;
}

char *cmd_bg(struct prompt_elems *args)
{
	char *color = NULL;
	if (args->count == 1) {
		if (args->items[0].etype != ETYPE_CHAR) {
			fprintf(stderr, "Color has to be a constant\n");
			return NULL;
		}
		int cs = parse_color(args->items[0].c.items);
		if (cs == -1) {
			fprintf(stderr, "Invalid color constant %s\n",
				args->items[0].c.items);
			return NULL;
		}
		color = strdup(color_vals_bg[cs]);
	}
	return color;
}

/* Reset color and style, args are ignored */
char *cmd_color_reset(struct prompt_elems *args __attribute__((unused)))
{
	return COLOR_RESET;
}

/* TODO: Implement this */
char *cmd_if(struct prompt_elems *args)
{
	return NULL;
}

#define CMDCOUNT 5

char *(*cmd_func[CMDCOUNT])(struct prompt_elems *) = { cmd_fg, cmd_fg_bold,
						       cmd_bg, cmd_color_reset,
						       cmd_if };
char *cmd_names[CMDCOUNT] = { "fg", "fg_bold", "bg", "color_reset", "if" };

int get_prompt_cmd(char *name)
{
	for (int i = 0; i < CMDCOUNT; i++) {
		if (strcmp(name, cmd_names[i]) == 0)
			return i;
	}
	return -1;
}

struct prompt_elems *parse_prompt(char *s, int args);

struct prompt_cmd parse_prompt_cmd(char *c)
{
	struct prompt_cmd out = { 0 };
	struct CharArray outname = { 0 };
	struct prompt_elems *arglist = malloc(sizeof(struct prompt_elems));
	(*arglist) = (struct prompt_elems){ 0 };
	struct prompt_elem *current_arg = malloc(sizeof(struct prompt_elem));
	(*current_arg) = (struct prompt_elem){ 0 };
	int parsing_args = 0;
	for (size_t i = 0; i < strlen(c); i++) {
		if (parsing_args) {
			struct CharArray args = { 0 };
			int pc = 0;
			while ((c[i] != ']' || pc > 0) && i < strlen(c)) {
				if (c[i] == '[')
					pc++;
				if (c[i] == ']')
					pc--;
				nob_da_append(&args, c[i]);
				i++;
			}
			parsing_args = 0;
			nob_da_append(&args, 0);
			arglist = parse_prompt(args.items, 1);
		} else {
			if (c[i] == '[') {
				parsing_args = 1;
			} else {
				nob_da_append(&outname, c[i]);
			}
		}
	}
	nob_da_append(&outname, 0);
	out.cmd_name = outname.items;
	out.args = arglist;
	return out;
}

/* Parse the prompt, if args is non-zero, parse function parameters instead */
struct prompt_elems *parse_prompt(char *s, int args)
{
	int parsing_cmd = 0;
	int parsing_var = 0;
	struct prompt_elems *elems = malloc(sizeof(struct prompt_elems));
	(*elems) = (struct prompt_elems){ 0 };
	struct prompt_elem *current = malloc(sizeof(struct prompt_elem));
	(*current) = (struct prompt_elem){ 0 };
	int f = 0;
	for (size_t i = 0; i < strlen(s); i++) {
		if (parsing_cmd) {
			printf("%s\n", s+i);
			if (current->etype == ETYPE_CHAR && f == 1)
				nob_da_append(&current->c, 0);
			if (!f) {
				current->etype = ETYPE_CMD;
				f = 1;
			} else {
				current->next =
					malloc(sizeof(struct prompt_elem));
				(*(current->next)) = (struct prompt_elem){ 0 };
				nob_da_append(elems, *current);
				current = current->next;
				current->etype = ETYPE_CMD;
			}
			struct CharArray cmd = { 0 };
			int pc = 1;
			while (s[i] != 0 && i < strlen(s)) {
				if (s[i] == '\%') {
					i++;
					if (s[i] == '{') {
						pc++;
					} else if (s[i] == '}') {
						pc--;
					}
				} else {
					nob_da_append(&cmd, s[i]);
				}
				if (pc <= 0)break;
				i++;
			}
			nob_da_append(&cmd, 0);
			current->cmd = parse_prompt_cmd(cmd.items);
			parsing_cmd = 0;
		} else if (parsing_var) {
			if (current->etype == ETYPE_CHAR && f == 1)
				nob_da_append(&current->c, 0);
			if (!f) {
				current->etype = ETYPE_VAR;
				f++;
			} else {
				nob_da_append(elems, *current);
				current->next =
					malloc(sizeof(struct prompt_elem));
				(*(current->next)) = (struct prompt_elem){ 0 };
				current = current->next;
				current->etype = ETYPE_VAR;
			}
			current->var = (struct prompt_var){ 0 };
			struct CharArray vn = { 0 };
			while (s[i] != '\%') {
				nob_da_append(&vn, s[i]);
				i++;
			}
			int vi = get_prompt_var(vn.items);
			if (vi != -1) {
				current->var.var_name = strdup(var_names[vi]);
			} else
				current->var.var_name = strdup(var_names[0]);
			parsing_var = 0;
		} else {
			if (s[i] == '\%') {
				i++;
				if (s[i] != '%') {
					if (s[i] == '{') {
						parsing_cmd = 1;
					} else
						parsing_var = 1;
					continue;
				}
			}
			if (!f) {
				current->etype = ETYPE_CHAR;
				f++;
			}
			if (current->etype != ETYPE_CHAR ||
			    (args && isspace(s[i]))) {
				if (current->etype == ETYPE_CHAR)
					nob_da_append(&current->c, 0);
				nob_da_append(elems, *current);
				current->next =
					malloc(sizeof(struct prompt_elem));
				(*(current->next)) = (struct prompt_elem){ 0 };
				current = current->next;
				current->etype = ETYPE_CHAR;
				current->c = (struct CharArray){ 0 };
			}
			nob_da_append(&(current->c), s[i]);
		}
	}
	nob_da_append(elems, *current);
	while (current->prev != NULL) {
		current = current->prev;
	}
	return elems;
}

char *prompt_stringify(struct prompt_elems *prompt)
{
	size_t cap = 16;
	size_t osize = 0;
	char *out = malloc(cap);
	for (size_t i = 0; i < prompt->count; i++) {
		switch (prompt->items[i].etype) {
		case ETYPE_CHAR:
			if (osize + prompt->items[i].c.count >= cap) {
				cap *= 2;
				out = realloc(out, cap);
			}
			if (prompt->items[i].c.items[0] != 0) {
				strcpy(out + osize, prompt->items[i].c.items);
				osize += prompt->items[i].c.count;
				out[osize] = 0;
			}
			break;
		case ETYPE_VAR: {
			int ix = get_prompt_var(prompt->items[i].var.var_name);
			if (ix != -1) {
				char *v = var_func[ix]();
				if (v != NULL) {
					if (osize + strlen(v) >= cap) {
						cap *= 2;
						out = realloc(out, cap);
					}
					strcpy(out + osize, v);
					osize += strlen(v);
					out[osize] = 0;
				}
			} else {
				char *v = var_func[0]();
				if (v != NULL) {
					if (osize + strlen(v) >= cap) {
						cap *= 2;
						out = realloc(out, cap);
					}
					strcpy(out + osize, v);
					osize += strlen(v);
					out[osize] = 0;
				}
			}
			break;
		}
		case ETYPE_CMD: {
			int ix = get_prompt_cmd(prompt->items[i].cmd.cmd_name);
			if (ix != -1) {
				char *v =
					cmd_func[ix](prompt->items[i].cmd.args);
				if (v != NULL) {
					if (osize + strlen(v) >= cap) {
						cap *= 2;
						out = realloc(out, cap);
					}
					strcpy(out + osize, v);
					osize += strlen(v);
					out[osize] = 0;
				}
			} else {
				char *v =
					cmd_func[0](prompt->items[i].cmd.args);
				if (v != NULL) {
					if (osize + strlen(v) >= cap) {
						cap *= 2;
						out = realloc(out, cap);
					}
					strcpy(out + osize, v);
					osize += strlen(v);
					out[osize] = 0;
				}
			}
			break;
		}
		}
	}
	return out;
}
