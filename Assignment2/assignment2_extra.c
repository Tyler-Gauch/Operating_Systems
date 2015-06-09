/////////////////////////////////////////////
//                                         //
//               Tyler Gauch               //
//               Assignment 2              //
//               June 16, 2015             //
//                                         //
/////////////////////////////////////////////


#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#define BUFSIZE 256

int main ( int argc, char *argv[]) {
	char buffer[BUFSIZE];
        int infile, outfile;

	if( argc < 3 ) {
		fprintf(stderr, "USAGE: %s inputFile outputFile.\n", argv[0]);
		return -1;
	}
        int i;
        //loop through all in files to copy
 	for(i = 1; i < argc-1; i++)
        {
            char outname[512];
            printf("OUTFILE: %s\n", outname);
            strcpy(outname, argv[argc-1]);
            printf("OUTFILE: %s\n", outname);
            //create the outfile name
	    strcat(outname, "/");
            strcat(outname, argv[i]);

            printf("Copying %s\n", outname);

            //open infile
            if((infile = open(argv[i], O_RDONLY)) < 0)
            {
                   fprintf(stderr, "Could not open inputfile: %s\n", argv[i]);
            }

            //open outfile
            if((outfile = open(outname, O_WRONLY|O_CREAT|O_TRUNC, 0644)) < 0)
            {
                   perror("Could not open ouput directory\n");
                   close(infile);
                   return -3;
            }

            //read from the infile until we have nothing left to read
            int readBytes;
            while((readBytes = read(infile, buffer, BUFSIZE)) > 0)
	    {
                readBytes = write(outfile, buffer, readBytes);
                if(readBytes <= 0)
                {
                    perror("Could not write to output file: ");
                    return -4;
                }
            }
            
            //close files when done
            close(infile);
            close(outfile);
            //memset(outname,0,strlen(outname));
        }

        return 0;
}
