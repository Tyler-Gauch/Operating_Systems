/////////////////////////////////////////////
//                                         //
//          Tyler Gauch                    //
//          Assignment 1                   //
//          June 2, 2015                   //
//                                         //
/////////////////////////////////////////////


#include <fcntl.h>
#include <stdio.h>


int main(int argc, char *argv[])
{
    int infile, outfile;
    char buffer[1];
    char t_type='0';
    int filesize;
    int rr, i;
    off_t x;

    if(argc < 3)
    {
        fprintf(stderr, "ERROR: Incorrect Usage\nUSAGE: %s <inputfile> <outputfile> <traversetype>\n", argv[0]);
        return -1;
    }

    if(argc == 4)
    {
        switch(argv[3][0])
        {
            case '0':
            case '1':
            case '2':
                t_type = argv[3][0];
                break;
            default:
                fprintf(stderr, "ERROR: Unknown traversetype [%s]\nTRAVERSETYPES: 0-SEEK_END, 1-SEEK_SET, 2-SEEK_CUR\nDEFAULTING TO SEEK_END.\n\n", argv[3]);
                t_type = 0;
        }
    }

    if((infile = open(argv[1], O_RDONLY)) < 0)
    {
        fprintf(stderr, "ERROR: Unable to open inputfile: '%s'\n", argv[1]);
        return -2;
    }

    if((outfile = open(argv[2], O_WRONLY|O_CREAT|O_TRUNC, 0644))  < 0)
    {
        close(infile);
        fprintf(stderr, "ERROR: Unable to open outputfile: '%s'\n", argv[2]);
        return -3;
    }

    filesize = lseek(infile, (off_t)0, SEEK_END);
    
    for(i = filesize-1; i > 0; i--)
    {
         switch(t_type)
         {
             case '0':
                 lseek(infile, (off_t)i-filesize-1, SEEK_END);
                 break;
             case '1':
                 lseek(infile, (off_t)i-1, SEEK_SET);
                 break;
             case '2':
                 if(i == filesize-1)
                 {
                     lseek(infile, (off_t)-1, SEEK_CUR);
                 }
                 else
                 {
                     lseek(infile, (off_t)-2, SEEK_CUR);
                 }
                 break;
             default:
                 fprintf(stderr, "ERROR: No Traverse Type");
         }

         rr = read(infile, buffer, 1);	/* read one byte */
         
         if(rr == 0)
         {
            printf("File was reversed");
            break;
         }
         else if( rr != 1 )
         {
             fprintf(stderr, "ERROR: Couldn't read 1 byte [%d]\n", rr);
             return -4;
         }

         rr = write(outfile, buffer, 1); /* write the byte to the file*/

         if( rr != 1 )
         {
             fprintf(stderr, "ERROR: Couldn't write 1 byte [%d]\n", rr); 
            return -5;
         }
    }
    close(infile);
    close(outfile);
    return 0;
}
