#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

/* opt_enc.c
Client function which communicates with otp_dec_d
Base networking code comes from client.c which was supplied in class.
*/

int main(int argc, char *argv[])
{
	// Step 1 - Initalize Client Variables.	
	int socketFD, portNumber, charsWritten, charsRead; 
	struct sockaddr_in serverAddress; // Server Connection Structure
	struct hostent* serverHostInfo;
	char buffer[150000]; // Large enough to hold 70k key and 70k plaintext
	char* decryptedText; // pointer needed for strtok to parse decrypted text after receiving response from otp_enc_d
	
    
	if (argc < 4) { fprintf(stderr,"USAGE: %s plaintext key port\n", argv[0]); exit(0); } // Check usage & args

	// Step 2 - Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(2); } // Cant connect, set exit to 2.
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address


	//Step 3 - Open plaintext and key for reading.
	FILE *plainText=fopen(argv[1],"r");
	FILE *key=fopen(argv[2],"r");
	
	if(plainText == NULL)
		fprintf(stderr,"error opening plainText\n");
	if(key == NULL)
		fprintf(stderr,"error opening key\n");

	//Step 4 - Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0){fprintf(stderr,"CLIENT: ERROR opening socket");}
	
	//Step 5 - Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
		{fprintf(stderr,"CLIENT: ERROR connecting"); exit(2);} // Cant connect, set exit to 2

	//Step 6 - Send identifier to server
	charsWritten = send(socketFD, "decryption", 10, 0); // Write to the server
	if (charsWritten < 0){fprintf(stderr,"CLIENT: ERROR writing to socket");}
	if (charsWritten < strlen(buffer)){fprintf(stderr,"CLIENT: WARNING: Not all data written to socket!\n");}
	
	//Step 7 - Get identifier from server
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); // Read data from the socket, leaving \0 at end
	if(strcmp(buffer,"encryption")==0) // If the encryption identifier is returned, this is an invalid attempt to connect to otp_enc_d
	{
	close(socketFD); // Close the socket
	{fprintf(stderr,"Invalid connection attempt - otp_dec cannot use otp_enc_d");}
	exit(2); // Invalid connection, error 2.
	}


	//Step 8 - Get Plaintext
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer array
	fgets(buffer, sizeof(buffer) - 1, plainText); // Get input from the user, trunc to buffer - 1 chars, leaving \0
	buffer[strcspn(buffer, "\n")] = '\0'; // Remove the trailing \n that fgets adds

	int y;
	for(y=0;y<15;y++)
	{
	if(buffer[y]<65 || buffer[y]>90)
	{
	if(buffer[y]!=32){fprintf(stderr,"Invalid Characters in plaintext file\n"); return 1;}
	}
	}

	//Step 9 - Send plaintext to server
	charsWritten = send(socketFD, buffer, strlen(buffer), 0); // Write to the server
	if (charsWritten < 0){fprintf(stderr,"CLIENT: ERROR writing to socket");}
	if (charsWritten < strlen(buffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");

	//Delimiter character separates plaintext from key for server
	char delimiter[1];
	memset(delimiter,'@',1);
	charsWritten = send(socketFD, delimiter, 1, 0); // Write a delimeter

	int keyLength=0;
	int textLength=0;
	textLength=strlen(buffer);



	//Step 10 - Get Key
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer array
	fgets(buffer, sizeof(buffer) - 1, key); // Get input from the user, trunc to buffer - 1 chars, leaving \0
	buffer[strcspn(buffer, "\n")] = '\0'; // Remove the trailing \n that fgets adds

	keyLength=strlen(buffer);
	if(keyLength<textLength)
	{fprintf(stderr,"Key is too short\n");exit(1);}

	//Step 11 - Send key to server
	charsWritten = send(socketFD, buffer, strlen(buffer), 0); // Write to the server
	if (charsWritten < 0){fprintf(stderr,"CLIENT: ERROR writing to socket");}
	if (charsWritten < strlen(buffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");
	

	// Step 12 - Send another delimiter followed by enough text (up to 150,000 characters) to close the MSG_WAITALL flag on the other end
	charsWritten = send(socketFD, delimiter, 1, 0); // Write a delimeter		
	memset(buffer, '1', sizeof(buffer)); // Clear out the buffer array
	charsWritten = send(socketFD, buffer, strlen(buffer), 0); // Write to the server

	//Step 13 - Get return message from server
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	charsRead = recv(socketFD, buffer, 100000, MSG_WAITALL); // Read data from the socket, leaving \0 at end
	if (charsRead < 0){fprintf(stderr,"CLIENT: ERROR 3 reading from socket");}
	
	//Step 14 - Parse Response and send decryptedText to stdout.
	decryptedText = strtok(buffer,"@");
	printf("%s\n", decryptedText);

	//Step 15 - Close Socket, Exit with 0
	close(socketFD); // Close the socket
	return 0;	
}
