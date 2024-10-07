#ifndef _ASH_UTILS_H
#define _ASH_UTILS_H

#define COLOR_RESET "\x1b[0m"
#define STYLE_BOLD "\x1b[1m"
#define STYLE_DIM "\x1b[2m"
#define STYLE_ITALIC "\x1b[3m"
#define STYLE_UNDERLINE "\x1b[4m"
#define STYLE_BLINK_SLOW "\x1b[5m"
#define STYLE_BLINK_FAST "\x1b[6m"
#define STYLE_REVERSE "\x1b[7m"
#define STYLE_HIDDEN "\x1b[8m"
#define STYLE_STRIKE "\x1b[9m"
#define STYLE_DEFAULT_FONT "\x1b[10m"
#define STYLE_ALT_FONT(n) "\x1b[1" #n "m" // n is 1-9
#define STYLE_FRAKTUR "\x1b[20m" // Rarely supported
#define STYLE_DOUBLE_UNDERLINE "\x1b[21m"
#define STYLE_NORMAL "\x1b[22m"
#define STYLE_NO_ITALIC "\x1b[23m"
#define STYLE_NO_UNDERLINE "\x1b[24m"
#define STYLE_NO_BLINK "\x1b[25m"
#define STYLE_PROPORTIONAL_SPACING "\x1b[26m"
#define STYLE_NO_REVERSE "\x1b[27m"
#define STYLE_NO_HIDDEN "\x1b[28m"
#define STYLE_NO_STRIKE "\x1b[29m"
#define COLOR_FG_BLACK "\x1b[30m"
#define COLOR_FG_RED "\x1b[31m"
#define COLOR_FG_GREEN "\x1b[32m"
#define COLOR_FG_YELLOW "\x1b[33m"
#define COLOR_FG_BLUE "\x1b[34m"
#define COLOR_FG_MAGENTA "\x1b[35m"
#define COLOR_FG_CYAN "\x1b[36m"
#define COLOR_FG_WHITE "\x1b[37m"
#define COLOR_FG_CUSTOM(r, g, b) "\x1b[38;2;" #r ";" #g ";" #b "m"
#define COLOR_FG_DEFAULT "\x1b[39m"
#define COLOR_BG_BLACK "\x1b[40m"
#define COLOR_BG_RED "\x1b[41m"
#define COLOR_BG_GREEN "\x1b[42m"
#define COLOR_BG_YELLOW "\x1b[43m"
#define COLOR_BG_BLUE "\x1b[44m"
#define COLOR_BG_MAGENTA "\x1b[45m"
#define COLOR_BG_CYAN "\x1b[46m"
#define COLOR_BG_WHITE "\x1b[47m"
#define COLOR_BG_CUSTOM(r, g, b) "\x1b[48;2;" #r ";" #g ";" #b "m"
#define COLOR_BG_DEFAULT "\x1b[49m"

#define eprintf(fmt, ...) fprintf(stderr, fmt __VA_OPT__(, ) __VA_ARGS__)
#define PERR(fmt, ...)                                             \
	eprintf(STYLE_BOLD COLOR_FG_RED "[ERROR] " COLOR_RESET fmt \
					"\n" __VA_OPT__(, ) __VA_ARGS__)
#define PINFO(fmt, ...)                                             \
	eprintf(STYLE_BOLD COLOR_FG_GREEN "[INFO] " COLOR_RESET fmt \
					  "\n" __VA_OPT__(, ) __VA_ARGS__)
#define PDEBUG(fmt, ...)                                            \
	eprintf(STYLE_BOLD COLOR_FG_CYAN "[DEBUG] " COLOR_RESET fmt \
					 "\n" __VA_OPT__(, ) __VA_ARGS__)
#define PWARN(fmt, ...)                                              \
	eprintf(STYLE_BOLD COLOR_FG_YELLOW "[WARN] " COLOR_RESET fmt \
					   "\n" __VA_OPT__(, ) __VA_ARGS__)

#define STARTS_WITH(s, prefix) (strstr(s, prefix) == s)
#define ENDS_WITH(s, suffix) \
	(strcmp(s + strlen(s) - strlen(suffix), suffix) == 0)

#endif
