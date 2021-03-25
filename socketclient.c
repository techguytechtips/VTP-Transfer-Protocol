#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/stat.h>
// max ram 1 GIG
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
		else{
			printf("Not overwriting.\n");
			return 0;
		}
	}
	else
		return 0;
}
int getsize(char file[250]){
	int size;
	FILE *fp;
	fp = fopen(file, "rb");
	if (fp == NULL){
		printf("\033[31mError: Failed to open file! Aborting.\033[0m\n");
		exit(-1);
	}
	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	fclose(fp);
	return size;
}

int main(int argc, char* argv[]) {
	// arg checing
	if (argc <  4){
		printf("Error: Not enough args.\nUsage: %s <ip address> <put|get> <file>\n", argv[0]);
		return -1;
	}	
	// check if the file exists and open it		


	// make the socket fd
	int networksocket = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(9067);
	server_address.sin_addr.s_addr = inet_addr(argv[1]);

	int status = connect(networksocket, (struct sockaddr *) &server_address, sizeof(server_address));
	if (status < 0){
		printf("\033[31mError: Failed to connect to server! Aborting.\033[0m\n");
		return -1;
	}
	printf("\033[32mConnected!\033[0m\n");
	int size;
	int recvdata;
	int state;
	int total;
	char* file = (char*) malloc(RAM);
	int namesize = strlen(argv[3]) + 1;
	send(networksocket, &(namesize),sizeof(int), 0);
	//send name to server
	
	// check method and send data
	if(strcmp(argv[2], "put") == 0){
		send(networksocket, argv[2], 4, 0);
		send(networksocket, argv[3], namesize, 0);	
		size = getsize(argv[3]);
		int amounttoread;
		int amountread;
		send(networksocket, &(size), sizeof(size), 0);
		printf("File is %d bytes.\n", size);		
		FILE *fp;
		fp = fopen(argv[3], "rb");
		do{
			amountread = fread(file, 1,RAM, fp);
			state = send(networksocket, file, amountread, 0 );
			total = total + state;
			printf("\rtotal: %d", total);
			fflush(stdout);
		
		}while(state > 0 && total < size);
	}
	else if(strcmp(argv[2], "get") == 0){
		send(networksocket, argv[2], 4, 0);
		send(networksocket, argv[3], namesize, 0);		
		FILE *wpf;
		exists(argv[3]);
		wpf = fopen(argv[3], "ab");
		
		recv(networksocket, &size, 4, 0);			
		printf("File is %d bytes.\n", size);			

		if (file == NULL){
			printf("\033[31mError: Failed to allocate memory! Aborting.\033[0m\n");
			close(networksocket);
			fclose(wpf);
		return -1;
		}
		
		
		do{
			state = recv(networksocket, file, RAM, 0);
			fwrite(file, 1, state, wpf);
			total = total + state;
			printf("\rwrote: %d", total);
			fflush(stdout);
		}while(state > 0 && total < size);
		fclose(wpf);

	}
	else{
		printf("\033[31mError: Invalid method!, valid methods: put|get.\033[0m\n");
		close(networksocket);
		return -1;
	}
	printf("\033[32mFinished! Wrote %d bytes.\033[0m\n", total);
	close(networksocket);
	return 0;
}
