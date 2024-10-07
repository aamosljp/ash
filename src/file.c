#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <env.h>

typedef enum {
	FILE_DIRECTORY,
	FILE_REGULAR,
	FILE_SYMLINK,
	FILE_OTHER,
} FileType;

int get_file_type(char *path)
{
	struct stat statbuf;
	if (stat(path, &statbuf) < 0) {
		return -1;
	}

	switch (statbuf.st_mode & S_IFMT) {
	case S_IFDIR:
		return FILE_DIRECTORY;
	case S_IFREG:
		return FILE_REGULAR;
	case S_IFLNK:
		return FILE_SYMLINK;
	default:
		return FILE_OTHER;
	}
}

char *get_dir(char *npath, char *path, int tilde)
{
	if (get_file_type(path) == FILE_DIRECTORY) {
		npath = strdup(path);
	} else {
		npath = malloc(sizeof(char)*strlen(path));
		char* xpath = strdup(path);
		int npsize = 0;
		if (strchr(xpath, '/') != NULL) {
			char *c = strtok(xpath, "/");
			char *oc = NULL;
			char *ooc = NULL;
			while (c != NULL) {
				if (oc)
					ooc = oc;
				if (ooc) {
					*(npath + (npsize++)) = '/';
					strcpy(npath + npsize, ooc);
					npsize += strlen(ooc);
				}
				oc = strdup(c);
				c = strtok(NULL, "/");
			}
			if (get_file_type(npath) != FILE_DIRECTORY) {
				fprintf(stderr,
					"Could not find directory of %s: %s",
					path, strerror(errno));
				return NULL;
			}
		} else {
			fprintf(stderr, "Could not find directory of %s: %s",
				path, strerror(errno));
			return NULL;
		}
	}
	if (tilde && strstr(npath, env_home) == npath) {
		char *nnpath = NULL;
		if (strlen(npath) > strlen(env_home)) {
			nnpath = malloc(strlen(npath) - strlen(env_home) + 2);
			(*nnpath) = '~';
			strcpy(nnpath + 1, npath + strlen(env_home));
		} else {
			nnpath = "~";
		}
		npath = nnpath;
	}
	return npath;
}
