/////////////////////////////////////////////////////////////////////////
//
// To compile: 			gcc -o my_httpd my_httpd.c -lnsl -lsocket
//
// To start your server:	./my_httpd 2000 .www			
//
// To Kill your server:		kill_my_httpd
//
/////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

#include <stdlib.h>
#include <sys/wait.h>
#include <netinet/in.h>

#include <signal.h>
#include <strings.h>

#define	ERROR_FILE	0
#define REG_FILE  	1
#define DIRECTORY 	2

#define QUELENGTH	5
#define NO_FILE 	-1
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
void GetMyHomeDir(char *myhome, char **environ) {
        int i=0;
	while( environ[i] != NULL ) {
        	if( ! strncmp(environ[i], "HOME=", 5) ){
			strcpy(myhome, &environ[i][5]);
			return;
		}
		i++;
        }
	fprintf(stderr, "[FATAL] SHOULD NEVER COME HERE\n");
	fflush(stderr);
	exit(-1);
}

////////////////////////////////////////////////////////////////////
// Tells us if the request is a directory or a regular file
////////////////////////////////////////////////////////////////////
int TypeOfFile(char *fullPathToFile) {
	struct stat buf;	/* to stat the file/dir */

        if( stat(fullPathToFile, &buf) != 0 ) {
		perror("stat()");
		fprintf(stderr, "[ERROR] stat() on file: |%s|\n", 
						fullPathToFile);
		fflush(stderr);
        	return -1;
	}


        if( S_ISREG(buf.st_mode) )
		return(REG_FILE);
        else if( S_ISDIR(buf.st_mode) ) 
		return(DIRECTORY);

	return(ERROR_FILE);
}

char * getErrorDocument(int status)
{
	char * Response;

	switch(status)
	{
		case 500:
			Response = "<html><head><title>Internal Server Error</title></head><body>An unknown error has occured</body></html>";
			break;
		case 404:
			Response = "<html><head><title>Page Not Found</title></head><body>Page was not found on the server</body></html>";
			break;
	}
	return Response;
}


#define HTML 0
#define JPEG 1
#define BITMAP 2
void buildHeader(char * Header, int status, int fileSize, int fileType)
{
	
	char * s;
	char * t;
	bzero(Header, sizeof(Header));	
	switch(status)
	{
		case 200:
			s = "OK";
			break;
		case 500:
			s = "Internal Server Error";
			break;
		case 404:
			s = "Not Found";
	}
	switch(fileType)
	{
		case HTML:
			t = "text/html";
			break;
		case JPEG:
			t = "image/jpeg";
		case BITMAP:
			t = "image/bmp";
			
	}
	
	sprintf(Header, "HTTP/1.0 %d %s\nContent-length: %d\nContent-type: %s\n\n", status, s, fileSize, t);
}

int executeCommandLine(char * command, int sock)
{
	/*
	execute the command on the command line and get the output
	*/	
	FILE *fp;
	char path[1035];
  	/* Open the command for reading. */
	fp = popen(command, "r");
	if (fp == NULL) 
	{
		printf("Failed to run command\n" );
		return -1;
	}
	/* Read the output a line at a time - output it. */
	while (fgets(path, sizeof(path)-1, fp) != NULL) 
	{
		write(sock, path, strlen(path));	
  	}
}

