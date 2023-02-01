#ifndef HTTP_H
#define HTTP_H

void *parse_request(void *p_client_socket);

int handle_get_request(int client_socket, char *path);
int handle_post_request(int client_socket, char *path, char *buffer);
int handle_delete_request(int client_socket, char *path);

#endif
