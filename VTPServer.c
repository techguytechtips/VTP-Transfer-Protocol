#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/stat.h>
// max ram 50 MB for buffer
#define RAM 52428800

// function to check if the file exists
int exists(char* file){
	struct stat buffer;
	int exist = stat(file,&buffer);
	// stat returns 0 if it reads the file which means it exists
	if (exist == 0)
	{
		printf("File exists, overwrite? [y|n]: ");
		if(getchar() == 'y'){
			printf("Overwriting.\n");
			// delete the file
			char rm[263] = "rm -rf ";
			strcat(rm, file);
			system(rm);
			return 0;
		}
		else{
			printf("Not overwriting.\n");
			return 0;
		}
	}
	else
		return 0;
}
// function to get the size of the file passed
int getsize(char file[256]){
	unsigned long size;
	FILE *fp;
	// open the file
	fp = fopen(file, "rb");
		if (fp == NULL){
		return 0;
	}
	
// seek to the end then report where the end is
	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	fclose(fp);
	return size;
}

int main(int argc, char *argv[]){
	// check if a port was specified, otherwise use default
	int port;
	if(argc > 1){
		port = atoi(argv[1]);

	}
	else 
	{
		printf("Note: Using default port 9067 as none was specified\n");
		port = 9067;
	}
	
	// make the socket fd
	int serversocket;
	serversocket = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	server_address.sin_addr.s_addr = INADDR_ANY;
	bind(serversocket, (struct sockaddr*) &server_address, sizeof(server_address));
	// listen for connnections
	listen(serversocket, 2);
	int clientsocket = accept(serversocket, NULL, NULL);


	printf("\033[32mConnected!\033[0m\n");
		// declare vars
		unsigned long size = 0;
		int state;
		unsigned long total;
		short namesize;
		char action[4];
		char name[256];
		// get the size of the file name
		recv(clientsocket, &namesize, sizeof(short), 0);
		namesize = ntohs(namesize);
		// get the method (put or get)
		recv(clientsocket, &action, sizeof(action), 0);
		// receive the file name
		if (recv(clientsocket, &name, namesize, 0) < 1)
		{
			printf("\033[31mError: Failed receiving data! Aborting.\033[0m\n");
			close(clientsocket);
			close(serversocket);
			return -1;
			
		}

		// allocate memory
		char* file = (char*) malloc(RAM);
		if(file == NULL){
			printf("\033[31mError: Failed to allocate memory! Aborting.\033[0m\n");
			close(clientsocket);
			close(serversocket);
			return -1;
		}
		// if statement to check the method
		if (strcmp(action, "get") == 0){
			printf("action: get\n");
			// get size of file
			size = getsize(name);
			if (size == 0){
				printf("\033[31mError: File not found or is empty! Aborting.\033[0m\n");
				close(clientsocket);
				close(serversocket);
				free(file);
				return -1;
			}
			FILE *fp;
			// open the file
			fp = fopen(name, "rb");
			unsigned long amountread;
			unsigned long networksize = htonl(size);
			send(clientsocket, &networksize, sizeof(unsigned long), 0);
			// loop to send the data
			do{
				amountread = fread(file, 1,RAM,fp);
				state = send(clientsocket, file, amountread, 0 );
				total = total + state;
		
			}while(state > 0 && total < size);
			fclose(fp);
			close(clientsocket);
		}
		else if (strcmp(action, "put") == 0){
			printf("action: put\n");
			// get the size of the incoming file
			recv(clientsocket, &size, 4, 0);
			size = ntohl(size);
			// check if it exists
			exists(name);
			// open the file
			FILE *wfp;
			wfp = fopen(name, "ab");
			// main loop for receiving and writing data
			do{
				state = recv(clientsocket, file, RAM, 0);
				fwrite(file, 1, state, wfp);
				total = total + state;
			}while(state > 0 && total < size);
			fclose(wfp);
			close(clientsocket);
		}
		else{
			printf("\033[31mError: Invalid method! Aborting.\033[0m\n");
			close(clientsocket);
			close(serversocket);
			free(file);
			return -1;
		}
	free(file);
	printf("\033[32mFinished! sent %lu bytes\033[32m\n", total);
	close(serversocket);
	return 0;
}
