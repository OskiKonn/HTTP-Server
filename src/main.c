#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "requests_structs.h"
#include "defines.h"

char* extract_path(char*, int);
int read_request(int, char*, size_t);
int parse_path(char*, char*);
int respond_with_body(int, char*);
int respond_not_found(int);
void on_connect(struct sockaddr_in* client);
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
	

	/* Bind socket to the address */								
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
	on_connect(&client_addr); 	/* Call on_connect function after establishing connection with the client */

	if (new_fd == -1)
	{
		printf("Error handling new connection: %s \n", strerror(errno));
		return 1;
	}

	char data[256];
	int data_ok = read_request(new_fd, data, sizeof(data));
	char* url_path = extract_path(data, new_fd);

	if (strcmp(url_path, "-1") != 0)
	{
		char response_body[256];

		if (parse_path(url_path, response_body) == 0)	/* If clients calls /echo respond with a body (/echo/{body}) */
			respond_with_body(new_fd, response_body);
		else
			respond_not_found(new_fd);

	}

	close(new_fd);
	close(server_fd);

	return 0;
}

	/* This function takes the HTTP request and extracts the URL path provided
		Returns '-1' if there is no path ("/") or if it failed to read the request
		If it finds URL valid returns the path */

char* extract_path(char *from_buffer, int connection)
{
	char buff[256];
	strncpy(buff, from_buffer, sizeof(buff));
	
	struct http_header request;
	char *token;

	token = strtok(buff, " ");		/* Take out the HTTP metod and set request.method */
	printf("\nMETHOD: %s", token);
	request.method = get_method_from_str(token);

	token = strtok(0, " ");
	request.url_path = token;		/* Take the whole URL path after HTTP method */
	printf("\nPATH: %s\n", request.url_path);

	if (strcmp(request.url_path, "/") == 0)
	{
		char response_ok[] = "HTTP/1.1 200 OK\r\n\r\n";
		send(connection, response_ok, strlen(response_ok), 0);
		return "-1";
	}

	return request.url_path;
}

	/* 	This function takes the  extraced URL path and pointer to the buffer where it would copy the body
		in case the client called "/echo/{str}"
		
		Returns 0 on /echo and -1 if path is different */

int parse_path(char* path, char* output)
{
	char _path[256];
	char* token;
	_path[sizeof(_path) - 1] = '\0';

	strncpy(_path, path, sizeof(_path)); // Copy path to _path for strtok()

	token = strtok(_path, "/");

	if (strncmp(token, "echo", 5) == 0)
	{
		char* str = strtok(NULL, " ");
		strcpy(output, str);	/* If client calss /echo copy the body value and set output to it */
		return RESPOND_ECHO;
	}

	if (strncmp(token, "user-agent", 10) == 0)
		return READ_USER_AGENT;

	return -1;
}

	/* 	This function takes the method from HTTP request in form of string and
		iterates over available methods structure and returns it if finds one */

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

	/* 	If the client called "/echo{str}" as request this function prepares the response with the 
		provided body and sends it back to the client
		
		Returns number of bytes successfully sent */

int respond_with_body(int connection, char* body)
{
	int body_len = strlen(body);

	char response_ok[1024];

	snprintf(response_ok, sizeof(response_ok), "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s", 
			body_len, body);

	int bytes_sent;
	bytes_sent = send(connection, response_ok, strlen(response_ok), 0);
		
	if (bytes_sent < 0)
	{
		printf("Failed sending message: %s", strerror(errno));
		return 1;
	}

	printf("\nMessage sent: %s\n", response_ok);
	return bytes_sent;
}

int respond_not_found(int connection)
{
	char response_not_found[] = "HTTP/1.1 404 Not Found\r\n\r\n";
	int bytes_sent = send(connection, response_not_found, strlen(response_not_found), 0);

	return bytes_sent;
}

void on_connect(struct sockaddr_in *client)
{
	char *client_ip = inet_ntoa(client->sin_addr);
	in_port_t client_port = ntohs(client->sin_port);

	printf("\nCONNECTED CLIENT INFO: \n\n\tIP: %s\n\tPORT: %d\n", client_ip, client_port);
}

char *read_header(char* data, int header)
{
	char _data[256];
	char *p_data;
	strncpy(_data, data, sizeof(_data));
	p_data = strtok(_data, "\n");
	//TODO: header parsing
	
}

int read_request(int connection, char* dest_buffer, size_t max_bytes)
{
	char request_data[256];
	ssize_t bytes_recvd = recv(connection, request_data, 256, 0);	// Read from the socket and save it to buffer

	if (bytes_recvd < 0)
	{
		printf("\nFailed reading request data: %s\n", strerror(errno));
		return -1;
	}

	strncpy(dest_buffer, request_data, max_bytes);
	return 0;
}
