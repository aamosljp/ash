#ifndef _ASH_PTYPES_H
#define _ASH_PTYPES_H

typedef struct word_desc {
	char *word;
	int flags;
} WORD_DESC;

typedef struct word_list {
	struct word_list *next;
	WORD_DESC *word;
} WORD_LIST;

#endif
