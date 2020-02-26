/*
 * the port number is passed as an argument
 * compiled as gcc server.c -Wall -lsocket -o server.out
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>

#define READ 0
#define WRITE 1
#define SIZE 4096

/* handler for async SIGINT function prototype */
void signal_handler_sigint(int);

/* the server workers' function prototype */
void do_stuff(int socket, struct sockaddr_in client_address);

/* print error function prototype */
void error(char* msg);


struct sigaction sa_sigint;     // define action to be taken on SIGINT

char* input;    // TODO

int socket_file_desc;       // socket file descriptor
int new_socket_file_desc;   // new socket file descriptor
int port_number;            // port_number
int client_size;            // size of client address
int pid;                    // process id

struct sockaddr_in server_address;              // server address
struct sockaddr_in client_address;              // client address
char client_address_buffer[INET_ADDRSTRLEN];    // string representation of client IP address

int main(int argc, char *argv[]) {

    // setup SIGINT to gracefully exit
    sa_sigint.sa_handler = signal_handler_sigint;
    sigemptyset(&sa_sigint.sa_mask);
    sa_sigint.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sa_sigint, NULL) == -1) {
        perror("sigaction failure: ");
        exit(1);
    }

    // if port number wasn't provided, print error
     if (argc < 2) {
         error("ERROR, no port provided");
     }

     //
     socket_file_desc = socket(AF_INET, SOCK_STREAM, 0);
     if (socket_file_desc < 0) {
         error("ERROR opening socket");
     }
     bzero((char *) &server_address, sizeof(server_address));
     port_number = atoi(argv[1]);
     server_address.sin_family = AF_INET;
     server_address.sin_addr.s_addr = INADDR_ANY;
     server_address.sin_port = htons(port_number);
     if (bind(socket_file_desc, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
         error("ERROR on binding");
     }
     listen(socket_file_desc, 5);
     client_size = sizeof(client_address);
     printf("Server started\n");

     while (1) {
         new_socket_file_desc = accept(socket_file_desc, (struct sockaddr*) &client_address, &client_size);
         if (new_socket_file_desc < 0)
             error("ERROR on accept");
         pid = fork();
         if (pid < 0)
             error("ERROR on fork");
         if (pid == 0)  {
             close(socket_file_desc);
             do_stuff(new_socket_file_desc, client_address);
             exit(0);
         }
         else close(new_socket_file_desc);
     }
     return 0; // we never get here
}

/******** do_stuff() *********************
 There is a separate instance of this function
 for each connection.  It handles all communication
 once a connection has been established.
 *****************************************/
void do_stuff(int socket, struct sockaddr_in client_address) {
        int p1[2];
        pid_t pid;
        ssize_t lsSize;
        //char* lsArgs[] = {"ls", "./files", NULL};
	char* lsArgs[] = {"ls",NULL};
        struct sockaddr_in server_address;
        struct hostent *server;

        int n, port_number, socket_file_desc;
        char buffer[SIZE];
        bzero(buffer, SIZE);
        input = malloc(SIZE);
        const char delim[2] = " ";
        char *token;

        // get string representation of client
        inet_ntop(AF_INET, &client_address.sin_addr, client_address_buffer, sizeof(client_address_buffer));

        printf("Connection established with %s\n", client_address_buffer);

        while(1) {
                bzero(buffer, SIZE);
                n = read(socket, buffer, SIZE - 1);
                if (n < 0) {
                    error("ERROR reading from socket");
                }
                strcpy(input, buffer);
                input[strlen(input)-1] = '\0';

                if (strstr(input, "LIST") != NULL) {
                        if (pipe(p1) < 0) {
                            perror ("Pipe Problem");
                        }
                        if ((pid = fork()) < 0) {
                            perror ("Fork Failed");
                        }
                        else if (!pid) {
                                dup2(p1[WRITE], STDOUT_FILENO);
                                close(p1[READ]);
                                close(p1[WRITE]);
                                sleep(0.2);
                                execvp(lsArgs[0], lsArgs);
                        }
                        dup2(p1[READ], STDIN_FILENO);
                        close(p1[READ]);
                        close(p1[WRITE]);
                        sleep(0.4);
                        lsSize = read(STDIN_FILENO, buffer, SIZE);
                        n = write(socket, buffer, lsSize);
                        if (n < 0) {
                            error("ERROR writing to socket");
                        }
                        printf("LIST executed\n");
                }
                else if(strstr(input, "RETRIEVE") != NULL){
                    char full_fname[] = {"./files/\0"};
		    char *fname;
		    FILE *fp;
		    	strtok(buffer," ");		    	
			fname = strtok(NULL," ");
		    	fname[strlen(fname)-1] = '\0';
		    	printf("fname = %s\n",fname);
			strcpy(buffer,fname);
			write(socket,buffer,SIZE);
                    	strcat(full_fname, fname);
			printf("fname = %s\n",full_fname);
                    	fp = fopen(full_fname, "r");
			size_t newLen = fread(buffer, sizeof(char), SIZE, fp);
    			if ( ferror( fp ) != 0 ) {
        			fputs("Error reading file", stderr);
   			}
			else {
        			buffer[newLen++] = '\0';
    			}
			fclose(fp);
			if(0 > write(socket,buffer,newLen)) error("ERROR writing to socket");
			printf("RETRIEVE Executed\n");
                }

                else if(strstr(input,"STORE") != NULL){
			char *tok;
        		char *fname;
        		FILE *fp;
                	read(socket, buffer, SIZE);
                	printf("%s\n",buffer);
                	fp = fopen(buffer,"w");
                	read(socket, buffer, SIZE);
                	printf("%s\n",buffer);
                	fwrite(buffer,sizeof(char),strlen(buffer),fp);
                	fclose(fp);
			bzero(buffer, SIZE);
                	printf("File %s created in local directory\n",fname);
                        printf("The file was received successfully.");
                        printf("STORE Executed\n");
                }

                else if (strstr(input, "QUIT") != NULL) {
                    printf("Connection terminated with %s\n", client_address_buffer);
                    free(input);
                    return;
                } else {
                        printf("Invalid command: %s\n", input);
                        n = write(socket,"Invalid Command\n",16);
                        if (n < 0) {
                            error("ERROR writing to socket");
                        }
                }
        }
        free(input);
}

/**********************************************************************
 * This function handles asynchronous SIGINT signals by reporting
 * the summary data about the receipt of the signal.
 *
 * @param int sigNum represents the identifier for the signal received.
 *********************************************************************/
void signal_handler_sigint(int sigNum) {
    printf(" received \n\n");
    printf("Exiting...\n");
    free(input);
    exit(0);
}

/* print error function */
void error(char *msg) {
    perror(msg);
    exit(1);
}
