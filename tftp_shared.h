#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BUFFER_SIZE 516
#define MAX_COMMAND_SIZE 500
#define MAX_FILENAME_SIZE 255
#define MAX_PATH_SIZE 255
#define MAX_MSG_LENGTH 100
#define MAX_DATA_SIZE 512

#define RRQ 1
#define DATA 3
#define ACK 4
#define ERROR 5

#define ILLEGAL_TFTP_OPERATION 4
#define FILE_NOT_FOUND 1

void stampa(char* buffer, int len){
	int i;
	for(i = 0; i < len; ++i){
		if(buffer[i] == '\0'){
			printf("Z");
			continue;
		}
		printf("%c", buffer[i]);
	}
	printf("\n");
}


