#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <time.h>
#include <sys/wait.h>
#include <pthread.h>
#include "server.h"

int main(int argc, char *argv[]) {
	char *port;

	if(argc != 2) {
		fprintf(stdout, "%s\n", "usage: ./server port_number");
		return 1;
	}

	port = argv[1];
	startServer(port);
}

/* 
* Start server wait client to connect
* @param port The port number that server will listen on
*/
int startServer(char *port) {
	struct addrinfo *res, *p;
	struct sockaddr_storage client_addr;
	socklen_t addr_size;
	int sockfd; 
	int status;

	struct clientinfo *info;

	// For multithreading
	pthread_t thread;

	getAddrinfoOfLocalHost(port, &res);

	// Loop for all results and bind to the first available
	for(p = res; p != NULL; p = p->ai_next) {

		// make a socket
		if((sockfd = socket(PF_INET, p->ai_socktype, p->ai_protocol)) == -1) {
			fprintf(stderr, "socket: %d\n", errno);
			continue;
		}

		// bind socket to the port
		if(bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
			fprintf(stderr, "bind: %d\n", errno);
			continue;
		}

		break;
	}

	freeaddrinfo(res);

	// listen for incoming connection
	if(listen(sockfd, MAX_CONNECTION) == -1) {
		fprintf(stderr, "listen: %d\n", errno);
		return 3;
	}

	puts("Server started.");
	puts("Server: waiting for connection...");

	// accept incoming connection
	while(1) {  // main accept() loop

		info = (struct clientinfo *)malloc(sizeof(struct clientinfo));
        addr_size = sizeof client_addr;
        info->addr_len = addr_size;
		info->sock = accept(sockfd, &(info->address), &(info->addr_len));
		if(info->sock == -1) {
			free(info);
			fprintf(stderr, "accept: %d\n", errno);
			continue;
		}

		// Create client thread
		pthread_create(&thread, 0, clientProcess, (void *)info);

		// Detach client thread
		pthread_detach(thread);
    }

    close(sockfd);
	return 0;
}

