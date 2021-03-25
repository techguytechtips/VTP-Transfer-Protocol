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
// function to get the size of file passed
int getsize(char file[250]){
	int size;
	FILE *fp;
	// open the file
	fp = fopen(file, "rb");
	if (fp == NULL){
		printf("\033[31mError: Failed to open file! Aborting.\033[0m\n");
		exit(-1);
	}
	// seek to the end and report where the end is
	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	fclose(fp);
	return size;
}

int main(int argc, char* argv[]) {
	// arg checking
	if (argc <  4){
		printf("Error: Not enough args.\nUsage: %s <ip address> <put|get> <file>\n", argv[0]);
		return -1;
	}
	if (strlen(argv[3]) > 255) {
		printf("\033[31mError: File name too large\033[0m\n");
	
	}

	// make the socket fd
	int networksocket = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(9067);
	server_address.sin_addr.s_addr = inet_addr(argv[1]);
	
	// connect to the server
	int status = connect(networksocket, (struct sockaddr *) &server_address, sizeof(server_address));
	if (status < 0){
		printf("\033[31mError: Failed to connect to server! Aborting.\033[0m\n");
		return -1;
	}
	printf("\033[32mConnected!\033[0m\n");
	// vars for sending data
	int size;
	int recvdata;
	int state;
	int total;
	char* file = (char*) malloc(RAM);
	if (file == NULL){
		printf("\033[31mError: Failed to allocate memory! Aborting.\033[0m\n");
		close(networksocket);
		return -1;
	}
	
	int namesize = strlen(argv[3]) + 1;
	send(networksocket, &(namesize),sizeof(int), 0);
	
	// if statement to check the data
	if(strcmp(argv[2], "put") == 0){
		// send the method and the filename
		send(networksocket, argv[2], 4, 0);
		send(networksocket, argv[3], namesize, 0);
		// get the size of the file	
		size = getsize(argv[3]);
		int amountread;
		// send the size of the file
		send(networksocket, &(size), sizeof(size), 0);
		printf("File is %d bytes.\n", size);
		// open the file for reading
		FILE *fp;
		fp = fopen(argv[3], "rb");
		// loop to send the data
		do{
			amountread = fread(file, 1,RAM, fp);
			state = send(networksocket, file, amountread, 0 );
			total = total + state;
			printf("\rtotal: %d", total);
			fflush(stdout);
		
		}while(state > 0 && total < size);
		// close the file
		fclose(fp);
	}
	else if(strcmp(argv[2], "get") == 0){
		// ssend the method and the file name
		send(networksocket, argv[2], 4, 0);
		send(networksocket, argv[3], namesize, 0);
		// open the file for writing		
		FILE *wpf;
		exists(argv[3]);
		wpf = fopen(argv[3], "ab");
		// receive size of file
		recv(networksocket, &size, 4, 0);			
		printf("File is %d bytes.\n", size);

		// main loop for receiving and writing data
		do{
			state = recv(networksocket, file, RAM, 0);
			fwrite(file, 1, state, wpf);
			total = total + state;
			printf("\rwrote: %d", total);
			fflush(stdout);
		}while(state > 0 && total < size);
		// close the file
		fclose(wpf);

	}
	// check valid method
	else{
		printf("\033[31mError: Invalid method!, valid methods: put|get.\033[0m\n");
		close(networksocket);
		return -1;
	}
	// close the socket
	printf("\033[32mFinished! Transferred %d bytes.\033[0m\n", total);
	close(networksocket);
	return 0;
}