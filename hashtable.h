#ifndef ROUTES_H
#define ROUTES_H

#include <signal.h>

typedef struct pair {
	char *key;
	char *value;
	struct pair *next;
} Pair;


void create_hashtable();
char *get_value_from_hashtable(char *key);
//Pair *get_node_from_hashtable(char *input, int offset);
void add_to_hashtable(char *new_key, char *value);
void remove_from_hashtable(char *input);
//void delete_hashtable();

void handle_interrupt(int sig);



#endif
