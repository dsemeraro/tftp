#include "tftp_shared.h"

void stampa_comandi(){
	printf(	"Sono disponibili i seguenti comandi:\n"
			"!help --> mostra l'elenco dei comandi disponibili\n"
			"!mode {txt|bin} --> imposta il modo di trasferimento dei files (testo o binario)\n"
			"!get filename nome_locale --> richiede al server il file di nome <filename> e lo salva localmente con il nome <nome_locale>\n"
			"!quit --> termina il client\n\n");
}

void scrivi_dati_txt(FILE *fd, char *buffer, int size){
	int i;
	for(i = 0; i < size; ++i){
		fprintf(fd, "%c", buffer[i]);
	}
}

int crea_pacchetto_richiesta(char* buffer, char* filename, char* mode){
	int index = 0;
	uint16_t opcode = htons(RRQ);
	memset(buffer, 0x73, 50);
	memcpy(buffer, &opcode, 2);
	index += 2;
	
	strcpy(&buffer[index], filename);
	index += strlen(filename) + 1;

	if(strcmp(mode, "txt") == 0){
		strcpy(&buffer[index], "netascii");
		index += strlen("netascii") + 1;
	} else {
		strcpy(&buffer[index], "octect");
		index += strlen("octect") + 1;
	}
	return index;
}

int crea_pacchetto_ack(char* buffer, uint16_t block_ack){
	uint16_t opcode = htons(ACK);
	uint16_t block = htons(block_ack);
	
	memcpy(buffer, &opcode, 2);
	memcpy(&buffer[2], &block, 2);
	return 4;
}

void scarica_file(int sd, char* nome_locale, char *mode){
	socklen_t len;
	int block = 0, block_srv, data_len, ret, file_created = 0;
	uint16_t opcode;
	char buffer[BUFFER_SIZE];
	FILE *file;
	struct sockaddr_in download_addr;
	
	memset(&download_addr, 0, sizeof(download_addr));
	len = sizeof(download_addr);
		
	do{
		ret = recvfrom(sd, (void*) buffer, BUFFER_SIZE, 0, (struct sockaddr*) &download_addr, &len);
		if(ret < 0){
			perror("errore nella recvfrom");
			return;
		}

		memcpy(&opcode, buffer, 2);
		opcode = ntohs(opcode);

		if(opcode == ERROR){

			printf("Errore: %s\n", &buffer[4]);
			return;
			
		} else if (opcode == DATA) {
			
		
			++block;
			if(block == 1){
				printf("Trasferimento file in corso.\n");
				
				if(strcmp(mode, "txt") == 0)
					file = fopen(nome_locale, "w");
				else
					file = fopen(nome_locale, "wb");
				
				++file_created;
			}
			data_len = ret - 4;

			memcpy(&block_srv, &buffer[2], 2);
			block_srv = htons(block_srv);

			if(block == block_srv)
				if(data_len > 0){
					if(strcmp(mode, "txt") == 0)
						scrivi_dati_txt(file, &buffer[4], data_len);
					else 
						ret = fwrite((void *) &buffer[4], 1, data_len, file);
				}
			ret = crea_pacchetto_ack(buffer, block);
			ret = sendto(sd, buffer, ret, 0, (struct sockaddr *) &download_addr, len);
			if(ret < 0){
				perror("errore sendto ack:");
				return;
			}
			
		}
		
	} while(data_len == MAX_DATA_SIZE);
	fclose(file);
	printf("Trasferimento completato (%d/%d blocchi).\n", block, block);
	printf("Salvataggio %s completato.\n", nome_locale);
}

int main(int argc, char** argv){
	int sd, ret, pack_len;
	uint16_t port;
	char buffer[BUFFER_SIZE];
	char linea_comando[MAX_COMMAND_SIZE];
	char comando[6];
	char mode[4] = {"bin"};
	char filename[MAX_FILENAME_SIZE];
	char nome_locale[MAX_FILENAME_SIZE];
	struct sockaddr_in srv_addr;
	
	if(argc != 3){
		printf("Inserire il giusto numero di argomenti!\n");
		exit(1);
	}
	
	memset(buffer, 0, BUFFER_SIZE);
	memset(filename, 0, strlen(filename));
	memset(nome_locale, 0, strlen(nome_locale));
	memset(&srv_addr, 0, sizeof(srv_addr));

	stampa_comandi();
	
	port = atoi(argv[2]);
	
	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sd < 0){
		perror("Creazione socket fallita\n");
		exit(1);	
	}
	
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port =	htons(port);
	inet_pton(AF_INET, argv[1], (void*)&srv_addr.sin_addr);
	
	while(1){
		printf("> ");
		fgets(linea_comando, MAX_COMMAND_SIZE, stdin);
		if(strlen(linea_comando) > 1){
			sscanf(linea_comando, "%s", comando);	
		
			if(strcmp(comando, "!help") == 0){
			
				stampa_comandi();
			
			} else if(strcmp(comando, "!mode") == 0){
			
				char temp[4];
				sscanf(linea_comando, "%s %s", comando, temp);
				
				if(strcmp(temp, "txt") == 0){

					strcpy(mode, temp);
					printf("Modo di trasferimento testo configurato.\n");

				} else if(strcmp(temp, "bin") == 0){

					strcpy(mode, temp);
					printf("Modo di trasferimento binario configurato.\n");

				} else

					printf("Sintassi errata (usare !mode {bin|txt}).\n");

			} else if(strcmp(comando, "!get") == 0){
				
				sscanf(linea_comando, "%s %s %s", comando, filename, nome_locale);

				if(strlen(filename) <= 0 || strlen(nome_locale) <= 0){
					printf("Sintassi comando errata (usare !get filename nome_locale).\n");
					continue;
				}
				
				pack_len = crea_pacchetto_richiesta(buffer, filename, mode);
				ret = sendto(sd, (void*)buffer, pack_len, 0, (struct sockaddr*) &srv_addr, sizeof(srv_addr));
				if(ret < 0){
					perror("Invio richiesta fallita.");
				}
				else{
					printf("Richiesta file %s al server in corso.\n", filename);
					scarica_file(sd, nome_locale, mode);
				}
								
				//pulizia delle variabili
				strcpy(filename, "");
				strcpy(nome_locale, "");
			
			} else if(strcmp(comando, "!quit") == 0){
				close(sd);
				return 0;
			} else {
				printf("Comando errato [!help]\n");
			}
		}
	}
}

