/*----------------------------------------------------------------------------*/
//
//  Author        : Mehmet Mazhar SALIKCI
//	number		  : 111044050
//  Date          : March 25,2015
//  FileName      : "wordCount.c"
//                
//  Description   : Bu program bir klasorun altindaki tum dosyaları bulur,
//                  her bulduğu dosyayı forklar.Daha sonra forklanan dosyalardaki
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

/*----------------------------------------------------------------------------*/
//                                 DEFINES
/*----------------------------------------------------------------------------*/
#define TRUE         1
#define FALSE        0
#define ERROR       -1

#define BUFFSIZE     1024
#define MAX_PATH     1024

#define READ_MODE 	 "r"
#define WRITE_MODE   "w"
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
/*----------------------------------------------------------------------------*/
//                                MAIN FUNCTION 
/*----------------------------------------------------------------------------*/
int main(int argc, char const *argv[])
{
	FILE*pfOutputFile,
		*pfInputFile;
	int iRowSizeInFile;
	pid_t pidChildPid;
	struct timeval startTime, endTime;
    double miliSecTime;
    char root[MAX_PATH]={0x0};
    char path[MAX_PATH]={0x0};
    int icharNumber;
    char*cptrLineString;
    FILE*pfTempFile;
    int iCurrWordSize;
    int iTotalWordsSize;
    FILE*pfProgramOutput;
    int iStatus;
    FILE*pfHoldWordSizeFile;
    int piDirSize = 0;
    
    gettimeofday(&startTime, NULL);

    if(( pfProgramOutput = fopen("pfProgramOutput.txt","w")) == NULL){
         printf("Failed to open file for writting!!\n");
         return 0;
     }

       piDirSize = fnDirSize(argv[1]);
    if ( MyUsage(argc,argv) == ERROR ){ 
    	return 0;

    }else{
	    getcwd(root, MAX_PATH);
	    strcpy(path, root);

	    if (argc==2) { 
	        strcat(root,"/");
	        strcat(root,argv[1]);
	        strcpy(path, argv[1]);
	    }

	    if (chdir(root) == -1) {
	        fprintf (stderr, "Error \"%s\" : %s\n", path, strerror(errno));
	        fflush(stderr);
	        exit(0);
	    }
	    if(( pfOutputFile = fopen("FilePathRoot.txt","w")) == NULL){
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
	   	int i;
        char cptrStr[255];
	    for (i = 0; i < iRowSizeInFile; ++i)
	    {	
	    		    	
	    	fscanf(pfInputFile,"%s",cptrStr);
	    	pidChildPid = fork();

	    	if (pidChildPid == -1)
	    	{
	    		perror("Failed to fork!!");
	    		return 1;
	    	}
	    	if (pidChildPid == 0)
	    	{	
	    		if(( pfTempFile = fopen(cptrStr,"r")) == NULL){
                    printf("Failed to open file for reading!! \n");
                    return 0;
                }
                if(( pfHoldWordSizeFile = fopen("pfHoldWordSizeFile.txt","w")) == NULL){
                    printf("Failed to open file for writting!!\n");
                    return 0;
                } 

	    		iCurrWordSize = fnCountWordsInTheFile(pfTempFile);
                fprintf(pfHoldWordSizeFile, "%d\n",iCurrWordSize);
                fprintf(pfProgramOutput, "%d.Pid:[%6d]--FilePath:[%s]--WordSize:[%d]\n",i+1,getpid(),cptrStr,iCurrWordSize);
	    		fclose(pfTempFile);
                fclose(pfHoldWordSizeFile);
	    		exit(0);
	    	}
	    	else{              
	    		wait(NULL);
                if(( pfHoldWordSizeFile = fopen("pfHoldWordSizeFile.txt","r")) == NULL){
                    printf("Failed to open file for reading!!\n");
                    return 0;
                }  
                fscanf(pfHoldWordSizeFile,"%d",&iStatus);
	    		iTotalWordsSize += iStatus;
                fclose(pfHoldWordSizeFile);
	    	}
	    }     	    
	}
   
    fprintf(pfProgramOutput, "\n\nTOTAL FILES: %d\n",iRowSizeInFile);
    fprintf(pfProgramOutput, "TOTAL CHILDS: %d\n",iRowSizeInFile);
    fprintf(pfProgramOutput, "TOTAL WORDS: %d\n",iTotalWordsSize);   
    fprintf(pfProgramOutput, "TOTAL DIRECTORY: %d\n",piDirSize);
    fclose(pfInputFile);
    fclose(pfProgramOutput);  
    
    remove("pfHoldWordSizeFile.txt");
    remove("FilePathRoot.txt");
   
    gettimeofday(&endTime, NULL);
    miliSecTime = ((endTime.tv_sec  - startTime.tv_sec) * 1000 + 
                  (endTime.tv_usec - startTime.tv_usec)/1000.0) + 0.5;
    printf("\nExecution Time: %.3lf miliseconds\n", miliSecTime);
    printf("Done! pid : %d\n", getpid());
    
	return 0;
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
    char   newPath  [MAX_PATH] = "";
    char   newRoot [MAX_PATH] = "";
    

    if ((dirp = opendir(root)) == NULL) {
        fprintf (stderr, "Failed to open directory \"%s\" : %s\n",
                 direntp->d_name, strerror(errno));
        return 1;
    } 
    while ((direntp = readdir(dirp)) != NULL) {

        if (direntp->d_name[0]=='.') continue;
        empty=0;

        if (fnIsDirectory(direntp->d_name)){
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