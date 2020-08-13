#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>

/* opt_enc_d.c
Server function which communicates with otp_enc
Base networking code comes from server.c which was supplied in class.
*/

int main(int argc, char *argv[])
{
	// Step 1 - Set up Server Variables	
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead, x;
	socklen_t sizeOfClientInfo;
	char buffer[150000],encryptedText[100000],identifier[100];//key[100000],plainText[150000];
	char* key;
	char* plainText;
	struct sockaddr_in serverAddress, clientAddress;
	memset(buffer, '\0', 150000);
	memset(encryptedText, '\0', 100000);
	memset(encryptedText, '\0', 100);
	int lengthKey, lengthText;
	pid_t spawnpid=-10;

	if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args

	//Step 2 - Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	//Step 3 - Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0){fprintf(stderr,"ERROR opening socket");}

	//Step 4 - Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
		fprintf(stderr,"ERROR on binding");
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections


while(1)
{
	
	//Step 5 - Accept a connection, blocking if one is not available until one connects
	sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
	establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
	if (establishedConnectionFD < 0){fprintf(stderr,"ERROR on accept");}

	//printf("\nSERVER: Connected Client at port %d\n",ntohs(clientAddress.sin_port));

	spawnpid=-10;
	spawnpid=fork(); // Step 6 - Fork a Child Process


	if(spawnpid==-1) // Error
	{	fprintf(stderr,"Error with Fork");
		exit(1);
		break;}
	
	if(spawnpid==0) // Child
	{	
	
		//Step 7 - (Child) Get the client identifier
		charsRead = recv(establishedConnectionFD, identifier, 100, 0); // Read the client's message from the socket
		if (charsRead < 0){fprintf(stderr,"ERROR reading from socket");}

		//Step 8 - (Child) Send the server identifier
		charsRead = send(establishedConnectionFD, "encryption", 10, 0); // Write to the client
		if (charsRead < 0){fprintf(stderr,"ERROR writing to socket");}

		//Step 9 - (Child) Check client identifier
		/*if(strcmp(identifier,"decryption")==0) // Invalid connection attempt from otp_dec
		{	
		close(establishedConnectionFD); // Close the existing socket which is connected to the client
		close(listenSocketFD); // Close the listening socket
		//{fprintf(stderr,"Invalid connection attempt - otp_dec cannot use otp_enc_d");}
		}*/


		//Step 10 - (Child) Get the plaintext, key, and any filler text up to 150000 characters from client.
		charsRead = recv(establishedConnectionFD, buffer, 150000, MSG_WAITALL); // Read the client's message from the socket
		if (charsRead < 0){fprintf(stderr,"ERROR reading from socket");}
		//lengthText=strlen(plainText);

		
		//Step 11 - (Child) Parse plaintext and key from 150k character buffer
		plainText = strtok(buffer,"@");
		key=strtok(NULL,"@");
			
		//Step 12 - (Child) Confirm Key is long enough
		lengthText=strlen(plainText);
		lengthKey=strlen(key);
		if(lengthKey<lengthText){fprintf(stderr,"key is too short %d",lengthKey,lengthText);}

		//Step 13 - (Child) Encrypt Text using Key
		int i,keyHolder,textHolder,encryptedHolder;
		memset(encryptedText, '\0', 100000);
		for(i=0;i<lengthText;i++) // Account for case where key is longer than text
		{
		keyHolder=key[i];
		textHolder=plainText[i];
	
		//Handle space character
		if(keyHolder==32){keyHolder=64;}
		if(textHolder==32){textHolder=64;}

		//Calculate ASCII Value
		encryptedHolder=(((keyHolder+textHolder-128)%27)+16);
	
		//Correct ASCII Value
		if(encryptedHolder==16){encryptedText[i]=-16+'0';}
		else{encryptedText[i]=encryptedHolder + '0';}
		}
		encryptedText[lengthText]='\0';


		//Step 14 - (Child) Send Ecrypted Text to client plus filler up to 100000 characters to close out MSG_WAITALL flag
		int charsWritten=0;
		charsWritten = send(establishedConnectionFD, encryptedText, strlen(encryptedText), 0); 
		char delimiter[1];
		memset(delimiter,'@',1);
		charsWritten = send(establishedConnectionFD, delimiter, 1, 0); // Write a delimeter		
		memset(buffer, '1', 150000);
		charsWritten = send(establishedConnectionFD, buffer, 100000, 0); // Send filler text to client up to 100000 characters	
		if(charsWritten < 0){fprintf(stderr,"ERROR writing to socket");}


	} // End of Child Fork
	else // Step 15 - (Parent) Close connection and wait for child.
	{
	close(establishedConnectionFD); // Close the existing socket which is connected to the client
	establishedConnectionFD=0;
	wait(NULL);
	}

} // End of accept() loop
	// Step 16 - Close Server -- in reality this is closed by CTRL+C and otherwise runs indefiniately while(1)
	close(establishedConnectionFD); // Close the existing socket which is connected to the client
	close(listenSocketFD); // Close the listening socket
	return 0; 
}