void SendResponse(int status, int file, char * fileExt, char * fullPathToFile, int sock)
{
	char * content;
	int fileSize;
	int readBytes;
	int bufferLength = 1000;
	char buffer[bufferLength];
	char Header[1024];
	int fileType;

	//We have an error document
	if(file == NO_FILE)
	{
		content = getErrorDocument(status);
		fileSize = strlen(content);
		fileType = HTML;
	}
	//we are reading from the file to get the contents
	else
	{	
		fileSize = lseek(file,(off_t) 0, SEEK_END);
		lseek(file, (off_t)0, SEEK_SET);


		/*
		Get the file type of the document
		*/
		if(strcmp(fileExt, "jpg") == 0 || strcmp(fileExt, "jpeg") == 0 || strcmp(fileExt, "jpe") == 0)
		{
			fileType = JPEG;
		}
		else if(strcmp(fileExt, "html") == 0 || strcmp(fileExt, "htm") == 0)
		{
			fileType = HTML;
		}
		else if(strcmp(fileExt, "bmp") == 0)
		{
			fileType = BITMAP;
		}
		else if(strcmp(fileExt, "php") == 0 || strcmp(fileExt, "cgi") == 0)
		{
			fileType = HTML;
		}
		else
		{
			/*
			If we had an error recall the SendResponse function with the proper error code
			*/
			SendResponse(500, NO_FILE, NULL, NULL, sock);
			return;
		}
	}	

	buildHeader(Header, status, fileSize, fileType);

	write(sock, Header, strlen(Header));
	if(file == NO_FILE)
	{
		write(sock, content, strlen(content));
	}
	else
	{
		if(strcmp(fileExt, "php") == 0)
		{
			char command[128];
			sprintf(command, "php %s", fullPathToFile);
			if(executeCommandLine(command, sock) < 0)
			{
				SendResponse(500, NO_FILE, NULL, NULL, sock);
			}
		}
		else if(strcmp(fileExt, "cgi") == 0)
		{
			if(executeCommandLine(fullPathToFile, sock) < 0)
			{
				SendResponse(500, NO_FILE, NULL, NULL, sock);
			}
		}
		else
		{
			/*
			read the file into the content string
			*/
			while((readBytes = read(file, buffer, bufferLength)) > 0)//read the file
			{
				write(sock, buffer, readBytes);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
void SendDataBin(char *fileToSend, int sock, char *home, char *content) {
	char fullPathToFile[256];
	int fret;		/* return value of TypeOfFile() */
	/*
	 * Build the full path to the file
	 */
	sprintf(fullPathToFile, "%s/%s%s", home, content, fileToSend);


	fret = TypeOfFile(fullPathToFile);
	
	if(fret == DIRECTORY)
	{
		if(strcmp(fileToSend, "/cgi-bin") == 0)
		{
			sprintf(fullPathToFile, "%s/%s", fullPathToFile, "index.cgi");
		}
		else
		{
			sprintf(fullPathToFile, "%s/%s", fullPathToFile,"index.html");
		}
	}
	else if(fret == ERROR_FILE || fret == -1)
	{
		fprintf(stderr, "ERROR Getting File");
		fflush(stderr);
		SendResponse(404, NO_FILE, NULL, NULL, sock);
		return;
	}

	int file;
	int i;
	char fileExt[4];
	char period[] = ".";
	
	i = strcspn(fullPathToFile, period);
	strcpy(fileExt, &fullPathToFile[i+1]);
	
	if((file = open(fullPathToFile, O_RDONLY)) < 0)
	{
		fprintf(stderr, "FILE: %s", fullPathToFile);
		perror("COULD NOT OPEN FILE");
		SendResponse(404, NO_FILE, NULL, NULL, sock);
		exit(-4);
	}
	SendResponse(200, file, fileExt, fullPathToFile, sock);
	close(file);
}


////////////////////////////////////////////////////////////////////
// Extract the file request from the request lines the client sent 
// to us.  Make sure you NULL terminate your result.
////////////////////////////////////////////////////////////////////
void ExtractFileRequest(char *req, char *buff) {
	char space[] = " ";
	char buf2[1024];
	bzero(buf2, sizeof(buf2));
	strcpy(buf2, &buff[4]);
	int index;
	index = strcspn(buf2, space);
	bzero(req, sizeof(req));
	strncpy(req, buf2, index);
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
int main(int argc, char **argv, char **environ) {
  	pid_t pid;		/* pid of child */
	int sockid;		/* our initial socket */
	int PORT;		/* Port number, used by 'bind' */
	char content[128];	/* Your directory that contains your web 
				   content such as .www in 
				   your home directory */
	char myhome[128];	/* Your home directory */
				/* (gets filled in by GetMyHomeDir() */
	/* 
	 * structs used for bind, accept..
	 */
  	struct sockaddr_in server_addr, client_addr; 

	char file_request[256];	/* where we store the requested file name */
        int one=1;		/* used to set socket options */


	/* 
	 * Get my home directory from the environment 
	 */
	GetMyHomeDir(myhome, environ);	

	if( argc != 3 ) {
		fprintf(stderr, "USAGE: %s <port number> <content directory>\n", 
								argv[0]);
		exit(-1);
	}

	PORT = atoi(argv[1]);		/* Get the port number */
	strcpy(content, argv[2]);	/* Get the content directory */


	if ( (pid = fork()) < 0) {
		perror("Cannot fork (for deamon)");
		exit(0);
  	}
	else if (pid != 0) {
		/*
	  	 * I am the parent
		 */
		char t[128];
		sprintf(t, "echo %d > %s.pid\n", pid, argv[0]);
		system(t);
    		exit(0);
  	}

  	// setsid();
  	// chdir("/");
  	// umask(0);

	/*
	 * Create our socket, bind it, listen
	 */

	/* TODO 1 */

	signal(SIGCHLD, SIG_IGN);

	if((sockid = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("SOCKET: ");
		exit(-1);
	}

	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORT);

	if(bind(sockid, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0)
	{
		perror("BIND: ");
		exit(-2);
	}

	if(listen(sockid, QUELENGTH) < 0)
	{
		perror("LISTEN");
		exit(-3);
	}
	/* 
	 * - accept a new connection and fork.
	 * - If you are the child process,  process the request and exit.
	 * - If you are the parent close the socket and come back to 
         *   accept another connection
	 */
  	while (1) {
		/* 
		 * socket that will be used for communication
		 * between the client and this server (the child) 
		 */
		int newsock;		

		/*
		 * Get the size of this structure, could pass NULL if we
		 * don't care about who the client is.
		 */
   		int client_len = sizeof(client_addr);

		/*
		 * Accept a connection from a client (a web browser)
		 * accept the new connection. newsock will be used for the 
		 * child to communicate to the client (browser)
		 */
		 /* TODO 2 */

		while((newsock = accept(sockid, (struct sockaddr *) &client_addr, &client_len)) >= 0)
		{
	
    			if (newsock < 0) {
				perror("accept");
				exit(-1);
			}
	
			if ( (pid = fork()) < 0) {
				perror("Cannot fork");
				exit(0);
	  		}
	///////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////
			else if( pid == 0 ) {
				/*
				 * I am the Child
				 */
				int r;
	      			char buff[1024];
				int read_so_far = 0;
				char ref[1024], rowRef[1024];
	
				close(sockid);
	
				memset(buff, 0, 1024);
	
				/*
				 * Read client request into buff
				 * 'use a while loop'
				 */
				/* TODO 3 */

				fcntl(newsock, F_SETFL, O_NONBLOCK);	
				while((r = read(newsock, buff, 1024)) > 0)
				{
					read_so_far = read_so_far + r;
					printf("%s", buff);
					fflush(stdout);
				}
				char method[3];
				strncpy(method, buff, 3);
				int cmp;
				if(strcmp(method, "GET") == 0)
				{
					printf("GET Request for ");
					fflush(stdout);
					//this is a GET request
					bzero(file_request, sizeof(file_request));
					ExtractFileRequest(file_request, buff);
					
					printf(" |%s|\n", file_request);
					fflush(stdout);
				
					SendDataBin(file_request, newsock, myhome, content);
				}
				else if(strcmp(method, "POS") == 0)
				{
					//this is a POST request
					printf("Post request");
					fflush(stdout);
				}
				else
				{
					//this is an unsupported method
					printf("Unsupported method '%s'", method);
				}
			
				shutdown(newsock, 1);
				close(newsock);
				exit(0);
	    		}
			/*
			 * I am the Parent
			 */
			close(newsock);	/* Parent handed off this connection to its child,
				           doesn't care about this socket */
	  	}
	}
}
