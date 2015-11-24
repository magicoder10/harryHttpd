#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>
#include "client.h"

int main(int argc, char *argv[]) {
    char *resHost;
    char *path;
    char *port;
    char *url;
    char *request_msg;
    short show_rtt;
    int loop;

	// char request_msg[MAX_STR_LENGTH];

	switch(argc) {
        case 3:
            show_rtt = 0;
            url = argv[1];
            port = argv[2];

            break;
        case 4:

            if(strcmp("-p", argv[1]) != 0) {
                fprintf(stderr, "usage: ./client -p server_url port_number\n");
                return 2;
            } 

            show_rtt = 1;   
            url = argv[2];
            port = argv[3];
            break;
        default:
    		fprintf(stderr, "usage: ./client server_url port_number\n");
            fprintf(stderr, "usage: ./client -p server_url port_number\n");
    		return 1;
	}

    getHostNameAndPath(url, &path, &resHost);
    generateRequestMsg(path, resHost, &request_msg);
    getHttpResponseFromRequest(resHost, port, request_msg, show_rtt);

    free(resHost);
    free(path);
    free(request_msg);
}

/** Split URL into hostname and path
* @param url The UL to split
* @param path Gets filled in with pointer to newly-allocated 
*     string containing path
* @param resHost Gets filled in with newly-allocated string 
*     containing host name
* @@return error code
*/

int getHostNameAndPath(char *url, char** path, char** resHost) {
    // struct HttpReuqestMessage reuqestMessage;
    char *pathIndex;
    char *slashPtr;
    int host_str_len;
    int url_len;

    // Find first slash character. It marks the end of host name
    slashPtr = strchr(url, '/');

    if (slashPtr == NULL) {
        // No slash, so there is no path.
        // So the hostname is the entire URL.
        // Set the path to "/";
        *resHost = strdup(url); // Just copy the whole URL
        *path = strdup("/");
        return 0;
    } 

    // There is both a host name and a path, separated by slash
    // There is a slash which is the last character

    host_str_len = slashPtr - url; 
    // length of URL up to, but not including the slash == length of host name
    *resHost = malloc(host_str_len + 1); // Space for host
    strncpy(*resHost, url, host_str_len);
    *(*resHost + host_str_len) = '\0'; // Terminate
    *path = strdup(slashPtr);

    return 0;
}

/* Generate http request message
* @param path The path of web page
* @param host The host of web page
* @param request_msg Get filled in with pointer to newly-allocated request msg
* @return error code
*/
int generateRequestMsg(char *path, char *host, char** request_msg) {

    *request_msg = malloc(
        strlen(HTTP_GET_METHOD) 
        + 1 
        + strlen(path)
        + 1
        + strlen(HTTP_VERSION) 
        + 2
        + strlen(HEADER_HOST)
        + strlen(host)
        + 2
        + strlen(HEADER_CONNECT_CLOSE)
        + strlen(HEADER_USERAGENT_CHROME)
        + 2
        + 1);
 
    // Setup request line
    strcpy(*request_msg, HTTP_GET_METHOD);
    strcat(*request_msg, path);
    strcat(*request_msg, HTTP_VERSION);

    // // Setup header lines
    strcat(*request_msg, HEADER_HOST);
    strcat(*request_msg, host);
    strcat(*request_msg, "\r\n");
    strcat(*request_msg, HEADER_CONNECT_CLOSE);
    strcat(*request_msg, HEADER_USERAGENT_CHROME);
    strcat(*request_msg, "\r\n");

    return 0;
}

/* Get addrinfo of target host
* @param host The target hostname
* @param post The post number of target session at remote
*/
int getHostAddrinfo(char *host, char *port,struct addrinfo **res) {
    struct addrinfo hints;
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    // get addrinfo from given hostname and port number
    if((status = getaddrinfo(host, port, &hints, res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }

    return 0;
}

/* Send http request to server and print out response
* @param host The host name of target host
* @param port The port number of target session at remote host
* @param The request message sending to remote host
* @param show_rtt show RTT of the request when is 1;
* @return error code 
*/
int getHttpResponseFromRequest(char *host, char *port, char *request_msg, short show_rtt) {
    int sockfd;
    struct addrinfo hints, *res;
    int status;
    int recv_size;
    struct timeval start, end;
    long rtt;

    char buffer[BUFFER_SIZE];

    getHostAddrinfo(host, port,&res);

    // make a socket
    if((sockfd = socket(PF_INET, res->ai_socktype, res->ai_protocol)) == -1) {
        fprintf(stderr, "socket: %d\n", errno);
        return 2;
    }

    if(show_rtt) {
        gettimeofday(&start, NULL);
    }

    // establish TCP connection to the target server
    if((status = connect(sockfd, res->ai_addr, res->ai_addrlen)) == -1) {
        fprintf(stderr, "connect: %ld\n", errno);
        return 3;
    }

    if(show_rtt) {
        gettimeofday(&end, NULL);
        // RTT = end_time - start_time
        rtt = (end.tv_sec * 1000000 + end.tv_usec) 
            - (start.tv_sec * 1000000 + start.tv_usec);

        printf("RTT: %d\n", rtt);
    }

    // send http request message to server
    if(send(sockfd, request_msg, strlen(request_msg), 0) < 0) {
        fprintf(stderr, "%d\n", errno);
        return 4;
    }

    // receive response message from server
    puts("response message:");
    do {
        if((recv_size = recv(sockfd, buffer, 100, 0)) < 0) {
            fprintf(stderr, "%d\n", errno);
            return 5;
        }
        buffer[recv_size] = '\0';
        fprintf(stdout, "%s", buffer);
    } while (recv_size > 0);

    freeaddrinfo(res);

    return 0;
}
