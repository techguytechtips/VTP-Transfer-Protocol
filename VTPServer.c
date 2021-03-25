#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/stat.h>
#define RAM 52428800

int exists(char* file){
	struct stat buffer;
	int exist = stat(file,&buffer);
	if (exist == 0)
	{
		printf("File exists, overwrite? [y|n]: ");
		if(getchar() == 'y'){
			printf("Overwriting.\n");
			char rm[263] = "rm -rf ";
			strcat(rm, file);
			system(rm);
			return 0;
		}
		else
			printf("Not overwriting.\n");
			return 0;
	}
	else
		return 0;
}

int getsize(char file[256]){
	FILE *fp;
	fp = fopen(file, "rb");
		if (fp == NULL){
		printf("\033[31mError: Failed to open file! Aborting.\033[0m\n");
		exit(-1);
	}
	int size;


	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	fclose(fp);
	return size;
}

int main(int argc, char *argv[]){
	int port;
	
	if(argc > 1){
		port = atoi(argv[1]);

	}
	else 
	{
		printf("Note: Using default port 9067 as none was specified\n");
		port = 9067;
	}
	
	// socket stuff
	int serversocket;
	serversocket = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	server_address.sin_addr.s_addr = INADDR_ANY;
	bind(serversocket, (struct sockaddr*) &server_address, sizeof(server_address));
	listen(serversocket, 2);
	int clientsocket = accept(serversocket, NULL, NULL);
	int state;
	int total;

	printf("\033[32mConnected!\033[0m\n");
		int namesize;
		recv(clientsocket, &namesize, sizeof(int), 0);
		
		char action[4];		
		recv(clientsocket, &action, sizeof(action), 0);
		char name[256];		
		
		if (recv(clientsocket, &name, namesize, 0) < 1)
		{
			printf("\033[31mError: Failed receiving data! Aborting.\033[0m\n");
			close(clientsocket);
			return -1;
			
		}
		// check method
		char* file = (char*) malloc(RAM);
		if(file == NULL){
			printf("\033[31mError: Failed to allocate memory! Aborting.\033[0m\n");
			close(clientsocket);
			return -1;

		}
		if (strcmp(action, "get") == 0){
			printf("action: get\n");
			int size = getsize(name);
			FILE *fp;
			fp = fopen(name, "rb");
			int amountread;
			int bytesleft = size;
			send(clientsocket, &(size), sizeof(int), 0);
		
			do{
				amountread = fread(file, 1,RAM,fp);
				state = send(clientsocket, file, amountread, 0 );
				total = total + state;
				bytesleft = bytesleft - state;
		
			}while(state > 0 && total < size);
		}
		else if (strcmp(action, "put") == 0){
			printf("action: put\n");	
			int putsize;
			recv(clientsocket, &putsize, 4, 0);
			
			FILE *wpf;
			char* server_res = malloc(putsize);

			exists(name);
			wpf = fopen(name, "ab");
		
			do{
				state = recv(clientsocket, file, RAM, 0);
				fwrite(file, 1, state, wpf);
				total = total + state;
			}while(state > 0 && total < putsize);
	
			
			close(clientsocket);		
		}
		else{
			printf("\033[31mError: Invalid method! Aborting.\033[0m\n");
			close(clientsocket);
			return -1;
		}

	printf("\033[32mFinished! sent %d bytes\033[32m\n", total);
	close(serversocket);
	return 0;
}
