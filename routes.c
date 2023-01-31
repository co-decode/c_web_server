#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>

#include "hashtable.h"

#define POSTS "posts/"

void initialise_existing_routes() {
	add_to_hashtable("/index", "index.html");
	add_to_hashtable("/", "index.html");
	add_to_hashtable("/about", "about.html");

	// Crawl through posts directory...
	
	DIR *d;
	struct dirent *dir;
	d = opendir("./serverfiles/posts");
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			// Reject hidden dot files
			if (dir->d_name[0] == '.') continue;
			char key[64];
			key[0] = '/';
			int i = 1;

			// This currently does not support the appearance of dots within the post name
			// It also assumes that a file extension is present

			while (dir->d_name[i-1] != '.') {
				key[i] = dir->d_name[i-1];
				i++;
			}
			key[i] = '\0';

			char value[128];
			snprintf(value, sizeof(value), "%s%s", POSTS, dir->d_name);
			add_to_hashtable(key, value);
		}
		closedir(d);
	}
}

