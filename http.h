#ifndef HTTP_H
#define HTTP_H

int parse_request(int client_socket);

int handle_get_request(int client_socket, char *path);
int handle_post_request(int client_socket, char *path, char *buffer);
int handle_delete_request(int client_socket, char *path);

#endif
