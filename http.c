#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>

#include "hashtable.h"
#include "http.h"

#define SERVERFILES "./serverfiles/"

#define STATUS_200 "200 OK"
#define STATUS_201 "201 Created"
#define STATUS_401 "401 Bad Request"
#define STATUS_403 "403 Forbidden"
#define STATUS_404 "404 Not Found"

//	TODO: Move the sending of responses to a central function

void *parse_request(void *p_client_socket) {
	int client_socket = *((int *)p_client_socket);
	free(p_client_socket);

	int buffer_size = 65536;
	char buffer[buffer_size];

	int bytes_received = 
	recv(client_socket, buffer, buffer_size - 1, 0);

	if (bytes_received < 0) {
		perror("No bytes were received\n");
		close(client_socket);
		return NULL;
	}
	printf("%s\n", buffer);

	char method[16];
	char path[128];

	sscanf(buffer, "%15s %127s", method, path);

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

	close(client_socket);
	return NULL;
}

char *generate_response(char *status, char *mime_type, char* content, unsigned long content_length) {
	char *header = calloc(2048, sizeof(char));
	snprintf(header, 2048,
		"HTTP/1.1 %s\r\n"
		"Connection: close\r\n"
		"Content-Length: %ld\r\n"
		"Content-Type: %s\r\n\r\n"
		"%s\r\n",
		status, content_length + 2, mime_type, content);

	return header;
}
	

int handle_get_request(int client_socket, char *path) {
	char *status = STATUS_200;
	char *content_type = "text/html";

	char *resource = get_value_from_hashtable(path);
	if (resource == NULL) {
		resource = "404.html";
		status = STATUS_404;
	}
	else { // crude mime check, TODO: make mime check functions
		char start[7];
		sscanf(resource, "%6s", start);
		start[6] = '\0';
		if (!strcmp("posts/", start))
			content_type = "text/plain";
	}

	char filepath[512];

	snprintf(filepath,sizeof(filepath),"%s%s",SERVERFILES,resource);

	//!printf("%s\n", filepath);
	//open a file to serve
	FILE *file_data;
	file_data = fopen(filepath, "r");

	char content[1024];
	char *content_pointer = content;
	unsigned long content_length = 0; 
	
	// read file into content string
	while ((*(content_pointer) = fgetc(file_data)) != EOF){
		content_pointer++;
		content_length++;
	}
	*content_pointer = '\0';

	fclose(file_data);

	char *response = generate_response(status, content_type, content, content_length);


	long response_length = strlen(response);
	
	int bytes_sent = send(client_socket, response, response_length, 0);

	free(response);

	if (bytes_sent < 0) {
		perror("No bytes were sent to the client\n");
		return 1;
	}

	return 0;
}

int handle_post_request(int client_socket, char *path, char *buffer) {
	// segfault on empty file name, kick the request out
	if (!strcmp(path, "/")) {
		perror("URL request path cannot be empty");
		char content[128] = "{\"status\": \"bad request\"}";
		unsigned long content_length = strlen(content);
		char *response = generate_response(STATUS_401, "application/json", content, content_length);
		long response_length = strlen(response);
		int bytes_sent = send(client_socket, response, response_length, 0);

		free(response);

		if (bytes_sent < 0) {
			perror("No bytes were sent to the client\n");
			return 2;
		}
		return 1;
	}

	// advance buffer pointer to the start of the request body
	char *buffer_pointer;
	buffer_pointer = strtok(buffer, "\n");
	//	strlen is more robust than strcmp here
	//	\r\n and \n are captured because one will leave strlen = 1 and the other strlen = 0
	while (strlen(buffer_pointer = strtok(NULL, "\n")) > 1);
	buffer_pointer = strtok(NULL, "\n");

	char file_name[128] = "serverfiles/posts";
	strcat(file_name, path);
	strcat(file_name, ".txt");
			
	FILE *new_file = fopen(file_name, "w");
	fwrite(buffer_pointer, sizeof(char), 1024, new_file);
	fclose(new_file);

	char content[128] = "{\"status\": \"created\"}";
	unsigned long content_length = strlen(content); // Is there a better way to do this?
	char *response = generate_response(STATUS_201, "application/json", content, content_length);

	long response_length = strlen(response);
	
	int bytes_sent = send(client_socket, response, response_length, 0);
	free(response);

	if (bytes_sent < 0) {
		perror("No bytes were sent to the client\n");
		return 2;
	}

	return 0;
}

int handle_delete_request(int client_socket, char *path) {

	char *value = get_value_from_hashtable(path);

	if (value == NULL) {
		char content[128] = "{\"status\": \"bad request\"}";
		unsigned long content_length = strlen(content);

		char *response = generate_response(STATUS_401, "application/json", content, content_length);
		long response_length = strlen(response);

		int bytes_sent = send(client_socket, response, response_length, 0);

		free(response);

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
		char content[128] = "{\"status\": \"forbidden\"}";
		unsigned int content_length = strlen(content);

		char *response = generate_response(STATUS_403, "application/json", content, content_length);

		long response_length = strlen(response); 		

		int bytes_sent = send(client_socket, response, response_length, 0);

		if (bytes_sent < 0) {
			perror("No bytes were sent to the client\n");
			return 2;
		}
		return 1;
	}

	char filepath[512];
	snprintf(filepath, sizeof(filepath), "%s%s", SERVERFILES, value);

	int successful_removal = remove(filepath);

	if (successful_removal) {
		perror("File could not be removed\n");
		return 1;
	}

	remove_from_hashtable(path);

	// Send successful delete response
	char *content = "{\"status\": \"ok\"}";
	unsigned long content_length = strlen(content);
	char *response = generate_response(STATUS_200, "application/json", content, content_length);

	long response_length = strlen(response);
	
	int bytes_sent = send(client_socket, response, response_length, 0);

	if (bytes_sent < 0) {
		perror("No bytes were sent to the client\n");
		return 2;
	}

	return 0;
}
