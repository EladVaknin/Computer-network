CC = gcc
CFLAG = -Wall -Wextra -g -o

all: 
	$(CC) myping.c $(CFLAG) ping.o
	$(CC) sniffer.c $(CFLAG) sniffer.o -lpcap

git:
	git add -A
	git commit -m "$m"
	git push

ping:
	$(CC) myping.c $(CFLAG) ping.o
	sudo ./ping.o

sniff:
	$(CC) sniffer.c $(CFLAG) sniffer.o -lpcap
	sudo ./sniffer.o

clean:
	rm -f *.o 
