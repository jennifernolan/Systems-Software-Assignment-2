#include <stdio.h> //for IO
#include <string.h> //for strlen
#include <sys/socket.h> //for socket
#include <arpa/inet.h> //for inet_addr
#include <unistd.h> //for write
#include <pthread.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

void *print_message_function(void *ptr);

//file synchronization
pthread_mutex_t lockFile;

int main(int argc, char *argv[])
{
	int s; //socket descriptor
	int cs; //client socket
	int connSize; //size of struct

	struct sockaddr_in server, client;

	//create socket
	s = socket(AF_INET, SOCK_STREAM, 0);
	if(s == -1)
	{
		printf("Could not create socket\n");
	} else {
		//printf("Socket successfully created!\n");
	}

	//set sockaddr_in variable
	server.sin_port = htons(8081); // set the port for communication
	server.sin_family = AF_INET; //use IPV4 protocol
	server.sin_addr.s_addr = INADDR_ANY; //when INADDR_ANY is specified in the bind call, the socket will be bound to all local interfaces

	//bind
	if(bind(s, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		perror("Bind issue!\n");
		return 1;
	} else {
		//printf("Bind complete!\n");
	}

	//listen for a connection
	listen(s, 3);

	//accept an incoming connection
	printf("Waiting for incoming connections from Client>>\n");
	connSize = sizeof(struct sockaddr_in);

	//accept connection from an incoming client
	while(cs = accept(s, (struct sockaddr *)&client, (socklen_t*)&connSize))
	{
		if(cs < 0)
		{
			perror("Can't establish connection\n");
			return 1;
		} else {
			printf("Connection from client accepted!\n");
			
			pthread_t thread;
			
			int *client_sock = malloc(200);
			*client_sock = cs;

			//threading to allow multiple client connections at once
			if(pthread_create(&thread, NULL, print_message_function, (void *) client_sock) < 0)
			{
				perror("Failed to create thread for client\n");
			}
		}
	}
	return 0;

}

void *print_message_function(void *ptr)
{
	int READSIZE; //size of sockaddr_in for client connection
	int sock = *(int *) ptr;	
	
	const char *filename = "/home/jennifer/Documents/Assignment2/users.txt";
	FILE *fileOpen = fopen(filename, "r");
	char line[256];
	int found = 0;
	char username[500];
	memset(username, 0, 500);

	recv(sock, username, 500, 0);
	
	//validate the entered credentials are in the user file
	while(fgets(line, sizeof(line), fileOpen))
	{
		if(strcmp(line, username) == 0)
		{
			found = 1;
		}
	}

	char *token = strtok(username, " ");
	printf("Username: %s\n", token);
	static char uname[50];
	strcpy(uname, token);

	//get the autenticated users permissions
	struct passwd *permissions;
	if((permissions = getpwnam(token)) != NULL)
	{
		printf("UserID: %d\n", permissions->pw_uid);
	}

	int user_uid = permissions->pw_uid;
	int group_gid = permissions->pw_gid;
	int j, ngroups;
	gid_t *groups;
	ngroups = 10;
	struct group *gr;

	groups = malloc(ngroups * sizeof(gid_t));
	
	if(getgrouplist(token, group_gid, groups, &ngroups) == -1)
	{
		printf("No groups found\n");
	}

	if(found == 1)
	{
		send(sock, "User authenticated", strlen("User authenticated"), 0);
		printf("\nLogin completed\n");

		char menuOption[2];
		memset(menuOption, 0, 50);
		READSIZE = recv(sock, menuOption, 50, 0);

		//determine which directory the file is being transferred to
		char file[20] = "";
		if(strcmp(menuOption, "1") == 0)
		{
			strcpy(file, "intranet");
		} else if(strcmp(menuOption, "2") == 0)
		{
			strcpy(file, "sales");
		} else if(strcmp(menuOption, "3") == 0)
		{
			strcpy(file, "promotions");
		} else if(strcmp(menuOption, "4") == 0)
		{
			strcpy(file, "offers");
		} else if(strcmp(menuOption, "5") == 0)
		{
			strcpy(file, "marketing");
		} else {
			printf("\nNo selection made\n");
		}

		char *fileName = file;

		int access = 0;
		
		//determine whether the authenticated user has access to the desired transfer directory 
		for(int j = 0; j < 10; j++)
		{
			gr = getgrgid(groups[j]);
			if(gr != NULL)
			{
				//printf("%s\n", gr->gr_name);
				if(strcmp(fileName, gr->gr_name) == 0 || strcmp(fileName, "intranet") == 0)
				{
					access = 0;
					break;
				}
				else
				{
					access = 1;
				}
			}
		}
		
		//if the user doesn't have access inform them	
		if (access == 1 && strcmp(file, "") != 0 )
		{
			printf("\nThe file %s cannot be opened by the server because the users does not have access\n\n", fileName);
			send(sock, "User does not have access to this folder", strlen("User does not have access to this folder"), 0);
			close(sock);
		}
		else if(access == 0 && strcmp(file, "") != 0)
		{
			char filePath[500];

			memset(filePath, 0, 500);
			READSIZE = recv(sock, filePath, 200, 0);

			printf("\nClient sent: %s\n", filePath);
			send(sock, "File name received\n", strlen("File name received\n"), 0);
			char path[500] = "/home/jennifer/Documents/Assignment2";
			strcat(path, filePath);

			char buffer[512];

			strcpy(fileName, path);
			printf("\nFile to be transferred to: %s\n", fileName);

			char option[2];
			READSIZE = recv(sock, option, 50, 0);

			if(strcmp(option, "1") == 0)
			{

				//lock the file for synchronisation purposes
				pthread_mutex_lock(&lockFile);

				FILE *openFile = fopen(fileName, "w");

				if(openFile == NULL)
				{
					printf("\nThe file %s cannot be opened by the server\n\n", fileName);
					close(sock);
				} else  {
					bzero(buffer, 512);
					int block = 0;
					//transfer the contents of the file
					while((block = recv(sock, buffer, 512, 0)) > 0)
					{
						printf("\nReceived data\n");
						int writeFile = fwrite(buffer, sizeof(char), block, openFile);
						bzero(buffer, 512);
						if(writeFile == 0 || writeFile != 512)
						{
							break;
						}
					}
					printf("\nTransfer complete!\n\n");
					fclose(openFile);
					//unlock the file
					pthread_mutex_unlock(&lockFile);
					printf("\n");
					send(sock, "Transfer executed successfully", strlen("Transfer executed successfully"), 0);

					//log the user, date and file details
					char logFile[500] = "/home/jennifer/Documents/Assignment2/intranet/log.txt";
					char log[500];
					char date[30];
					time_t now = time(0);
					struct tm *timeDate;
					timeDate = gmtime(&now);	

					strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", timeDate);
					strcat(log, "User: ");
					strcat(log, uname);
					strcat(log, "\nFile changed: ");
					strcat(log, fileName);
					strcat(log, "\nDate and Time: ");
					strcat(log, date);
					strcat(log, "\n----------------------------------------\n");

					FILE *logfile = fopen(logFile, "a");
					if(logfile != NULL)
					{
						fputs(log, logfile);
					}
					else
					{
						printf("%s\n", logFile);
					}	
					fclose(logfile);	
					
					close(sock);
				}
			} else if(strcmp(option, "2") == 0)
			{
				printf("\nTransfer cancelled\n\n");
			}
		} else {
			printf("\nClient exited\n\n");
		}
	}
	else if(found == 0) //if the user is not authenticated inform the user
	{
		send(sock, "Authentication failed", strlen("Authentication failed"), 0);
		printf("\nUser authentication failed\n\n");
		close(sock);
	}
	
}
