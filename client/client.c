/*
 * compiled as gcc client.c -Wall -o client.out
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/stat.h>
#include <ctype.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 4096

/* print error function prototype */
void error(char *msg);

/* connect user command function prototype */
void connect_command(void);

/* list user command function prototype */
void list_command(void);

/* retrieve user command function prototype */
void retrieve_command(void);

/* store user command function prototype */
void store_command(void);

/* quit user command function prototype */
void quit_command(void);

pid_t pid;                // process id
int socket_file_desc;   // socket file descriptor
int port_number;        // port number
int num_bytes;          // number of bytes read/written
int connected = 0;      // if connection to server has been made

struct sockaddr_in server_address;              // server address
char server_address_buffer[INET_ADDRSTRLEN];    // string representation of server IP address
struct hostent* server;                         // host (server)
const char delim[2] = " ";                      // delimiter for user input
char* token;                                    // token for user input
char buffer[BUFFER_SIZE];                       // buffer space

int main(int argc, char *argv[]) {

    // infinite loop that waits for user input
    while(1) {

        // prompt user
        printf("Please enter a command: ");

        // clear buffer and put user input in buffer
        bzero(buffer, BUFFER_SIZE);
        fgets(buffer, BUFFER_SIZE - 1, stdin);

        // if the client has not yet connected to the server
        if (connected == 0) {

            // if user wants to connect to server
            if (strstr(buffer, "CONNECT") != NULL) {
                connect_command();
            } else {
                printf("Invalid Command\n");
            }
        }

        // if the client is already connected to the server
        else {

            // if user wants to list files
            if (strstr(buffer, "LIST") != NULL) {
                list_command();
            }

            // if user wants to retrieve a file
            else if (strstr(buffer, "RETRIEVE") != NULL) {
                retrieve_command();
            }

            // if user wants to store a file
            else if (strstr(buffer, "STORE") != NULL) {
                store_command();
            }

            // if user wants to quit
            else if (strstr(buffer, "QUIT") != NULL) {
                quit_command();
            }

            // an invalid command was entered
            else {
                printf("Invalid Command\n");
            }
        }
    }
}

/* print error function */
void error(char *msg) {
    perror(msg);
    exit(0);
}

/* connect user command function */
void connect_command(void) {

    // get user command
    token = strtok(buffer, delim);

    // get server (IP)
    token = strtok(NULL, delim);

    // if user provided server (IP)
    if (token != NULL) {

        // print server
        printf("Server/IP: %s \n", token);

        // initialize server connection
        server = gethostbyname(token);

        // get port number
        token = strtok(NULL, delim);

        // if user provided port number
        if (token != NULL) {
            printf("Port number: %s \n", token);
            // convert port number to int
            port_number = atoi(token);
        } else {
            printf("NOT ENOUGH ARGUMENTS FOR CONNECTION\n");
        }

        // create new socket
        socket_file_desc = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_file_desc < 0) {
            error("ERROR opening socket");
        }
        if (server == NULL) {
            error("ERROR, no such host");
            exit(0);
        }

        // connect to server
        bzero((char *) &server_address, sizeof(server_address));
        server_address.sin_family = AF_INET;
        bcopy((char *) server->h_addr, (char *) &server_address.sin_addr.s_addr, server->h_length);
        server_address.sin_port = htons(port_number);
        if (connect(socket_file_desc, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
            error("ERROR connecting");
        }

        // get string representation of server
        inet_ntop(AF_INET, &server_address.sin_addr, server_address_buffer, sizeof(server_address_buffer));

        printf("Connection established with %s\n", server_address_buffer);
        connected = 1;
    } else {
        printf("NOT ENOUGH ARGUMENTS FOR CONNECTION\n");
    }
}

/* list user command function */
void list_command(void) {

    // write the user command from the buffer to the socket
    num_bytes = write(socket_file_desc, buffer, strlen(buffer));

    // if no bytes were written, print error
    if (num_bytes < 0) {
        error("ERROR writing to socket");
    }

    // clear buffer
    bzero(buffer, BUFFER_SIZE);

    // read from socket into buffer
    num_bytes = read(socket_file_desc, buffer, BUFFER_SIZE - 1);

    // if no bytes were read, print error
    if (num_bytes < 0) {
        error("ERROR reading from socket");
    }

    // print listed files
    printf("%s\n", buffer);

    // clear buffer
    bzero(buffer, BUFFER_SIZE);
}

/* retrieve user command function */
void retrieve_command(void) {
	char *tok;
	char *fname;
	FILE *fp;
    	// write the user command from the buffer to the socket
    	num_bytes = write(socket_file_desc, buffer, BUFFER_SIZE - 1);

    	// if no bytes were written, print error
    	if (num_bytes < 0) {
        	error("ERROR writing to socket");
    	}
    	// tokenize to make sure enough arguments were given
    	tok = strtok(buffer, " ");
    	tok = strtok(NULL, " ");
    	if (token != NULL) {
		read(socket_file_desc, buffer, BUFFER_SIZE);
		printf("%s\n",buffer);
		strcpy(fname,buffer);
		fp = fopen(fname,"w");
		read(socket_file_desc, buffer, BUFFER_SIZE);
		printf("%s\n",buffer);
		fwrite(buffer,sizeof(char),strlen(buffer),fp);
		fclose(fp);
		printf("File %s created in local directory\n",fname);
    	}
    	else {
        	printf("NOT ENOUGH ARGUMENTS FOR FILE DOWNLOAD\n");
    	}
}

/* store user command function */
void store_command(void) {
	char full_fname[] = {"./\0"};
        char *fname;
        FILE *fp;
	// write the user command from the buffer to the socket
        num_bytes = write(socket_file_desc, buffer, BUFFER_SIZE - 1);

        // if no bytes were written, print error
        if (num_bytes < 0) {
		error("ERROR writing to socket");
        }
       	strtok(buffer," ");
        fname = strtok(NULL," ");
        fname[strlen(fname)-1] = '\0';
        printf("fname = %s\n",fname);
        strcpy(buffer,fname);
        write(socket_file_desc,buffer,BUFFER_SIZE);
        strcat(full_fname, fname);
        fp = fopen(full_fname, "r");
        size_t newLen = fread(buffer, sizeof(char), BUFFER_SIZE, fp);
        if ( ferror( fp ) != 0 ) {
		fputs("Error reading file", stderr);
       	}
       	else {
                buffer[newLen++] = '\0';
        }
        fclose(fp);
        if(0 > write(socket_file_desc,buffer,BUFFER_SIZE)) error("ERROR writing to socket");
        bzero(buffer,BUFFER_SIZE);
	printf("STORE Executed\n");
}

/* quit user command function */
void quit_command(void) {

    // write the user command from the buffer to the socket
    num_bytes = write(socket_file_desc, buffer, strlen(buffer));

    // if no bytes were written, print error
    if (num_bytes < 0) {
        error("ERROR writing to socket");
    }

    printf("Connection termtinated with %s\n", server_address_buffer);

    // clear buffer
    bzero(buffer, BUFFER_SIZE);
    // TODO: close gracefully?
    exit(0);
}

