#include <stdio.h>
#include <string.h> //for strlen
#include <sys/socket.h>
#include <arpa/inet.h> //for inet_addr
#include <unistd.h> //for write
#include <stdlib.h>

int main(int argc, char *argv[])
{
	int SID;
	struct sockaddr_in server;
	char clientMessage[500];
	char serverMessage[500];
	char menuOption[2];
	char fileName[200];
	char fileDirectory[500];
	char *destPath;
	char *finalDestPath;
	char destBuffer[500];

	//create a socket
	SID = socket(AF_INET, SOCK_STREAM, 0);
	if(SID == -1)
	{
		printf("Error creating socket\n");
	} else {
		//printf("Socket created\n");
	}

	//set sockaddr_in variables
	server.sin_port = htons(8081); // port to connect to
	server.sin_addr.s_addr = inet_addr("127.0.0.1"); // server IP
	server.sin_family = AF_INET; //IPV4 protocol

	//connect to server
	if(connect(SID, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		printf("Connect failed. Error\n");
		return 1;
	}

	printf("Connected to server ok!\n");

	char username[500];
	char password[500];

	//Have the user enter their username and password for authentication puproses
	printf("Login\nEnter username: ");
	scanf("%s", username);
	printf("Enter password: ");
	scanf("%s", password);

	strcat(username, " ");
	strcat(username, password);
	strcat(username, "\n");

	//send some login data to the server to authenticate
	if(send(SID, username, strlen(username), 0) < 0)
	{
		printf("Send failed\n");
		return 1;
	}

	if(recv(SID, serverMessage, 500, 0) < 0)
	{
		printf("IO Error\n");
		return 1;
	}	
	printf("\nServer sent following message: ");
	printf(serverMessage);	
	printf("\n");	

	//if the user was authenticated continue on with the file transfer process
	if(strcmp("User authenticated", serverMessage) == 0)
	{
		printf("\nMenu\n1: Transfer file\n2: Exit\nEnter choice: ");
		scanf("%s", menuOption); 

		//if the user choses to transfer a file
		if(strcmp(menuOption, "1") == 0)
		{
			//get the file the user would like to transfer
			printf("\nBeginning transfer\n");
			printf("\nEnter the name of the file to transfer: ");
			scanf("%s", fileName);
			strcpy(fileDirectory, "/home/jennifer/Documents/Assignment2/");
			strcat(fileDirectory, fileName);

			printf("\nLocal File Path: %s\n", fileDirectory);

			//have the user chose which directory they would like to transfer the file to
			printf("\nChoose a destination path\n1: Root(intranet)\n2: Sales\n3: Promotions\n4: Offers\n5: Marketing\nEnter choice: ");
			scanf("%s", menuOption);

			if(strcmp(menuOption, "1") == 0)
			{
				strcpy(destPath, "/intranet/");
			} else if(strcmp(menuOption, "2") == 0)
			{
				strcpy(destPath, "/intranet/sales/");
			} else if(strcmp(menuOption, "3") == 0)
			{
				strcpy(destPath, "/intranet/promotions/");
			} else if(strcmp(menuOption, "4") == 0)
			{
				strcpy(destPath, "/intranet/offers/");
			} else if(strcmp(menuOption, "5") == 0)
			{
				strcpy(destPath, "/intranet/marketing/");
			} else
			{
				printf("Invalid entry\n");
				return 1;
			}

			//send menu option selected
			if(send(SID, menuOption, strlen(menuOption), 0) < 0)
			{
				printf("Send failed\n");
				return 1;
			}	
			
			//get the destination file path the file will be transfered to
			finalDestPath = strcat(destPath, fileName);
			strcpy(destBuffer, finalDestPath);

		} else if(strcmp(menuOption, "2") == 0) //if the user choses to exit the program
		{
			printf("Exiting Program\n");
			exit(0);
		} else
		{
			printf("Invalid entry\n");
			return 1;
		}

		//communicate with server
		while(1)
		{
			//send the file path to transfer
			if(send(SID, destBuffer, strlen(destBuffer), 0) < 0)
			{
				printf("Send failed\n");
				return 1;
			}

			if(recv(SID, serverMessage, 500, 0) < 0)
			{
				printf("IO Error\n");
				break;
			}	

			printf("\nServer sent following message: ");
			printf(serverMessage);
			
			//if the user does not have the permissions to transfer to the selected directory inform the user on the client end
			if(strstr(serverMessage, "User does not have access to this folder"))
			{
				printf("\nExiting program\n");
				exit(0);
			}
			else
			{
				printf("\nWould you like to continue with the transfer?\n1: Yes\n2: No\nEnter your choice: ");
				scanf("%s", menuOption);

				if(send(SID, menuOption, strlen(menuOption), 0) < 0)
				{
					printf("Send failed\n");
					return 1;
				}

				//continue with the file transfer
				if(strcmp(menuOption, "1") == 0)
				{	
					char buffer[512];
					char *file_name = fileDirectory;
					
					printf("\nSending the file %s to the server\n", fileName);

					//open the file to send its contents to the server to transfer
					FILE *openFile = fopen(file_name, "r");
					bzero(buffer, 512);
					int block = 0;

					//send the file contents to the server for transmission
					while((block = fread(buffer, sizeof(char), 512, openFile)) > 0)
					{
						printf("\nSending data\n");
						if(send(SID, buffer, block, 0) < 0)
						{
							exit(1);
						}
						bzero(buffer, 512);
					}

					if(recv(SID, serverMessage, 500, 0) < 0)
					{
						printf("IO Error\n");
						break;
					}	
					printf("\nServer sent following message: ");
					printf(serverMessage);	
					printf("\n");					
				} else if(strcmp(menuOption, "2") == 0)
				{
					printf("Exiting Program\n");
					exit(0);
				}else
				{
					printf("Invalid entry\n");
					return 1;
				}
				
				break;
			}
		}
	} else if(strcmp("Authentication failed", serverMessage) == 0) //if the incorrect user details were provided inform the user
	{
		printf("\nIncorrect login details\n");
		printf("Closing program\n\n");
		exit(0);
	}

	close(SID);
	return 0;
}
