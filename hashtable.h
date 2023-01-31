#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <signal.h>

typedef struct pair {
	char *key;
	char *value;
	struct pair *next;
} Pair;

void create_hashtable();

char *get_value_from_hashtable(char *key);

void add_to_hashtable(char *new_key, char *value);
void remove_from_hashtable(char *input);

void handle_interrupt(int sig);

#endif
