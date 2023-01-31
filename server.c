#include <stdio.h>
#include <unistd.h>
#include <signal.h>

//#include <sys/socket.h>
//#include <sys/types.h>
//#include <netinet/in.h>
#include <arpa/inet.h>

#include "http.h"
#include "hashtable.h"
#include "routes.h"

#define PORT 8001

// TODO:
// 	- Extract http_header composition to its own function
// 	- Integrate multiple threads.

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
	// initialise sockets
	int server_socket = initialise_server_socket();
	int client_socket;

	printf("Web server awaiting connections on port: %d\n", PORT);

	// initialise routes
	create_hashtable();
	signal(SIGINT, handle_interrupt); // handle_interrupt defined in hashtable.c
	initialise_existing_routes();

	while(1) {
		client_socket = accept(server_socket, NULL, NULL);

		parse_request(client_socket);

		close(client_socket);
	}

	return 0;
}
