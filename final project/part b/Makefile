all: Udp-Server Udp-Client
 
Udp-Server: Udp-Server.o
	g++ -o Udp-Server Udp-Server.o

Udp-Server.o: Udp-Server.cpp
	g++ -std=c++11 -c Udp-Server.cpp

Udp-Client: Udp-Client.o
	g++ -o Udp-Client Udp-Client.o

Udp-Client.o: Udp-Client.cpp
	g++ -std=c++11 -c Udp-Client.cpp

.PHONY: clean all

clean:
	rm *.o Udp-Server Udp-Client