/* 
* Get addrinfo struct of session running on local machine
* @param port The port number of the session
* @param res Get filled in by pointer of addrinfo
*/
int getAddrinfoOfLocalHost(char *port, struct addrinfo **res) {
	struct addrinfo hints;
	int status;
	char ipstr[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if((status = getaddrinfo(NULL, port, &hints, res)) != 0) {
		fprintf(stderr, "gai_strerror: %s\n", gai_strerror(status));
		return 1;
	}
}

/* 
* Receive request message from client
* @param client_sock_fd The socket descriptor of client
* return error code
*/
int recv_request_msg(int client_sock_fd, char request_msg[]) {
	int recv_size;
	char buffer[MAX_BUFFER_SIZE];

	// receive request message from client

	memset(&buffer, '\0', MAX_BUFFER_SIZE);
	memset(request_msg, '\0', MAX_LEN_GET_REQUEST + 1);

	do {
		if((recv_size = recv(client_sock_fd, buffer, MAX_BUFFER_SIZE, 0)) == -1) {
			fprintf(stderr, "recv: %d\n", errno);
			return 5;
		}
		strcat(request_msg, buffer);

		if(request_completed(request_msg)) {
			break;
		}
	} while (1);
}

/*
* Process request message, then send respond message to client
* request_msg The request message received from client
* client_sock_fd The socket descriptor for accepted client 
*/
int respondToHttpRequest(char request_msg[], int client_sock_fd) {
	char *path;
	char *complete_path;
	char *buffer;
	int status;
	int num_byte_sent;
	int response_msg_len;

	char *response_msg;

	// Extract file path from request message
	getPathFromRequestMsg(request_msg, &path);

	getCompletePath(path, &complete_path, 200);
	// Read file into buffer with path
	status = readFileFromPath(complete_path, &buffer);
	switch(status) {
		case 0:
			// 200 OK
			generateResponseMsg(buffer, &response_msg, 200);
			break;
	 	case 1:
	 		// 404 Not Found
	 		getCompletePath(path, &complete_path, 404);
	 		readFileFromPath(complete_path, &buffer);
	 		generateResponseMsg(buffer, &response_msg, 404);
			break;
		default:
			// 404 Not Found
			generateResponseMsg(buffer, &response_msg, 404);
			break;
	}

	free(buffer);
	response_msg_len = strlen(response_msg);

	// Send response message back to client
	do {
		if((num_byte_sent = send(client_sock_fd, response_msg, response_msg_len, 0)) == -1) {
			puts("Fail to send out data");
			return 1;
		}

		response_msg_len -= num_byte_sent;
	} while(num_byte_sent > 0);

	free(response_msg);
	free(complete_path);
	free(path);
}

/*
* Retrive path from client request message
* @param request_msg The request message from client
* @param path Get filled in by path of requested file
*/
int getPathFromRequestMsg(char request_msg[], char **path) {
	char *header;
	int header_str_len;
	char *method;
	char *header_divider_ptr;
	char *header_divider = "\r\n";

	// // Find first \r\n string in request message
	header_divider_ptr = strstr(request_msg, header_divider);

	if(header_divider_ptr == NULL) {
		puts("Cannot resolve http header!");
		return 1;
	}

	// Find the length of request header
	header_str_len = header_divider_ptr - request_msg;

	// // Get request header string
	header = malloc(header_str_len + 1);
	memset(header, '\0', header_str_len + 1);

	strncpy(header, request_msg, header_str_len);
	*(header + header_str_len) = '\0';

	// Get path information from http header
	getPathFromHeader(header, path);
	free(header);
}

/*
* Retrive path from header message
* @param header The request header of request message
* @param path Get filled in by path of requested file
*/
int getPathFromHeader(char *header, char **path) {
	int path_str_len;
	char *path_divider_ptr_start;
	char *path_divider_ptr_end;
	char *slash_ptr;

	// Find first ' ' character in header message
	path_divider_ptr_start = strchr(header,' ');

	if(path_divider_ptr_start == NULL) {
		puts("Cannot resolve http header field!");
		return 1;
	}

	// Find second ' ' character in header message
	path_divider_ptr_end = strchr(path_divider_ptr_start + 1, ' ');
	if(path_divider_ptr_end == NULL) {
		puts("Cannot resolve http header field!");
		return 2;
	}

	// Find the length of path
	path_str_len = path_divider_ptr_end - path_divider_ptr_start - 1;
	slash_ptr = strchr(path_divider_ptr_start, '/');

	if(slash_ptr == NULL) {
		puts("Path format is incorrect!");
		return 3;
	}

	// when path is slash, fill in path with default www index
	if(path_str_len == 1) {

		*path = malloc(strlen(WWW_DEFAULT_INDEX) + 2);
		memset(*path, '\0', strlen(WWW_DEFAULT_INDEX) + 2);

		strcpy(*path, "/");
		strcat(*path, WWW_DEFAULT_INDEX);
		return 0;
	}

	*path = malloc(path_str_len + 2); // Space for path
	memset(*path, '\0', path_str_len + 2);

	// copy path of requested file with slash
	strncpy(*path, path_divider_ptr_start + 1, path_str_len);
	*(*path + path_str_len) = '\0';

	return 0;
}

/*
* Get current with format
* time_str Get filled in formatted time
*/
int format_time(char time_str[]) {
	time_t rawtime;
	struct tm *info;

	time(&rawtime);
	info = gmtime(&rawtime);

	strftime(time_str, 26, "%a, %d %b %Y %H:%M:%S", info);
	return 0;
}

/*
* Read a file from path to buffer
* path The location of the file
*/
int readFileFromPath(char *complete_path, char **buffer) {
	FILE *file;
	int file_len;

	// open file
	file = fopen(complete_path,"r");
	if(file == NULL) {
		return 1;
	}

	// read file content into buffer
	fseek(file, 0,SEEK_END);
	file_len = ftell(file);

	rewind(file);
	if(file_len > 0) {
		*buffer = malloc(file_len + 1);
		memset(*buffer, '\0', file_len + 1);
		fread(*buffer, 1, file_len, file);
	
	}

	fclose(file);
	return 0;
}

/*
* Generate http response message following HTTP protocol
* body The body of response message
* response_msg Get filled in with response message
* status_code The status code of status line of response message
*/
int generateResponseMsg(char *body, char **response_msg, int status_code) {

	char time_str[26];
	char *status_msg;
	int body_len;
	int res_msg_len;

	switch(status_code) {
		case 200:
			status_msg = HTTP_STATUS_OK;
			break;
		case 404:
			status_msg = HTTP_STATUS_NOT_FOUND;
			break;
	}

	body_len = strlen(body);

	// Get format time
	format_time(time_str);

	// Calculate length of response message
	res_msg_len = 
		strlen(HTTP_VERSION) +
		strlen(status_msg) +
		strlen(HTTP_HEADER_DATE) +
		strlen(time_str) + 2 +
		strlen(HTTP_RESPONSE_HEADER_SERVER) +
		strlen(HTTP_RESPONSE_HEADER_ACC_RANG) + 2 +
		body_len;

	// allocate memory for response message
	*response_msg = malloc(res_msg_len + 1);
	memset(*response_msg, '\0', res_msg_len + 1);

	// fill in response message with status line, headers and body
	strcpy(*response_msg, HTTP_VERSION);
	strcat(*response_msg, status_msg);
	strcat(*response_msg, HTTP_HEADER_DATE);
	strcat(*response_msg, time_str);
	strcat(*response_msg, "\r\n");
	strcat(*response_msg, HTTP_RESPONSE_HEADER_SERVER);
	strcat(*response_msg, HTTP_RESPONSE_HEADER_ACC_RANG);
	strcat(*response_msg, "\r\n");

	// add page content into response message
	strcat(*response_msg, body);

	return 0;
}

/*
* client handler
*/
void sigchild_handler(int sockfd) {

	// waitpid() might overwrite errno, so save it
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}

/*
* get sockaddr, IPv4 or IPv6:
*/
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/*
* Check whether request message is completely received
* return 1 if received
*/
int request_completed(char *request_msg) {
	char *res = strstr(request_msg, "\r\n\r\n");
	if( res == NULL) {
		return 0;
	}

	return 1;
}

/*
* Generate file name from path name and status code
*/
int getCompletePath(char *path, char **complete_path, int status) {
	int file_name_len;

	switch(status) {
		case 200:
			// Convert path of file to www/path
			file_name_len = strlen(ROOT_DIR) + strlen(path);

			*complete_path = malloc(file_name_len + 1);
			memset(*complete_path, '\0', file_name_len + 1);
			strcat(*complete_path, ROOT_DIR);
			strcat(*complete_path, path);
			break;
		case 404:
			// 404 Not Found, load 404 page
	 		file_name_len = strlen(ERROR_PAGE_DIR) + strlen(ERROR_404_NOT_FOUND_PAGE) + 1;
	 		*complete_path = malloc(file_name_len + 1);
	 		memset(*complete_path, '\0', file_name_len + 1);
	 		strcpy(*complete_path, ERROR_PAGE_DIR);
	 		strcat(*complete_path, "/");
	 		strcat(*complete_path, ERROR_404_NOT_FOUND_PAGE);
			break;
	}
}

/*
* Client process
* info_ptr This struct contains sockdf, address and addr_len information for current client
*/
void * clientProcess(void * info_ptr) {
	struct clientinfo * info;
	char request_msg[MAX_LEN_GET_REQUEST + 1];

	char addr_str[INET6_ADDRSTRLEN];

	// If not a client, exit thread
	if(!info_ptr) {
		pthread_exit(0);
	}

	// Cast to clientinfo
	info = (struct clientinfo *) info_ptr;

	// Receive request message
    recv_request_msg(info->sock, request_msg);

    // Respond to request message
    respondToHttpRequest(request_msg, info->sock);

    close(info->sock);
    free(info);
    pthread_exit(0);
}
