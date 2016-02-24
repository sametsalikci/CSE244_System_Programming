/*----------------------------------------------------------------------------*/
//
//  Author        : Mehmet Mazhar SALIKCI
//	number		  : 111044050
//  Date          : April 25,2015
//  FileName      : "wordCount.c"
//                
//  Description   : Bu program bir klasorun altindaki tum dosyaları bulur,
//                  her bulduğu dosya icin therad olusturu.Daha sonra oluşturulan thread dosyalardaki
//                  kelime sayısını bulur.(harf dısında karekter icermeyen kelimeler).
//                  
//
/*----------------------------------------------------------------------------*/
//                                 INCLUDES
/*----------------------------------------------------------------------------*/
#include <sys/select.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

/*----------------------------------------------------------------------------*/
//                                 DEFINES
/*----------------------------------------------------------------------------*/
#define TRUE         1
#define FALSE        0
#define ERROR       -1

#define BUFFSIZE     1024

#define READ_MODE 	 "r"
#define WRITE_MODE   "w"
#define LOG_FILE "LogFile.txt"

/*----------------------------------------------------------------------------*/
//                                MY FUNCTIONS
/*----------------------------------------------------------------------------*/
int fnIsDirectory(char *path);
int getPathFileInDirectory(char root[],char path[],FILE*filename,int*piDirSize);
int fnDirSize(const char *path);
static inline int MyUsage(int argc,const char* argv[]);
int fnIsWord(const char*cptrWord,int iWordSize);
int fnGetLine(char*cptrLine,int iLineSize);
int fnCountWordsInTheFile(FILE*fptrFilename);
int fnFindRowNumber(FILE *pfInp);
int fnFindLongLine(FILE *pfInp);
void*fnReturnWordCount(void*arg);
/*----------------------------------------------------------------------------*/
//                                MY STRUCT
/*----------------------------------------------------------------------------*/
typedef struct
{
    char chFilePath[PATH_MAX];
    int iWordCount;
    double iMilSec;
}MyThreadStruct;
/*----------------------------------------------------------------------------*/
//                                MAIN FUNCTION 
/*----------------------------------------------------------------------------*/
int main(int argc, char const *argv[])
{
	FILE*pfOutputFile= NULL,
		*pfInputFile= NULL;
	int iRowSizeInFile;
	struct timeval startTime, endTime;
    double miliSecTime;
    char root[PATH_MAX]={0x0};
    char path[PATH_MAX]={0x0};
    int iCurrWordSize;
    int iTotalWordsSize;
    FILE*pfLogFile= NULL;
    FILE*pfHoldWordSizeFile= NULL;
    int piDirSize = 0;
    int i;
    int iResultTotal = 0;
    
    gettimeofday(&startTime, NULL);
    piDirSize = fnDirSize(argv[1]);
    if(( pfLogFile = fopen(LOG_FILE,"w")) == NULL){
        fprintf(stderr,"Failed to open file for writting!!\n");
        return 0;
    }

    if ( MyUsage(argc,argv) == ERROR ){ 
    	return 0;

    }else{
        
        getcwd(root, PATH_MAX);
        strcpy(path, root);

        if (argc==2){ 
            strcat(root,"/");
            strcat(root,argv[1]);
            strcpy(path, argv[1]);
        }
        if (chdir(root) == -1){
            fprintf (stderr, "Error \"%s\" : %s\n", path, strerror(errno));
            fflush(stderr);
            exit(0);
        }
        if((pfOutputFile = fopen("FilePathRoot.txt","w")) == NULL){
            printf("Failed to open file for writting!!\n");
            return 0;
        } 
        
        getPathFileInDirectory(root, "",pfOutputFile,&piDirSize);
        fclose(pfOutputFile);

        if(( pfInputFile = fopen("FilePathRoot.txt","r")) == NULL){
         	printf("Failed to open file for reading!! \n");
         	return 0;
     	} 

        iRowSizeInFile = fnFindRowNumber(pfInputFile);
        rewind(pfInputFile);
        MyThreadStruct stThread[iRowSizeInFile];
        pthread_t thread[iRowSizeInFile];
       	char cptrStr[PATH_MAX];
        int j = 0;
        for ( i = 0; i < iRowSizeInFile; ++i)
        {
            memset(cptrStr,'\0', PATH_MAX * sizeof(char));
            memset(stThread[i].chFilePath,'\0',PATH_MAX * sizeof(char));
            fgets(cptrStr, PATH_MAX, pfInputFile);
            strncpy(stThread[i].chFilePath,cptrStr,strnlen(cptrStr, PATH_MAX)-1);     
        }
        int iError;
        for (i = 0; i < iRowSizeInFile; ++i)
        {
            if (iError = pthread_create(&thread[i],NULL,fnReturnWordCount,&stThread[i]))
            {
              fprintf(stderr, "Failed to create thread %d: %s\n",i+1,strerror(iError));
            }
        }
        for (i = 0; i < iRowSizeInFile; ++i)
        {	
           	if (iError = pthread_join(thread[i],NULL))
            {
                fprintf(stderr, "Failed to join thread\n");
                continue;
            }
            printf("\nFileName : %s\n",stThread[i].chFilePath);
            printf("Execution Time: %.6lf miliseconds.\n", stThread[i].iMilSec);
            iTotalWordsSize += stThread[i].iWordCount;	    	 	
        }
    }
    fprintf(pfLogFile, "\n\nTOTAL FILES: %d\n",iRowSizeInFile);
    fprintf(pfLogFile, "TOTAL THREAD: %d\n",iRowSizeInFile);
    fprintf(pfLogFile, "TOTAL WORDS: %d\n",iTotalWordsSize);   
    fprintf(pfLogFile, "TOTAL DIRECTORY: %d\n",piDirSize);
    
    fclose(pfInputFile);
    fclose(pfLogFile);  
    remove("FilePathRoot.txt");     	    
	
   
    gettimeofday(&endTime, NULL);
    miliSecTime = ((endTime.tv_sec  - startTime.tv_sec) * 1000 + 
                  (endTime.tv_usec - startTime.tv_usec)/1000.0) + 0.5;
    printf("\n\nTOTAL Execution Time: %.6lf miliseconds\n", miliSecTime);
    
	return 0;
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "fnReturnWordCount"
// Parameters     : 
// Description    : read and open file then return word number
//
/*----------------------------------------------------------------------------*/
void*fnReturnWordCount(void*arg){

    struct timeval startTime1, endTime1;
    double miliSecTime;

    gettimeofday(&startTime1, NULL);
    MyThreadStruct *stTemp = (MyThreadStruct*)arg;

    FILE*fpInpFile = fopen(stTemp->chFilePath,READ_MODE);
    if (fpInpFile ==NULL)
    {
        fprintf(stderr, "Failed to open file:%s\n",stTemp->chFilePath);
        return (void*)0;
    }
    stTemp->iWordCount = fnCountWordsInTheFile(fpInpFile);

    gettimeofday(&endTime1, NULL);
    miliSecTime = ((endTime1.tv_sec  - startTime1.tv_sec) * 1000 + 
                  (endTime1.tv_usec - startTime1.tv_usec)/1000.0) + 0.5;
    stTemp->iMilSec = miliSecTime;
    //printf("\nFileName : %s\n",stTemp->chFilePath);
    fclose(fpInpFile);
    return (void*)1;
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "fnIsDirectory"
// Parameters     : <path> : file path
// Description    : Checks whether it is a directory
//
/*----------------------------------------------------------------------------*/
int fnIsDirectory(char *path){
    struct stat statbuf;
    
    if (stat(path, &statbuf) == -1) 
        return ERROR;
    else
        return S_ISDIR(statbuf.st_mode);
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "getDirectory"
// Parameters     : <root> : path from linux root
//                  <path> : path from current working directory
// Description    : Listing the files from directory with subdirectories
//                  multi-process funciton
//
/*----------------------------------------------------------------------------*/
int getPathFileInDirectory(char root[], char path[],FILE*filename,int*piDirSize) {
   
    struct dirent *direntp;
    DIR    *dirp;
    pid_t  childpid;
    int    empty = 1;
    char   newPath  [PATH_MAX] = "";
    char   newRoot [PATH_MAX] = "";
    

    if ((dirp = opendir(root)) == NULL) {
        fprintf (stderr, "Failed to open directory \"%s\" : %s\n",
                 direntp->d_name, strerror(errno));
        return 1;
    } 
    while ((direntp = readdir(dirp)) != NULL) {

        if (direntp->d_name[0]=='.') continue;
        empty=0;

        if (fnIsDirectory(direntp->d_name)) {
            //(*piDirSize)++;
            childpid = fork();
            if (childpid == -1) {
                fprintf(stderr, "Failed to fork, for directory \"%s\":%s %d\n", direntp->d_name, strerror(errno), getpid());
                return 2;
            }
            else if (childpid == 0) {

                if (path != "") {

                    strcat(newPath, path);
                    strcat(newPath, "/");
                }
                strcat(newPath, direntp->d_name);
                strcat(newRoot, root);
                strcat(newRoot, "/");
                strcat(newRoot, direntp->d_name);

                if (chdir(newRoot) == -1) {
                    fprintf (stderr, "Error \"%s\" : %s\n", newPath, strerror(errno));
                    fflush(stderr);
                    exit(0);
                }
                
                getPathFileInDirectory(newRoot, newPath,filename,piDirSize); // recursive call - sets new child's directory

                exit(0); // kill child
            }
            else {
                wait(NULL);// ++dirNum;
            }
        } // Directory icindeki okunan name'in filename olma durumu
        else {

            if (path != ""){
                fprintf(filename, "%s/%s\n",path, direntp->d_name);}
            else fprintf(filename, "%s\n",direntp->d_name);
                fflush(filename);
        }
    }

    if ( empty == 1 ) {
        fprintf(stdout, "PPid:%6d  Pid: %6d empty : %s\n", getppid(), getpid(), direntp->d_name);
        fflush(stdout);
    }

    while ((closedir(dirp) == -1) && (errno == EINTR)); // force to close directory

    return 0;
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "fnDirSize"
// Parameters     : <path> : path from linux root
// Description    : counting directory from directory with subdirectories
//                  multi-process funciton
//	Note = referans :"stackOwerflow"
//
/*----------------------------------------------------------------------------*/
int fnDirSize(const char *path)
{
    struct dirent *direntp = NULL;
    DIR *dirp = NULL;
    size_t path_len;
    int sizeCount = 0;

    /* Check input parameters. */
    if (!path)
        return -1;
    path_len = strlen(path);  

    if (!path || !path_len || (path_len > _POSIX_PATH_MAX))
        return -1;

    /* Open directory */
    dirp = opendir(path);
    if (dirp == NULL)
        return -1;

    while ((direntp = readdir(dirp)) != NULL)
    {
        /* For every directory entry... */
        struct stat fstat;
        char full_name[_POSIX_PATH_MAX + 1];

        /* Calculate full name, check we are in file length limts */
        if ((path_len + strlen(direntp->d_name) + 1) > _POSIX_PATH_MAX)
            continue;

        strcpy(full_name, path);
        if (full_name[path_len - 1] != '/')
            strcat(full_name, "/");
        strcat(full_name, direntp->d_name);

        /* Ignore special directories.*/
        if ((strcmp(direntp->d_name, ".") == 0) ||
            (strcmp(direntp->d_name, "..") == 0))
            continue;

        /* Print only if it is really directory. */
        if (stat(full_name, &fstat) < 0)
            continue;
        if (S_ISDIR(fstat.st_mode))
        {
            //printf("%s\n", full_name);
            
            //if (recursive)
             sizeCount = 1 + fnDirSize(full_name);
        }
    }
    /* Finalize resources. */
    (void)closedir(dirp);
    return sizeCount;
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "MyUsage"
// Parameters     : <argc> : num of program's parameters
//                  <argv> : program's parameter arrays
// Description    : Checks the correctness of the parameters
//
/*----------------------------------------------------------------------------*/
static inline int MyUsage(int argc,const char* argv[])
{
    if(argc != 2)
    {
        fprintf(stderr,"\n\n\t Usage: %s [DIRECTORY]\n\n  %s\n  %s\n\n",
                        argv[0],
                        "This program is listing files from the given directory with subdirectories.",
                        "Then counting words in every files");
        fflush(stderr);
        return ERROR;
    }
    return TRUE;
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "fnIsWord"
// Parameters     : <cptrWord> : the word
//                  <iWordSize> : word size
// Description    : Checks the correctness of the word
//
/*----------------------------------------------------------------------------*/
int fnIsWord(const char*cptrWord,int iWordSize){
	int i;
	for (i = 0; i < iWordSize; ++i)
	{
		if (isalpha(cptrWord[i]) == 0)
		{
			return FALSE;
		}
	}
	return TRUE;
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "fnGetLine"
// Parameters     : <cptrLine> : sentences of words
//                  <iLineSize> : sentences size
// Description    : calculating right words in sentence
//
/*----------------------------------------------------------------------------*/
int fnGetLine(char*cptrLine,int iLineSize){

	int iCountWords = 0;
	char*cptrToken;
	cptrToken = strtok(cptrLine," ");
	while(cptrToken !=NULL){
		
		if (fnIsWord(cptrToken,strlen(cptrToken)))
		{
			iCountWords += 1;
			cptrToken = strtok(NULL," ");
		}
		else{
			cptrToken = strtok(NULL," ");
		}
	}
	return iCountWords;
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "fnCountWordsInTheFile"
// Parameters     : <pfFilename> : sentences of words
// Description    : calculating right words in the file
//
/*----------------------------------------------------------------------------*/
int fnCountWordsInTheFile(FILE*pfFilename){
	
	char buf[BUFFSIZE] = {0x0};
	int iTotalWordInFile = 0;
	
	while( !feof (pfFilename) && !ferror (pfFilename) 
          && fgets (buf, sizeof (buf), pfFilename) != NULL ){

		iTotalWordInFile += fnGetLine(buf,strlen(buf));
	}
	return iTotalWordInFile;
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "fnFindLongLine"
// Parameters     : <pfInp> : file Pointer
// Description    : find the row length in the file
//
/*----------------------------------------------------------------------------*/
int fnFindLongLine(FILE *pfInp)
{
	char ch;
	int icountChar = 0;
	ch = fgetc(pfInp);
	
	while(ch != '\n'){
		++icountChar;	
		ch=fgetc(pfInp);
	}
	return (icountChar+2);
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "fnFindRowNumber"
// Parameters     : <pfInp> : file Pointer
// Description    : find the row size in the file
//
/*----------------------------------------------------------------------------*/
int fnFindRowNumber(FILE *pfInp)
{	
	int icountRow = 0;
	int ch;
	
	ch = getc(pfInp);
	
	while(ch != EOF){
		
		if(ch == '\n')
			++icountRow; 
		ch = fgetc(pfInp);

	}
	return icountRow;
}