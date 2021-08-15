#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h> 
#include <sys/socket.h>
#include "transfer.h"


void sendfile(FILE *fp, int sockfd);
ssize_t total = 0;  // counts the number of bits that we've got

int main(int argc, char* argv[]) {
    if (argc != 3) {  // should get 3 parameters (sender, data.txt and localhost)
        perror("usage:send_file filepath <IPaddress>");
        exit(1);
    }
    socklen_t len;  // the size of the socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd < 0) {
        perror("Can't allocate sockfd");
        exit(1);
    }

    char buff[BUFFSIZE] = {0};
    struct sockaddr_in serveraddr;  // socket adress
    memset(&serveraddr, 0, sizeof(serveraddr));  // memory set
    serveraddr.sin_family = AF_INET;  // IPv4
    serveraddr.sin_port = htons(SERVERPORT);  
    // convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, argv[2], &serveraddr.sin_addr) < 0) {
        perror("IPaddress Convert Error");
        exit(1);
    }
    // connect to the server
    if (connect(sockfd, (const struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {
        perror("Connect Error");
        exit(1);
    }
    
    char *filename = basename(argv[1]);  // data.txt
    if (filename == NULL) {
        perror("Can't get filename");
        exit(1);
    }
    
    strncpy(buff, filename, strlen(filename));  // copy the filname to the memory
    if (send(sockfd, buff, BUFFSIZE, 0) == -1) {
        perror("Can't send filename");
        exit(1);
    }
    
    FILE *fp = fopen(argv[1], "r");  // open data.txt file
    if (fp == NULL) {
        perror("Can't open file");
        exit(1);
    }

    len = sizeof(buff); 
    if (getsockopt(sockfd, IPPROTO_TCP, TCP_CONGESTION, buff, &len) != 0) { 
        perror("getsockopt");
        return -1;
    } 

    printf("Current: %s\n", buff); 

    // the current algorithm is: cubic (default)
    for(int i = 0; i< 5; i++) {
        sendfile(fp, sockfd);
        printf("Send Success, NumBytes = %ld\n", total);
    }

    strcpy(buff, "reno"); 
    len = strlen(buff);
    // the current algorithm is: reno
    if (setsockopt(sockfd, IPPROTO_TCP, TCP_CONGESTION, buff, len) != 0) {
        perror("setsockopt"); 
        return -1;
    }
    len = sizeof(buff); 
    if (getsockopt(sockfd, IPPROTO_TCP, TCP_CONGESTION, buff, &len) != 0) {
        perror("getsockopt"); 
        return -1; 
    } 
    
    printf("New: %s\n", buff); 
    for(int i = 0; i< 5; i++){
        sendfile(fp, sockfd);
        printf("Send Success, NumBytes = %ld\n", total);
    }

    fclose(fp);
    close(sockfd);
    return 0;
}

void sendfile(FILE *fp, int sockfd) {
    int n; 
    char sendline[MAX_LINE] = {0}; 
    while ((n = fread(sendline, sizeof(char), MAX_LINE, fp)) > 0) {
	    total+=n;
        if (n != MAX_LINE && ferror(fp)) {
            perror("Read File Error");
            exit(1);
        }   
        if (send(sockfd, sendline, n, 0) == -1) {
            perror("Can't send file");
            exit(1);
        }
        memset(sendline, 0, MAX_LINE);
    }
}
