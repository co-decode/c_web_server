#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8001

char *match_path(char *path) {
	if (!strcmp(path, "/"))
		return "serverfiles/index.html";
	else if (!strcmp(path, "/about"))
		return "serverfiles/about.html";
	else 
		return "serverfiles/404.html";
}

int handle_get_request(int client_socket, char *path) {
	char *resource = match_path(path);

	printf("%s\n", resource);
	printf("I'm a happy tea pot");
	printf("Come on now");
	//open a file to serve
	FILE *html_data;
	html_data = fopen(resource, "r");

	char response_data[1024];
	char *rd_pointer = response_data;
	long content_length = 0; 
	// read file into response_data string
	while ((*(rd_pointer) = fgetc(html_data)) != EOF){
		rd_pointer++;
		content_length++;
	}
	*rd_pointer = '\0';
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
		"Content-Length: 18\r\n"
		"Content-Type: application/json\r\n\r\n"
		"{\"status\": \"ok\"}\r\n");

	long header_length = strlen(http_header); // always 113
	
	int bytes_sent = send(client_socket, http_header, header_length, 0);

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

int main() {
	int server_socket = initialise_server_socket();

	int client_socket;

	printf("Web server awaiting connections on port: %d\n", PORT);

	while(1) {
		client_socket = accept(server_socket, NULL, NULL);

		parse_request(client_socket);

		close(client_socket);
	}

	return 0;
}
