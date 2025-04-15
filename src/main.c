#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

int handleRequest(int);

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

	char* buff;
	int bytes_sent;

	if (handleRequest(new_fd) != 0)
		buff = "HTTP/1.1 404 Not Found\r\n\r\n";
	else
		buff = "HTTP/1.1 200 OK\r\n\r\n";
		

	bytes_sent = send(new_fd, buff, strlen(buff), 0);
		
	if (bytes_sent < 0)
	{
		printf("Failed sendind message: %s", strerror(errno));
		return 1;
	}
	
	/*
	char *buff = "HTTP/1.1 200 OK\r\n\r\n";
	int bytes_sent = send(new_fd, buff, strlen(buff), 0);

	if (bytes_sent < 0)
	{
		printf("Failed sendind message: %s", strerror(errno));
		return 1;
	}
	*/

	printf("Message sent!\n");

	close(server_fd);

	return 0;
}

int handleRequest(int new_fd)
{
	char buff[256], target[128];
	char* token;
	size_t i = 0;
	ssize_t bytes_recvd = recv(new_fd, buff, 256, 0);
	
	if (bytes_recvd == -1)
	{
		printf("Error reading message: %s", strerror(errno));
		return -1;
	}

	token = strchr(buff, '/');

	if (token == NULL)
	{
		printf("Invalid request, no URL");
		return -1;
	}

	while (*token != ' ')
	{
		target[i] = *token;
		token++;
		i++;
	}

	target[i] = '\0';
	printf("Message: %s :: length : %d\n\n", target, strlen(target));

	if (strcmp(target, "/") != 0)
	{
		printf("Invalid URL: %s", target);
		return -1;
	}

	return 0;
}
