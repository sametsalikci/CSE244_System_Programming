/*----------------------------------------------------------------------------*/
/** 
 *  Course:System Programming
 *  File:   hw1.c
 *  @author Mehmet Mazhar SALIKCI
 *  @create 11.03.2015
 *  Number:111044050
 ---------------
 *  Introduction:
 *  This is a program that reads given number of lines from file and 
 *  prints them on screen.
 *
 */
/*----------------------------------------------------------------------------*/
//                                 INCLUDES
/*----------------------------------------------------------------------------*/
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <dirent.h>
# include <errno.h>
# include<time.h>
# include<sys/stat.h>
# include <sys/ioctl.h>
# include <fcntl.h>
# include <termios.h>
# include <unistd.h>

/*----------------------------------------------------------------------------*/
//                                 DEFINES
/*----------------------------------------------------------------------------*/
# define BUF_SIZE 1024
# define ERROR -1
# define TRUE 1

/*----------------------------------------------------------------------------*/
//                                 GLOBALS
/*----------------------------------------------------------------------------*/
static struct termios oldt;

/*----------------------------------------------------------------------------*/
//                                 FUNCTIONS
/*----------------------------------------------------------------------------*/
void restoreTerminalSettings(void);
void disableWaitingForKeys(void);
void myMoreORLess(const char *pfFilename,int iScreenSize);
int MyUsage(int argc);

/*----------------------------------------------------------------------------*/
//                                MAIN FUNCTION
/*----------------------------------------------------------------------------*/
int main(int argc, char const *argv[])
{
	
    if(MyUsage(argc) != ERROR){
		myMoreORLess(argv[1],atoi(argv[2]));
	}	
	
    return 0;
}
/*----------------------------------------------------------------------------*/
// Function Name  : "restoreTerminalSettings"
// Parameters     : no parameters
// Description    : make sure settings will be restored when program ends  
//
/*----------------------------------------------------------------------------*/
void restoreTerminalSettings(void)
{
    tcsetattr(0, TCSANOW, &oldt);  // Apply saved settings   
}
/*----------------------------------------------------------------------------*/
// Function Name  : "disableWaitingForKeys"
// Parameters     : no parameters
// Description    : disabling terminal setting for keys behaviors
//
/*----------------------------------------------------------------------------*/
void disableWaitingForKeys(void)
{
    struct termios newt;                     
    tcgetattr(0, &oldt);  // Save terminal settings           
    newt = oldt;          // Init new settings                
    newt.c_lflag &= ~(ICANON | ECHO);  // Change settings     
    tcsetattr(0, TCSANOW, &newt);      // Apply settings      
    atexit(restoreTerminalSettings);
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "myMoreORLess"
// Parameters     : <pfFilename> : reading file name
//                  <iScreenSize> : will be show row size
// Description    : reads rows and show on screen
//
/*----------------------------------------------------------------------------*/
void myMoreORLess(const char *pfFilename,int iScreenSize)
{
    /*Variables*/
    int i, j,iCount=0;
    FILE *pfFilePtr;
    char buf[BUF_SIZE] = {0x0};
    char bufe[4];

	/*checks whether file opens*/
    if(( pfFilePtr = fopen(pfFilename,"r")) == NULL){
         fprintf(stderr, "Failed to open file : %s\n", pfFilename);
         return ;
     } 
    /*change the terminal settings*/ 
    disableWaitingForKeys();

    while (1){            
        if (bufe[0] == 'Q' || bufe[0] == 'q'){            
            printf("\n\n\n\t\t\t\t---QUIT FROM FILE---\n\n");
			return;
		}
        /**/
        if(bufe[0] == 27 && bufe[1] != '[' && bufe[2] != 'A' && bufe[3] != 'B') {//esc
            
            for(j = 0; j < iScreenSize; j++)
            {
               if ( !feof (pfFilePtr) && !ferror (pfFilePtr) 
                && fgets (buf, sizeof (buf), pfFilePtr) != NULL )
                    fputs (buf, stderr);
                else break;
           ++iCount;
            }
        }
        else if (bufe[0] == 12 || bufe[0] == '\n' || bufe[0] == '\r')
        {
            if ( !feof (pfFilePtr) && !ferror (pfFilePtr) 
                && fgets (buf, sizeof (buf), pfFilePtr) != NULL )
                fputs (buf,stderr);
                else break;
           ++iCount;
        }else if (bufe[1] == '[' && bufe[2] == 'A') {//Enter Up   
            
            rewind(pfFilePtr);
            --iCount;
            system("clear");
            for ( i = 0; i < iCount; ++i)
            {   
               if ( !feof (pfFilePtr) && !ferror (pfFilePtr) 
                    && fgets (buf, sizeof (buf), pfFilePtr) != NULL )
                    fputs (buf, stderr);
                else break;     
            }
        }
        else if (bufe[1] == '[' && bufe[2] == 'B'){//Enter Down
           if ( !feof (pfFilePtr) && !ferror (pfFilePtr) 
                && fgets (buf, sizeof (buf), pfFilePtr) != NULL )
                fputs (buf, stderr);
                else break;
           ++iCount;
        }
        if ( feof(pfFilePtr) || ferror(pfFilePtr) )
            break;
		strcpy(bufe,"   ");
		read(STDIN_FILENO, bufe, sizeof(char)*4);         		
    }
    printf("\n\n\n\t\t\t\t---END OF FILE---\n");
    
    fclose(pfFilePtr);
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "MyUsage"
// Parameters     : <argc> : num of program's parameters
//                  <argv> : program's parameter arrays
// Description    : Checks the correctness of the parameters
//
/*----------------------------------------------------------------------------*/
int MyUsage(int argc)
{
    if(argc != 3)
    {
        fprintf(stderr, "\n\t Usage:./moreOrLess [FILENAME] [SIZE..]\n\n  %s\t\n  %s\t\n\n",
                        "This program reads rows from the file.",
                        "Then shows until given size or one.");
        fflush(stderr);
        return ERROR;
    }
    else{ 
        fprintf(stderr, "\n\n\tInfo:\n\tShould be used: UP,Down,Enter,Q(QUIT) and Esc keys.\n\n");
                        
        fflush(stderr);
        return TRUE;
    }    
}
/*----------------------------------------------------------------------------*/
//                              - END OF FILE -
/*----------------------------------------------------------------------------*/