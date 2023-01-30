#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HASHTABLE_SIZE 512

/*
 * Xcreate_hashtable
 * Xadd_to_hashtable
 * Xget_from_hashtable
 * Xdelete_from_hashtable
 * delete_hashtable
 */

// TODO - Must free - TODO
// Xboth strings in each pair
// Xeach linked pair
// each pair allocated in the creation of the hashtable

struct pair {
	char *key;
	char *value;
	struct pair *next;
};

struct pair *create_hashtable() {
	struct pair *hashtable = 
		calloc(HASHTABLE_SIZE, sizeof(struct pair));
	return hashtable;
}

int hash(char *input) {
	// folding 2 bytes at a time
	unsigned long sum = 0, mul = 1;
	for (int i = 0; *(input + i) != '\0'; i++) {
		mul = (i % 2 == 0) ? 1 : mul * 1024;
		sum += *(input + i) * mul;
	}
	return sum % HASHTABLE_SIZE;
}

struct pair *get_from_hashtable(struct pair *hashtable, char *input, int offset) {
	struct pair current_pair = *(hashtable + offset);
	if (current_pair.key == NULL) return NULL;

	struct pair prev_pair = current_pair;
	while (strcmp(current_pair.key,input)) {
		if (current_pair.next == NULL) {
			printf("%s is not in the hashtable\n", input);
			break;
		}
		else {
			prev_pair = current_pair;
			current_pair = *current_pair.next;
		}
 }
	return prev_pair.next == NULL ? (hashtable + offset) : prev_pair.next;
}


void add_to_hashtable(struct pair *hashtable, char *new_key, char *value) {
	// !! THIS function may be overwriting linkages:
	// finding a node that is in the middle of a linked list and writing to the next node.
	//
	// Or maybe it always goes to the last node and finding matches causes no action to be taken...
	printf("sizeof pair: %ld\n", sizeof(struct pair));
	int offset = hash(new_key);

	struct pair new_pair = {
		.key = calloc(64, sizeof(char)),
		.value = calloc(128, sizeof(char)),
		.next = NULL
	};
	strcpy(new_pair.key, new_key);
	strcpy(new_pair.value, value);
	
	struct pair *key_check = get_from_hashtable(hashtable, new_pair.key, offset);
	
	if (key_check == NULL)
		*(hashtable + offset) = new_pair;
	else if (strcmp(key_check->key, new_pair.key)) { 
	// if the key check returns a pair that doesn't have the same key
		key_check->next = calloc(1, sizeof(struct pair));
		key_check->next = &new_pair; 
	}
	else 
		printf("That key already exists\n");
	return;
}

void remove_from_hashtable(struct pair *hashtable, char *input) {
	int offset = hash(input); // can the hashing be moved to some central point?
	
	struct pair current_pair = *(hashtable + offset);
	if (current_pair.key == NULL) {
		printf("%s is not in the hashtable\n",input);
		return;
	}

	struct pair prev_pair = current_pair;
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
		memset((hashtable + offset), 0, sizeof(struct pair));
		printf("Test after remove: %s\n", (hashtable + offset)->key);
	}
	else { // Part of a linked list
		struct pair *tmp = current_pair.next; 
		free(prev_pair.next);
		prev_pair.next = tmp;
		printf("Test after remove from ll: %p\n", prev_pair.next);
	}

	return;
}	






int main() {
	struct pair *hashtable = create_hashtable();
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

		add_to_hashtable(hashtable, input, value);
		remove_from_hashtable(hashtable, input);
	}
	return 0;
}
