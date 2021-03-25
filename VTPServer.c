#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/stat.h>
// struct to return to main
typedef struct sizeandfile{
	int size;
	char* file;
}Struct;
int exists(char* file){
	printf("%s\n", file);
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
// read from file and get size
Struct getfile(char file[200]){
	FILE *fp;
	fp = fopen(file, "rb");
		if (fp == NULL){
		printf("\033[31mError: Failed to open file! Aborting.\033[0m\n");
		exit(-1);
	}

	Struct sizestruct;

	Struct *sizestructptr = &sizestruct;


	fseek(fp, 0L, SEEK_END);
	sizestructptr->size = ftell(fp);
	rewind(fp);
	sizestructptr->file = (char*)malloc(sizestructptr->size);
	fread(sizestruct.file, 1, sizestructptr->size, fp);
	printf("File is %d bytes.\n", sizestructptr->size);
	fclose(fp);
	return sizestruct;
}
//rewrite the file with diff name (testing purpose)
int putfile(char* buffer,char *name, int size)
{
	FILE *wfp;
	wfp = fopen(name, "wb");
	printf("\n\nbuffer in putfile(): %s\nsize inside putfile(): %d\n",buffer, size );
	fwrite(buffer, 1, size, wfp);	
	fclose(wfp);
	return 0;
}

int main(int argc, char *argv[]){

	
	// socket stuff
	int serversocket;
	serversocket = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(9067);
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
		printf("%s\n", name);
		// check method
		if (strcmp(action, "get") == 0){
			printf("action: get\n");
			printf("namesize: %d\nname: %s\n", namesize, name);				
			Struct result = getfile(name);			
			int bytesleft = result.size;
			printf("%d", result.size);	
			send(clientsocket, &(result.size), sizeof(result.size), 0);
		
			do{
				state = send(clientsocket, result.file, bytesleft, 0 );
				total = total + state;
				bytesleft = bytesleft - state;
		
			}while(state > 0 && total < result.size);
		}
		else if (strcmp(action, "put") == 0){
			int portion = 1000000000;
			int putsize;
			recv(clientsocket, &putsize, 4, 0);
			
			FILE *wpf;
			char* server_res = malloc(putsize);

			exists(name);
			wpf = fopen(name, "ab");
		
			do{
				state = recv(clientsocket, server_res, portion, 0);
				fwrite(server_res, 1, state, wpf);
				total = total + state;
			}while(state > 0 && total < putsize);
	
			printf("action: put\n");
			close(clientsocket);		
			// do shiz
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
