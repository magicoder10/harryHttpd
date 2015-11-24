#ifndef HTTP_SERVER
#define HTTP_SERVER
#define MAX_CONNECTION (20)
#define MAX_BUFFER_SIZE (50)
#define MAX_LEN_GET_REQUEST (64000)

#define HTTP_VERSION ("HTTP/1.1 ")
#define HTTP_STATUS_OK ("200 OK\r\n")
#define HTTP_STATUS_NOT_FOUND ("404 Not Found\r\n")
#define HTTP_HEADER_DATE ("Date: ")
#define HTTP_RESPONSE_HEADER_SERVER ("Server: Harry Mini Http Server/0.0.1\r\n")
#define HTTP_RESPONSE_HEADER_ACC_RANG ("Accept-Ranges: bytes\r\n")

#define ROOT_DIR ("www")
#define ERROR_PAGE_DIR ("error_pages")
#define ERROR_404_NOT_FOUND_PAGE ("404.html")
#define WWW_DEFAULT_INDEX ("index.html")

struct clientinfo {
	int sock;
	struct sockaddr address;
	int addr_len;
};

int getAddrinfoOfLocalHost(char *port, struct addrinfo **res);
int startServer(char *port);
int recv_request_msg(int client_sock_fd, char request_msg[]);
int getPathFromHeader(char *header, char **path);
int getPathFromRequestMsg(char request_msg[], char **path);
int respondToHttpRequest(char request_msg[], int client_sock_fd);
int readFileFromPath(char *complete_path, char **buffer);
int generateResponseMsg(char *body, char **response_msg, int status_code);
int format_time(char time_str[]);
void *get_in_addr(struct sockaddr *sa);
void sigchild_handler(int sockfd);
int request_completed(char *request_msg);
int getCompletePath(char *path, char **complete_path, int status);
void * clientProcess(void * info_ptr);
#endif
