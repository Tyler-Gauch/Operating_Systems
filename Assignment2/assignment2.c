/////////////////////////////////////////////
//                                         //
//               Tyler Gauch               //
//               Assignment 2              //
//               June 16, 2015             //
//                                         //
/////////////////////////////////////////////


#include <fcntl.h>
#include <stdio.h>


int main ( int argc, char *argv[]) {
	char buffer[256];
        int infile, outfile;

	if( argc !=3 ) {
		fprintf(stderr, "USAGE: %s inputFile outputFile.\n", argv[0]);
		return -1;
	}
        
        //open infile
        if((infile = open(argv[1], O_RDONLY)) < 0)
        {
               fprintf(stderr, "Could not open inputfile: %s\n", argv[1]);
               return -2;
        }

        //open outfile
        if((outfile = open(argv[2], O_WRONLY|O_CREAT|O_TRUNC, 0644)) < 0)
        {
               fprintf(stderr, "Could not open ouputfile: %s\n", argv[2]);
               close(infile);
               return -3;
        }

        //read from the infile until we have nothing left to read
        int readBytes;
        while((readBytes = read(infile, buffer, 256)) > 0)
	{
            readBytes = write(outfile, buffer, readBytes);
            if(readBytes <= 0)
            {
                fprintf(stderr, "Could not write to output file.\n");
                return -4;
            }
        }

        //close files when done
        close(infile);
        close(outfile);
        return 0;
}