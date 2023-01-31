#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "hashtable.h"
#define HASHTABLE_SIZE 512

Pair *hashtable = NULL;

void create_hashtable() {
	hashtable = 
		calloc(HASHTABLE_SIZE, sizeof(Pair));
	return;
}

int hash(char *input) {
	// folding 2 bytes at a time
	unsigned long sum = 0, mul = 1;
	for (int i = 0; *(input + i) != '\0'; i++) {
		mul = (i % 2 == 0) ? 1 : mul * 997;
		sum += *(input + i) * mul;
	}
	return sum % HASHTABLE_SIZE;
}

char *get_value_from_hashtable(char *key) {
	int offset = hash(key);
	Pair current_pair = *(hashtable + offset);
	if (current_pair.key == NULL) return NULL;

	while (strcmp(current_pair.key, key))
		if (current_pair.next == NULL) return NULL;
		else current_pair = *(current_pair.next);
	return current_pair.value;
}

Pair *get_node_from_hashtable(char *input, int offset) {
	Pair current_pair = *(hashtable + offset);
	if (current_pair.key == NULL) return NULL;

	Pair prev_pair = current_pair;
	while (strcmp(current_pair.key,input)) {
		if (current_pair.next == NULL) break;
		else {
			prev_pair = current_pair;
			current_pair = *current_pair.next;
		}
	}
	return prev_pair.next == NULL ? (hashtable + offset) : prev_pair.next;
}


void add_to_hashtable(char *new_key, char *value) {
	int offset = hash(new_key);

	Pair new_pair = {
		.key = calloc(64, sizeof(char)),
		.value = calloc(128, sizeof(char)),
		.next = NULL
	};
	strcpy(new_pair.key, new_key);
	strcpy(new_pair.value, value);
	
	Pair *key_check = get_node_from_hashtable(new_pair.key, offset);
	
	if (key_check == NULL)
		*(hashtable + offset) = new_pair;
	else if (strcmp(key_check->key, new_pair.key)) { 
	// if the key check returns a pair that doesn't have the same key
		key_check->next = calloc(1, sizeof(Pair));
		key_check->next->key = new_pair.key;
		key_check->next->value = new_pair.value;
		key_check->next->next = new_pair.next;
	}
	else // TODO: Handle this error, inform the client
		printf("That key already exists\n");
	return;
}

void remove_from_hashtable(char *input) {
	int offset = hash(input); // can the hashing be moved to some central point?
	
	Pair current_pair = *(hashtable + offset);
	if (current_pair.key == NULL) {
		// TODO: Handle this error, inform the client
		printf("%s is not in the hashtable\n",input);
		return;
	}

	Pair prev_pair = current_pair;
	while (strcmp(current_pair.key,input)) {
		if (current_pair.next == NULL) {
			printf("%s is not in the hashtable\n", input);
			return;
		}
		else {
			prev_pair = current_pair;
			current_pair = *current_pair.next;
		}
	}
	free(current_pair.key);
	free(current_pair.value);
	if (prev_pair.next == NULL) { // No existing linkages
		memset((hashtable + offset), 0, sizeof(Pair));
	}
	else { // Part of a linked list
		Pair *tmp = current_pair.next; 
		free(prev_pair.next);
		prev_pair.next = tmp;
	}

	return;
}	

void free_pair(Pair *node) {
	free(node->key);
	free(node->value);
	return;
}

void traverse_and_free(Pair *node) {
	free_pair(node);
	if (node->next == NULL)
		return;
	traverse_and_free(node->next);
	free(node->next);
	return;
}

void delete_hashtable() {
	for (int i = 0; i < HASHTABLE_SIZE; i++) {
		Pair *pointer = (hashtable + i);
		if (pointer->key == NULL) continue;
		else if (pointer->next == NULL) 
			free_pair(pointer);
		else traverse_and_free(pointer);
	}
	free(hashtable);
	hashtable = NULL;

	return;
}


void handle_interrupt(int sig) {
	signal(sig, SIG_IGN); // Is this good practice?
	delete_hashtable();
	printf("\nClean up complete\n");
	exit(0);
}
/*
int main() {
	create_hashtable();
	signal(SIGINT, handle_interrupt);
	while (1) {
		char input[64];
		char ch;
		int i = 0;
		printf("Enter the new key\n");
		while ((ch = getchar()) != '\n' && i < 63)
			input[i++] = ch;
		input[i] = '\0';
		
		char value[128];
		i = 0;
		printf("Enter the value of the key\n");
		while ((ch = getchar()) != '\n' && i < 127)
			value[i++] = ch;
		value[i] = '\0';

		add_to_hashtable(input, value);
	}
	return 0;
}
*/
