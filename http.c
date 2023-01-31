#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/socket.h>

#include "hashtable.h"
#include "http.h"

#define SERVERFILES "./serverfiles/"

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

int handle_get_request(int client_socket, char *path) {
	char *resource = get_value_from_hashtable(path);
	if (resource == NULL) resource = "404.html";

	char filepath[4096];

	snprintf(filepath,
		sizeof(filepath),
		"%s%s",
		SERVERFILES,
		resource);

	printf("%s\n", filepath);
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



