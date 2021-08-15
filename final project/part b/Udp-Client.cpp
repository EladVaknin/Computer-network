
//
// Udp-Client.cpp Skeleton Ping-Pong.
// Robert Iakobashvili for Ariel uni, BSD/MIT/Apache-license.
//
// 1. Correct the IP-address to be your local-IP address.
// 2. Check that server and client have the same rendezvous port and the same IP-address
// 3. Compile using either MSVC or g++ compiler.
// 4. First run the server.
// 5. In the console run netstat -a to see the server UDP socket at port 5060.
// 6. Run the client and capture the communication using wireshark.
//
#include "stdio.h"

#if defined _WIN32
// See at https://msdn.microsoft.com/en-us/library/windows/desktop/ms740506(v=vs.85).aspx
// link with Ws2_32.lib
#pragma comment(lib,"Ws2_32.lib")
#include <winsock2.h>
#include <ws2tcpip.h>

#else //  linux
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/types.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#define SERVER_IP_ADDRESS "192.168.63.129"
#define SERVER_PORT 5060

int main()
{
#if defined _WIN32
	WSADATA wsaData = { 0 };
	int iResult = 0;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}
#endif

	int s = -1;
	char bufferReply[80] = { '\0' };
	char message[] = "Good morning from Elad and Hila :) \n";
	int messageLen = strlen(message) + 1;

	// Create socket
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) // In Windows -1 is SOCKET_ERROR
	{
		printf("Could not create socket : %d"
#if defined _WIN32
			, WSAGetLastError()
#else
			, errno
#endif
			);
			return -1;
	}

	// Setup the server address structure.
	// Port and IP should be filled in network byte order (learn bin-endian, little-endian)
	//
	struct sockaddr_in6 serverAddress;
	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin6_family = AF_INET;
	serverAddress.sin6_port = htons(SERVER_PORT);
	int rval = inet_pton(AF_INET, (const char*)SERVER_IP_ADDRESS, &serverAddress.sin6_addr);
	if (rval <= 0)
	{
		printf("inet_pton() failed");
		return -1;
	}

	//send the message
	if (sendto(s, message, messageLen, 0, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) == -1)
	{
		printf("sendto() failed with error code  : %d"
#if defined _WIN32
			, WSAGetLastError()
#else
			, errno
#endif
			);
			return -1;
	}

	struct sockaddr_in6 fromAddress;
	int fromAddressSize = sizeof(fromAddress);

	memset((char *)&fromAddress, 0, sizeof(fromAddress));

	// try to receive some data, this is a blocking call
	if (recvfrom(s, bufferReply, sizeof(bufferReply) -1, 0, (struct sockaddr *) &fromAddress, (socklen_t*) &fromAddressSize) == -1)
	{
		printf("recvfrom() failed with error code  : %d"
#if defined _WIN32
			, WSAGetLastError()
#else
			, errno
#endif
			);
		return -1;
	}

	printf("%s", bufferReply);

#if defined _WIN32
	closesocket(s);
	WSACleanup();
#else
	close(s);
#endif

    return 0;
}
