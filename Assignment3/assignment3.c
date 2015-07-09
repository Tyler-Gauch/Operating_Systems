////////////////////////////////////////////
//                                         //
//               Tyler Gauch               //
//               Assignment 2              //
//               June 16, 2015             //
//                                         //
/////////////////////////////////////////////


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int g_close = 0;

void my_close(int fd)
{
	if(g_close == 0)
	{
		close(fd);
	}
}

int main ( int argc, char *argv[]) {
        
	int pfd[2];
	int pfd2[2];
	int pid[2];
	
	if(argc < 2)
	{
		fprintf(stderr, "ERROR: incorrect usage\n\tUSAGE: %s <USER-ID> <NO-CLOSE>", argv[0]);
		exit(-8);
	}

	if(argc == 3 && argv[2][0] == '1')
	{
		g_close = 1;
	}
	
	
	if(pipe(pfd) < 0) //create first pipe
	{
		perror("ERROR: Pipe1 Failed");
		exit(-1);
	}

	if((pid[0] = fork()) < 0) //start next process to speak to first pipe
	{
		perror("ERROR: Fork1 Failed");
		exit(-2);
	}
	
	if(pid[0] == 0) //it is the child
	{
		my_close(pfd[1]);	//close the unused file descriptor
		dup2(pfd[0], 0); //replace stdin with the pipe file descriptor
		my_close(pfd[0]);
		//create the process for the second pipe
		if(pipe(pfd2) < 0)
		{
			perror("ERROR: Pipe2 Failed");
			exit(-5);
		}
		if((pid[1] = fork()) < 0)
		{
			perror("ERROR: Fork2 Failed");
			exit(-6);
		}

		if(pid[1] == 0) //if it is a child process
		{
			my_close(pfd2[1]); //close unused file descriptor
			dup2(pfd2[0], 0); //replace stdin with the pipe file descriptor
			my_close(pfd2[0]);
			execlp("wc", "wc", (char *) 0);
			exit(-7);
		}
		else
		{
			my_close(pfd2[0]); //close unused file descriptor
			dup2(pfd2[1], 1);
			my_close(pfd2[1]);
			execlp("grep", "grep", argv[1], (char *) 0); //this starts a new process and only
							//returns if there was an error
			perror("ERROR: GREP Falied");
			exit(-3);
		}
	}
	else //it is the parent
	{
		my_close(pfd[0]); //close the unused file descriptor
		dup2(pfd[1], 1);//replace stdout with the pipe file descriptor
		my_close(pfd[1]); // close the old descriptor
		execlp("ps", "ps", "-ef", (char *) 0); //this starts a new process and only
							//returns is there was an error
		perror("ERROR: LS Failed");
		exit(-4);
	}

	return 0;
}
