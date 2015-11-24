#ifndef HTTP_CLIENT
#define HTTP_CLIENT

#define MAX_STR_LENGTH (100)
#define BUFFER_SIZE (60000)
#define HTTP_GET_METHOD ("GET ")
#define HTTP_VERSION (" HTTP/1.1\r\n")
#define HEADER_HOST ("Host: ")
#define HEADER_CONNECT_CLOSE ("Connection: close\r\n")
#define HEADER_USERAGENT_CHROME ("User-agent: Mozilla/5.0 (Windows NT 6.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/41.0.2228.0 Safari/537.36\r\n")

int getHostNameAndPath(char *url, char **path, char **resHost);
int generateRequestMsg(char *path, char *host, char **request_msg);
int getHostAddrinfo(char *host, char *port,struct addrinfo **res);
int getHttpResponseFromRequest(char *host, char *port, char *request_msg, short show_rtt);
#endif
