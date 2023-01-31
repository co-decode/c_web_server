#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "hashtable.h"
#define PORT 8001
#define SERVERFILES "./serverfiles/"
#define POSTS "posts/"

// TODO:
// 	- Extract http_header composition to its own function
// 	- Extract http handling from server handling, place http stuff in a separate file
// 	- Extract route initialisation from server handling, place route initialisation into a separate file
// 		It would be nice to only hardcode the default routes and have a function set up all the other routes based on file name.
// 	Integrate multiple threads.

int handle_get_request(int client_socket, char *path) {
	char *resource = get_value_from_hashtable(path);
	if (resource == NULL) resource = "404.html";

	char filepath[4096];

	snprintf(filepath,
		sizeof(filepath),
		"%s%s",
		SERVERFILES,
		resource);

	//open a file to serve
	FILE *html_data;
	html_data = fopen(filepath, "r");

	char response_data[1024];
	char *rd_pointer = response_data;
	long content_length = 0; 
	// read file into response_data string
	while ((*(rd_pointer) = fgetc(html_data)) != EOF){
		rd_pointer++;
		content_length++;
	}
	*rd_pointer = '\0';

	fclose(html_data);
	printf("%d %ld\n", *(rd_pointer - 1), content_length);


	char http_header[2048];
	sprintf(http_header, "HTTP/1.1 200 OK\r\n"
		"Connection: close\r\n"
		"Content-Length: %ld\r\n"
		"Content-Type: text/html\r\n\r\n", content_length);
	strcat(http_header, response_data);

	long header_length = strlen(http_header);
	
	int bytes_sent = send(client_socket, http_header, header_length, 0);

	if (bytes_sent < 0) {
		perror("No bytes were sent to the client\n");
		return 1;
	}

	return 0;
}

int handle_post_request(int client_socket, char *path, char *buffer) {
	
	char http_header[2048];
	// segfault on empty file name, kick the request out
	if (!strcmp(path, "/")) {
		perror("URL request path cannot be empty");
		sprintf(http_header,
				"HTTP/1.1 400 BAD REQUEST\r\n"
				"Connection: close\r\n"
				"Content-Length: 27\r\n"
				"Content-Type: application/json\r\n\r\n"
				"{\"status\": \"bad request\"}\r\n");
		long header_length = strlen(http_header); // always 122
		int bytes_sent = send(client_socket, http_header, header_length, 0);

		if (bytes_sent < 0) {
			perror("No bytes were sent to the client\n");
			return 2;
		}
		return 1;
	}

	// advance buffer pointer to the start of the body
	char *buffer_pointer;
	buffer_pointer = strtok(buffer, "\n");
	while (strcmp(buffer_pointer = (strtok(NULL, "\n")), "\r"));
	buffer_pointer += 2;

	//It doesn't seem like a line feed can appear in the body,
	//Using strlen is probably cleaner than strcmp
	//Using strtok once more instead of precariously moving the pointer is probably cleaner
	
	char file_name[128] = "serverfiles/posts";
	strcat(file_name, path);
			
	FILE *new_file = fopen(file_name, "w");
	fwrite(buffer_pointer, sizeof(char), 1024, new_file);
	fclose(new_file);

	sprintf(http_header, 
		"HTTP/1.1 201 CREATED\r\n"
		"Connection: close\r\n"
		"Content-Length: 23\r\n"
		"Content-Type: application/json\r\n\r\n"
		"{\"status\": \"created\"}\r\n");

	long header_length = strlen(http_header); // always 118
	
	int bytes_sent = send(client_socket, http_header, header_length, 0);

	if (bytes_sent < 0) {
		perror("No bytes were sent to the client\n");
		return 2;
	}

	return 0;
}

int handle_delete_request(int client_socket, char *path) {
	char response[1024];

	char *value = get_value_from_hashtable(path);
	if (value == NULL) {
		sprintf(response, 
			"HTTP/1.1 401 Bad request\r\n"
			"Connection: close\r\n"
			"Content-Length: 27\r\n"
			"Content-Type: application/json\r\n\r\n"
			"{\"status\": \"bad request\"}\r\n");

		long header_length = strlen(response); 		

		int bytes_sent = send(client_socket, response, header_length, 0);

		if (bytes_sent < 0) {
			perror("No bytes were sent to the client\n");
			return 2;
		}
		return 1;
	}
	char start[128];
	strcpy(start, value);
	*(start + 6) = '\0';


	if (strcmp(start, "posts/")) {
		printf("Request rejected - may not delete non-post files\n");
		sprintf(response, 
			"HTTP/1.1 403 Forbidden\r\n"
			"Connection: close\r\n"
			"Content-Length: 25\r\n"
			"Content-Type: application/json\r\n\r\n"
			"{\"status\": \"forbidden\"}\r\n");

		long header_length = strlen(response); 		

		int bytes_sent = send(client_socket, response, header_length, 0);

		if (bytes_sent < 0) {
			perror("No bytes were sent to the client\n");
			return 2;
		}
		return 1;
	}

	char filepath[4096];
	snprintf(filepath, sizeof(filepath), "%s%s", SERVERFILES, value);

	int successful_removal = remove(filepath);

	if (successful_removal) {
		perror("File could not be removed\n");
		return 1;
	}

	remove_from_hashtable(path);

	// Send successful delete response
	
	sprintf(response, 
		"HTTP/1.1 200 OK\r\n"
		"Connection: close\r\n"
		"Content-Length: 18\r\n"
		"Content-Type: application/json\r\n\r\n"
		"{\"status\": \"ok\"}\r\n");

	long header_length = strlen(response); // always 108
	
	int bytes_sent = send(client_socket, response, header_length, 0);

	if (bytes_sent < 0) {
		perror("No bytes were sent to the client\n");
		return 2;
	}

	return 0;
}

int parse_request(int client_socket) {
	int buffer_size = 65536;
	char buffer[buffer_size];

	int bytes_received = 
	recv(client_socket, buffer, buffer_size - 1, 0);

	if (bytes_received < 0) {
		perror("No bytes were received\n");
		return 1;
	}
	printf("%s\n", buffer);

	char method[16];
	char path[128];

	sscanf(buffer, "%s %s", method, path);

	printf("method: %s, path: %s\n", method, path);

	if (!strcmp(method, "GET")) {
		handle_get_request(client_socket, path);
	}
	else if (!strcmp(method, "POST")) {
		handle_post_request(client_socket, path, buffer);
	}
	else if (!strcmp(method, "DELETE")) {
		handle_delete_request(client_socket, path);
	}
	else
		perror("Unrecognised METHOD\n");
	return 0;
}

int initialise_server_socket() {
	//create a socket
	int server_socket;
	server_socket = socket(AF_INET, SOCK_STREAM, 0);

	//define the address
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT);
	server_address.sin_addr.s_addr = INADDR_ANY;

	// attempt to speed up the re use of the socket:
	int option = 1;
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

	int bindStatus =
	bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address));
	
	if (bindStatus) perror("Bind failed\n");

	listen(server_socket, 5);

	return server_socket;
}

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

int main() {
	int server_socket = initialise_server_socket();

	int client_socket;

	printf("Web server awaiting connections on port: %d\n", PORT);

	signal(SIGINT, handle_interrupt); // defined in hashtable.c

	create_hashtable();

	initialise_existing_routes();

	while(1) {
		client_socket = accept(server_socket, NULL, NULL);

		parse_request(client_socket);

		close(client_socket);
	}

	return 0;
}
