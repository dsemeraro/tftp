CC = gcc
CFLAGS = -Wall

all: tftp_server tftp_client 

tftp_client: tftp_client.c
	$(CC) $(CFLAGS) -o tftp_client tftp_client.c

tftp_server: tftp_server.c
	$(CC) $(CFLAGS) -o tftp_server tftp_server.c

clean:
	rm -f tftp_server tftp_client


