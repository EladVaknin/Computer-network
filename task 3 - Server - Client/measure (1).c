#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/tcp.h> 
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "transfer.h"
#include <time.h>
#include <sys/time.h>


char buff[MAX_LINE] = {0};  // initialize a buffer
int recieve_file(int sockfd, FILE *fp);  // the function that gets a file
ssize_t total = 0;  // counts the number of bits that we've got

int main(int argc, char *argv[]) {

    int sum1 = 0;  // sum the cubic times
    int sum2 = 0;  // sum the reno times
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Can't allocate sockfd");
        exit(1);
    }
    socklen_t len;  // the size of the socket
    struct sockaddr_in clientaddr, serveraddr;  // the address of the socket
    memset(&serveraddr, 0, sizeof(serveraddr));  // memory set 
    serveraddr.sin_family = AF_INET;  // initialize IPv4
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);  // open the server for listening
    serveraddr.sin_port = htons(SERVERPORT);

    if (bind(sockfd, (const struct sockaddr *) &serveraddr, sizeof(serveraddr)) == -1) {
        perror("Bind Error");
        exit(1);
    }

    if(listen(sockfd, LINSTENPORT) == 0) {
	      printf("Measure Listening....\n");
    } else {
        perror("Listen Error!");
        exit(1);
    }
    socklen_t addrlen = sizeof(clientaddr);  // create an adress for the client
    int connfd = accept(sockfd, (struct sockaddr *) &clientaddr, &addrlen);  
    if (connfd == -1) {
        perror("Connect Error");
        exit(1);
    }
    close(sockfd); 

    char filename[BUFFSIZE] = {0}; 
    if (recv(connfd, filename, BUFFSIZE, 0) == -1) {
        perror("Can't receive filename");
        exit(1);
    }

    FILE *fp = fopen("debug.txt", "w");
    if (fp == NULL) {
        perror("Can't open file");
        exit(1);
    }

    len = sizeof(buff);  // check the type of the socket
    if (getsockopt(connfd, IPPROTO_TCP, TCP_CONGESTION, buff, &len) != 0) { 
        perror("getsockopt");
        return -1;
    } 
    char addr[INET_ADDRSTRLEN];
    printf("Current: %s\n", buff); 

    // the current algorithm is: cubic
    for(int i = 0; i< 5; i++) {
        printf("Start receive file: %s from %s\n", filename, inet_ntop(AF_INET, &clientaddr.sin_addr, addr, INET_ADDRSTRLEN));
        sum1 += recieve_file(connfd, fp);
        printf("Receive Success, NumBytes = %ld\n", total);
    }
    printf("Average receiving time is %f ms\n", sum1/5.0);

    strcpy(buff, "reno"); 
    len = strlen(buff);
     // the current algorithm is: reno
    if (setsockopt(connfd, IPPROTO_TCP, TCP_CONGESTION, buff, len) != 0) {  
      perror("setsockopt"); 
      return -1;
    }
    len = sizeof(buff); 
    if (getsockopt(connfd, IPPROTO_TCP, TCP_CONGESTION, buff, &len) != 0) {
        perror("getsockopt"); 
        return -1; 
    } 
      
    printf("New: %s\n", buff); 
    for(int i = 0; i< 5; i++){
        printf("Start receive file: %s from %s\n", filename, inet_ntop(AF_INET, &clientaddr.sin_addr, addr, INET_ADDRSTRLEN));
        sum2 += recieve_file(connfd, fp);
        printf("Receive Success, NumBytes = %ld\n", total);
    }
    // printf("Average receiving time is %f ms\n", sum2/5.0);

    fclose(fp);
    close(connfd);
    return 0;
}

int recieve_file(int sockfd, FILE *fp) {
    ssize_t n = 0;
    struct timeval start, end;
    double elapsedTime = 0;
    gettimeofday(&start, NULL);

    while ((n = recv(sockfd, buff, MAX_LINE, 0)) > 0) { 
	    total+=n;
      if (n == -1) {
          perror("Receive File Error");
          exit(1);
      }
		
      if (fwrite(buff, sizeof(char), n, fp) != n)	{
          perror("Write File Error");
          exit(1);
      }
		  memset(buff, 0, MAX_LINE);
    }
    gettimeofday(&end, NULL);
    elapsedTime = (end.tv_sec - start.tv_sec) * 1000.0;  // convert seconds to ms
    elapsedTime += (end.tv_usec - start.tv_usec) / 1000.0;  // convert us to ms
    printf("Receiving time is %f ms\n", elapsedTime);
    return elapsedTime;
}
