#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "requests_structs.h"

char* extract_path(int);
char* parse_path(char*, char*);
enum http_request get_method_from_str(char* str);

int main() {
	// Disable output buffering
	setbuf(stdout, NULL);
 	setbuf(stderr, NULL);

	// client structure, socket file descriptor
	int server_fd, client_addr_len;
	struct sockaddr_in client_addr;
	
	// Create new TCP socket
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		printf("Socket creation failed: %s...\n", strerror(errno));
		return 1;
	}
	
	// Since the tester restarts your program quite often, setting SO_REUSEADDR
	// ensures that we don't run into 'Address already in use' errors
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		printf("SO_REUSEADDR failed: %s \n", strerror(errno));
		return 1;
	}
	
	// Set server socket to be IPv4 address on port 4221 listening to all messages
	struct sockaddr_in serv_addr = { .sin_family = AF_INET ,
									 .sin_port = htons(4221),
									 .sin_addr = { htonl(INADDR_ANY) },
									};
	

	if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
		printf("Bind failed: %s \n", strerror(errno));
		return 1;
	}
	
	int connection_backlog = 5;
	if (listen(server_fd, connection_backlog) != 0) {
		printf("Listen failed: %s \n", strerror(errno));
		return 1;
	}
	
	printf("Waiting for a client to connect...\n");
	client_addr_len = sizeof(client_addr);
	
	int new_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
	printf("Client connected\n");

	if (new_fd == -1)
	{
		printf("Error handling new connection: %s \n", strerror(errno));
		return 1;
	}

	char* url_path = extract_path(new_fd);

	if (strcmp(url_path, "-1") != 0)
	{
		char response_body[256];
		parse_path(url_path, response_body);
		
		int body_len = strlen(response_body);

		char response_ok[1024];

		snprintf(response_ok, sizeof(response_ok), "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s", 
				body_len, response_body);

		int bytes_sent;
		bytes_sent = send(new_fd, response_ok, strlen(response_ok), 0);
			
		if (bytes_sent < 0)
		{
			printf("Failed sending message: %s", strerror(errno));
			return 1;
		}

		printf("\nMessage sent: %s\n", response_ok);
	}

	close(new_fd);
	close(server_fd);

	return 0;
}

char* extract_path(int new_fd)
{
	char buff[256];
	ssize_t bytes_recvd = recv(new_fd, buff, 256, 0);	// Read from the socket and save it to buffer
	
	if (bytes_recvd == -1)
	{
		printf("Error reading message: %s", strerror(errno));
		return "-1";
	}

	struct http_header request;
	char *token;

	token = strtok(buff, " ");
	printf("\nMETHOD: %s", token);
	request.method = get_method_from_str(token);

	token = strtok(0, " ");
	request.url_path = token;
	printf("\nPATH: %s\n", request.url_path);

	return request.url_path;
}

char* parse_path(char* path, char* output)
{
	char _path[256];
	char* token;
	_path[sizeof(_path) - 1] = '\0';

	strncpy(_path, path, sizeof(_path)); // Copy path to _path for strtok()

	token = strtok(_path, "/");
	printf("\nTOKEN: %s\n", token);

	if (strncmp(token, "echo", 5) == 0)
	{
		char* str = strtok(NULL, " ");
		printf("\nSTRING: %s\n", str);
		strcpy(output, str);
		return output;
	}

	strncpy(output, token, sizeof(output));
	return output;
}

enum http_request get_method_from_str(char* str)
{
	size_t methods_size = sizeof(methods) / sizeof(methods[0]);

	for (size_t i = 0; i < methods_size; ++i)
	{
		if (strcmp(str, methods[i].string) == 0)
			return methods[i].method;
	}

	return -1;
}
