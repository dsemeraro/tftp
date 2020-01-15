#include "tftp_shared.h"

int copia_blocco_txt(char *buf, FILE *file){
	int i;
	for(i = 0; i < MAX_DATA_SIZE; ++i){
		buf[i] = fgetc(file);
		if(feof(file))
		 	return i;
	}
	return MAX_DATA_SIZE;
}

int crea_pacchetto_errore(char *buffer, int error_c, char* error_msg){
	uint16_t opcode = htons(ERROR);
	uint16_t error_code = htons(error_c);
	int ret = 0;

	memcpy(buffer, &opcode, 2);
	memcpy(&buffer[2], &error_code, 2);
	strcpy(&buffer[4], error_msg);
	ret = 4 + strlen(error_msg) + 1;
	return ret;
}

int crea_pacchetto_dati(char *buffer, int block, FILE *file, char *mode){
	uint16_t opcode = htons(DATA);
	uint16_t block_n = htons(block);
	int ret;
	
	memcpy(buffer, &opcode, 2);
	memcpy(&buffer[2], &block_n, 2);

	if(strcmp(mode, "netascii") == 0)
		ret = copia_blocco_txt(&buffer[4], file);
	else
		ret = fread(&buffer[4], 1, MAX_DATA_SIZE, file);

	return (ret + 4);
}

void gestisci_richiesta(char *buffer, struct sockaddr_in *cl_addr, char *dir){
	int sd, index, pack_len, data_len, ret;
	uint16_t opcode;
	char filename[MAX_FILENAME_SIZE];
	char mode[9];
	char complete_path[MAX_FILENAME_SIZE + MAX_PATH_SIZE];
	FILE *file;
	
	sd = socket(AF_INET, SOCK_DGRAM, 0);
	connect(sd,(struct sockaddr*) cl_addr, sizeof(*cl_addr));
	
	memcpy(&opcode, buffer, 2);
	opcode = ntohs(opcode);
	index = 2;
	strcpy(filename, &buffer[index]);
	index += strlen(filename) + 1;
	strcpy(mode, &buffer[index]);
	
	strcpy(complete_path, dir);
	strcat(complete_path, filename);
	
	if(opcode != RRQ){
		pack_len = crea_pacchetto_errore(buffer, ILLEGAL_TFTP_OPERATION, "Ricevuta operazione errata!");
		ret = send(sd, (void*)buffer, pack_len, 0);
		if(ret < 0){
			fprintf(stderr, "%d: ", getpid());
			perror("sendto RRQ opcode errato");
			exit(1);
		}
		printf("%d: Ricevuta operazione errata\n", getpid());
		return;
	} else {
		int block = 1;
		printf("%d: richiesto file: %s\n", getpid(), complete_path);
		
		if(strcmp(mode, "netascii") == 0)
			file = fopen(complete_path, "r");
		else
			file = fopen(complete_path, "rb");
		
		if(file == NULL){
			pack_len = crea_pacchetto_errore(buffer, FILE_NOT_FOUND, "File non trovato.");
			ret = send(sd, (void*)buffer, pack_len, 0);
			if(ret < 0){
				fprintf(stderr, "%d: ", getpid());
				perror("sendto file non trovato");
				exit(1);
			}
			printf("%d: File non presente\n", getpid());
			exit(1);
		}
		do {	
			
			pack_len = crea_pacchetto_dati(buffer, block, file, mode);
			ret = send(sd, (void*)buffer, pack_len, 0);
			if(ret < 0){
				fprintf(stderr, "%d: ", getpid());
				perror("errore nella send data");
				exit(1);
			}
			data_len = pack_len - 4;
			/*
			usleep(10000);
			printf("%d: invio blocco %d\n", getpid(), block);
			*/
			ret = recv(sd, (void*)buffer, 4, 0);
			if(ret < 0){
				fprintf(stderr, "%d: ", getpid());
				perror("errore nella recv ack");
				exit(1);
			}
			++block;
		} while(data_len == MAX_DATA_SIZE);

		printf("%d: Traferimento completato\n", getpid());
		fclose(file);
	}
}

int main(int argc, char** argv){

	int sd, ret;
	uint16_t port;
	socklen_t len;
	struct sockaddr_in my_addr, cl_addr;
	pid_t pid;
	char directory[MAX_PATH_SIZE];
	char buffer[BUFFER_SIZE];
	
	if(argc != 3){
		printf("Inserire il giusto numero di argomenti!\n");
		exit(1);
	}
	memset(buffer, 0, BUFFER_SIZE);
	memset(directory, 0, MAX_PATH_SIZE);
	strcpy(directory, argv[2]);
	directory[strlen(argv[2])] = '\0';
		
	port = atoi(argv[1]);
	
	printf("Creazione del socket di ascolto..\n");
	sd = socket(AF_INET, SOCK_DGRAM, 0);	
	
	if(sd < 0){
		perror("Creazione socket fallita!\n");
		exit(1);
	}
	
	memset(&cl_addr, 0, sizeof(cl_addr));
	len = sizeof(cl_addr);
	
	memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	
	ret = bind(sd, (struct sockaddr*) &my_addr, sizeof(my_addr));
	if(ret < 0){
		perror("Errore in fase di binding\n");
		exit(1);
	}
	
	printf("Server in ascolto\n");
	
	while(1){
		ret = recvfrom(sd, (void*) buffer, BUFFER_SIZE, 0, (struct sockaddr*) &cl_addr, &len);
		if(ret < 0){
			perror("errore nella recv del socket di ascolto");
			exit(1);
		}
		
		printf("Connessione accettata. Creazione processo di comunicazione con client.\n");
		pid = fork();
		if(pid < 0){
			printf("Errore nella creazione del processo\n");
			continue;
		}
		if(pid == 0){
			close(sd);
			printf("Creato processo di comunicazione con client: %d\n", getpid());		
			gestisci_richiesta(buffer, &cl_addr, directory);
			return 0;
		}
	}
	close(sd);

}

