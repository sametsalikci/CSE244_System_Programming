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
#define FILE_ERROR  -2

# define BUFFSIZE     1024
# define MAX_PATH     1024
# define WORD_SIZE	  30

# define FIFO_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
# define READ_MODE 	 "r"
# define WRITE_MODE  "w"

/*----------------------------------------------------------------------------*/
//                                MY STRUCT
/*----------------------------------------------------------------------------*/
typedef struct{
	char word[WORD_SIZE];
	int number;		
}Book;

typedef union{
    char size[1024];
    Book b;
}writeable;
/*----------------------------------------------------------------------------*/
//                                MY FUNCTIONS
/*----------------------------------------------------------------------------*/
void fnAllocate(char**ptr,int size);
void fnMyFree(Book*ptrsBook,int size);
int fnCountWordsInTheFile(FILE*pfFilename);
int fnIsWord(const char*cptrWord,int iWordSize);
int fnGetLine(char*cptrLine,int iLineSize);
int fnAddWordToStruct(Book*ptrsBook,char*cptrLine,int *iIndex);
Book*fnFindWordsInTheFile(FILE*pfFilename,Book*ptrsBook,int*iptrSize);
int fnTheSameTwoString(char*ptrStr,char*ptrToken,int iStrSize,int iTokenSize);
int fnSearchWordInTheStruct(Book*ptrsBook,char*ptrSearchStr,int iSize,int*iptrWordCount,int iStart);
Book* fnFindTotalWordsIntheFile(char*pfFilename,Book*ptrsBook,int *iptrIndex);
pid_t fnWaitChilds(int*iptrStatLoc);
char*fnIgnoreBackSlashN(char *cptrStr,int iNumberSize);
Book*fnCombineAllOfWords(Book *ptrsBook,int iNumberSize,int *iptrNewsize);
int fnGetWordNumber(Book*ptrsBook,char*ptrSearchStr,int iSize,int*iptrWordCount,int iStart);
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
    FILE*pfLogFileOutput;
    Book*ptrsBook;
    int iIndex;
    int iFifo;
    int iFileCount = 0;
    int iTempCount = 0;
    int waitStatus;
    char cptrFifoName[] = "myFifo";


    gettimeofday(&startTime, NULL);

   
    piDirSize = fnDirSize(argv[1]);
    if ( MyUsage(argc,argv) == ERROR ){ 
    	return 0;

    }
    if(( pfLogFileOutput = fopen("logFile.txt","w")) == NULL){
        fprintf(stdout,"Failed to open file for writting!!\n");
        return 0;
    } 
    getcwd(root, MAX_PATH);
    strcpy(path, root);

    if (argc==2) { 
        strcat(root,"/");
        strcat(root,argv[1]);
        strcpy(path, argv[1]);
    }
     //Create fifo
    if (mkfifo(cptrFifoName,FIFO_PERMS) == -1)
    {
        perror("Failed to create myfifo");
    }

    if (chdir(root) == -1) {
        fprintf (stdout, "Error \"%s\" : %s\n", path, strerror(errno));
        fflush(stdout);
        exit(0);
    }
    if(( pfOutputFile = fopen("FilePathRoot.txt","w")) == NULL){
        fprintf(stdout,"Failed to open file for writting!!\n");
        return 0;
    } 
    
    getPathFileInDirectory(root, "",pfOutputFile,&piDirSize);
    fclose(pfOutputFile);

    if(( pfInputFile = fopen("FilePathRoot.txt","r")) == NULL){
     	fprintf(stdout,"Failed to open file for reading: %s \n","FilePathRoot.txt");
     	return 0;
 	} 
 	
 	//My new Programa
    iRowSizeInFile = fnFindRowNumber(pfInputFile);
    rewind(pfInputFile);
   	int i = 0;
    char cptrStr[255];
    int iSizeArr[iRowSizeInFile];

    iFifo= open(cptrFifoName,O_RDWR);

    
    for (i = 0; i < 10; ++i)
    {	     	
       
        fscanf(pfInputFile,"%s",cptrStr);
        if (fnTheSameTwoString(cptrStr,cptrFifoName,strlen(cptrStr),strlen(cptrFifoName)))
        { 
            continue;
        }
        if (fnTheSameTwoString(cptrStr,"newWordCount",strlen(cptrStr),strlen("newWordCount")))
        {   
            continue;
        }
        if (fnTheSameTwoString(cptrStr,"FilePathRoot.txt",strlen(cptrStr),strlen("FilePathRoot.txt")))
        {   
            continue;
        }
        if (fnTheSameTwoString(cptrStr,"newWordCount.c",strlen(cptrStr),strlen("newWordCount.c")))
        {   
            continue;
        }
        int pipefd[2];
        if(pipe(pipefd) < 0){
            perror("pipe");
            return -1;
        }
        pidChildPid = fork();

        if (pidChildPid == -1)
    	{
            ++iFileCount;
            continue;
    	}
    	else if (pidChildPid == 0)
    	{	           
            ptrsBook = 	fnFindTotalWordsIntheFile(cptrStr,ptrsBook,&iIndex);
            if (ptrsBook == NULL)
            {
                ++iFileCount;
                exit(0);
                continue;
                
            }
            int x = 0;          
            for (x = 0; x < iIndex; ++x)
            {   printf("%d.test\n",i);
                writeable iTempBook;
                iTempBook.b.number = ptrsBook[x].number;
                strcpy(iTempBook.b.word,fnIgnoreBackSlashN(ptrsBook[x].word,strlen(ptrsBook[x].word)));
                write(iFifo,&iTempBook,sizeof(writeable));
            }
            close(pipefd[0]);
            write(pipefd[1], &iIndex, sizeof(int));
            close(pipefd[1]);
            free(ptrsBook);
    		exit(0);
    	} else {
            fnWaitChilds(NULL);
            close(pipefd[1]);
            int iTemp = 0;

            read(pipefd[0], &iTemp, sizeof(iTemp));
            close(pipefd[0]);
            iTempCount += iTemp;
        }
                         	
    }
    printf("read\n");
    writeable iTempStruct[iTempCount];
    for (i = 0; i < iTempCount; ++i)
    {           
        read(iFifo,&iTempStruct[i],sizeof(writeable));     
    }
    Book bTempBook[iTempCount];

    for (i = 0; i < iTempCount; ++i)
    {
        strcpy(bTempBook[i].word,iTempStruct[i].b.word);
        bTempBook[i].number = iTempStruct[i].b.number;
    }
    int iNumber = 0;
    int iTotalWordsNumber = 0;
    Book*ptrsTemp = fnCombineAllOfWords(bTempBook,iTempCount,&iNumber);
    for (i = 0; i < iNumber; ++i)
    {
        fprintf(pfLogFileOutput," %s : %d\n",ptrsTemp[i].word,ptrsTemp[i].number);
        iTotalWordsNumber += ptrsTemp[i].number;
    }
    printf("\n\n\t\tTOTAL UNIQUE WORDS NUMBER : %d\n",iNumber);
    printf("\n\n\t\tTOTAL WORDS NUMBER : %d\n",iTotalWordsNumber);
    
    close(iFifo);
    if (unlink(cptrFifoName) == -1)
        perror("Failed to remove myfifo");
     
    remove("FilePathRoot.txt");
    
    free(ptrsTemp);
    
    gettimeofday(&endTime, NULL);
    miliSecTime = ((endTime.tv_sec  - startTime.tv_sec) * 1000 + 
                  (endTime.tv_usec - startTime.tv_usec)/1000.0) + 0.5;
    printf("\nExecution Time: %.3lf miliseconds\n", miliSecTime);
    printf("Done! pid : %d\n", getpid());
	

    return 0;
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "fnGetWordNumber"
// Parameters     : <ptrsBook> : struct pointer
//                  <ptrSearchStr> : string pointer
//                  <iSize> : struct size
//                  <iptrWordCount> : total word size in the struct
//                  <iStart> : starter value
// Description    : search a word in the struct
//
/*----------------------------------------------------------------------------*/
Book*fnCombineAllOfWords(Book *ptrsBook,int iNumberSize,int *iptrNewsize){

    int i ;
    int iIndex1 = 0;
    int iTempCount = 0;
    Book* ptrsTemp = (Book*)malloc(sizeof(Book) * (iNumberSize+1));

    for ( i = 0; i < iNumberSize; ++i){
        
        if (fnSearchWordInTheStruct(ptrsTemp,ptrsBook[i].word,iTempCount,&iIndex1,0) == FALSE)
        {   
            iIndex1 = 0;
            fnGetWordNumber(ptrsBook,ptrsBook[i].word,iNumberSize,&iIndex1,i);
            strcpy(ptrsTemp[iTempCount].word,ptrsBook[i].word);
            ptrsTemp[iTempCount].number = iIndex1;
            ++iTempCount;
        }
    }
    *iptrNewsize = iTempCount;
           
    return ptrsTemp;   
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "fnGetWordNumber"
// Parameters     : <ptrsBook> : struct pointer
//                  <ptrSearchStr> : string pointer
//                  <iSize> : struct size
//                  <iptrWordCount> : total word size in the struct
//                  <iStart> : starter value
// Description    : search a word in the struct
//
/*----------------------------------------------------------------------------*/
int fnGetWordNumber(Book*ptrsBook,char*ptrSearchStr,int iSize,int*iptrWordCount,int iStart){

    int i;
    int count = 0;
    int status = 0;
    for ( i = iStart; i < iSize; ++i)
    {
        if (fnTheSameTwoString(ptrsBook[i].word,ptrSearchStr,strlen(ptrsBook[i].word),strlen(ptrSearchStr)))
        {
            status = 1;
            count += ptrsBook[i].number;
        }
    }
    *iptrWordCount = count;
    return status;
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "fnIgnoreBackSlashN"
// Parameters     : <cptrStr> : char pointer type for word 
//                  <iNumberSize> : string size
// Description    : ignore '\n' character 
//
/*----------------------------------------------------------------------------*/
char*fnIgnoreBackSlashN(char *cptrStr,int iNumberSize){
    int i = 0;
    for ( i = 0; i <iNumberSize; ++i)
    {
        if (cptrStr[i] == '\n')
        {
            cptrStr[i] = ' ';
            return cptrStr;
        }
    }
    return cptrStr;
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "fnFindTotalWordsIntheFile"
// Parameters     : <pfFilename> : char pointer type for file name 
//                  <ptrsBook> : struct pointer
//					<iptrIndex> : struct index size
// Description    : arrange struct for all words in the file 
//
/*----------------------------------------------------------------------------*/
Book* fnFindTotalWordsIntheFile(char*pfFilename,Book*ptrsBook,int *iptrIndex){
	
	FILE *pfInputFile;
	if(( pfInputFile = fopen(pfFilename,"r")) == NULL){
         fprintf(stderr,"Failed to open file for reading: %s\n",pfFilename);
         return NULL;
    }
	int iTempCount = 0;
	int iIndex = 0;
	int iIndex1 = 0;
	int i,j;
	int iNumberSize = fnCountWordsInTheFile(pfInputFile);
	rewind(pfInputFile);
	Book*ptrTemp = fnFindWordsInTheFile(pfInputFile,ptrTemp,&iIndex);
	ptrsBook = (Book*)malloc(sizeof(Book) * (iNumberSize+1));
	for ( i = 0; i < iIndex; ++i)
	{
		if (fnSearchWordInTheStruct(ptrsBook,ptrTemp[i].word,iTempCount,&iIndex1,0) == FALSE)
		{	
			iIndex1 = 0;
			fnSearchWordInTheStruct(ptrTemp,ptrTemp[i].word,iIndex,&iIndex1,i+1);
			strcpy(ptrsBook[iTempCount].word,ptrTemp[i].word);
			ptrsBook[iTempCount].number = iIndex1;
			++iTempCount;
		}
	}
	free(ptrTemp);
	*iptrIndex = iTempCount;
	fclose(pfInputFile);	
	return ptrsBook;
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "fnSearchWordInTheStruct"
// Parameters     : <ptrsBook> : struct pointer
//                  <ptrSearchStr> : string pointer
//					<iSize> : struct size
//					<iptrWordCount> : total word size in the struct
//					<iStart> : starter value
// Description    : search a word in the struct
//
/*----------------------------------------------------------------------------*/
int fnSearchWordInTheStruct(Book*ptrsBook,char*ptrSearchStr,int iSize,int*iptrWordCount,int iStart){

	int i;
	int count = 1;
	int status = 0;
	for ( i = iStart; i < iSize; ++i)
	{
		if (fnTheSameTwoString(ptrsBook[i].word,ptrSearchStr,strlen(ptrsBook[i].word),strlen(ptrSearchStr)))
		{
			status = 1;
			++count;
		}
	}
	*iptrWordCount = count;
	return status;
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "fnTheSameTwoString"
// Parameters     : <ptrToken> : target string
//                  <ptrStr> : source string
//					<iptrSize> : source size
//					<iTokenSize> : target size
// Description    : checks whether  the two string are the same 
//
/*----------------------------------------------------------------------------*/
int fnTheSameTwoString(char*ptrStr,char*ptrToken,int iStrSize,int iTokenSize){

	int i,j;
	if (iStrSize == iTokenSize){	
		for ( i = 0; i < iStrSize; ++i)
		{
			if (ptrStr[i] != ptrToken[i])
			{
				return FALSE;
			}			
		}
	}
	else{
		return FALSE;
	}
	return TRUE;
}

/*----------------------------------------------------------------------------*/
//
// Function Name  : "fnFindWordsInTheFile"
// Parameters     : <pfFilename> : file pointer
//                  <ptrsBook> : struct pointer
//					<iptrSize> : struct size
// Description    : find the all words in the file and adds words to struct
//
/*----------------------------------------------------------------------------*/
Book* fnFindWordsInTheFile(FILE*pfFilename,Book*ptrsBook,int*iptrSize){
	char buf[BUFFSIZE] = {0x0};
	int iIndex = 0;
	int iNumberSize = fnCountWordsInTheFile(pfFilename);
	rewind(pfFilename);
	ptrsBook = (Book*)malloc(sizeof(Book) * (iNumberSize+1));
	while( !feof (pfFilename) && !ferror (pfFilename) 
          && fgets (buf, sizeof (buf), pfFilename) != NULL ){
		fnAddWordToStruct(ptrsBook,buf,&iIndex);
	}
	*iptrSize = iIndex;
	return ptrsBook;
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "fnMyFree"
// Parameters     : <ptrsBook> : the word
//                  <size> : struct size
// Description    : does free
//
/*----------------------------------------------------------------------------*/
void fnMyFree(Book*ptrsBook,int size){
	int i ;
	for ( i = 0; i < size; ++i)
	{
		free(ptrsBook[i].word);
	}
	free(ptrsBook);
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
		if (isalpha(cptrWord[i]) == 0 )
		{	
			if (cptrWord[i] != '\n')
			{
				return FALSE;
			}			
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
// Function Name  : "fnGetLine"
// Parameters     : <cptrLine> : sentences of words
//                  <iLineSize> : sentences size
// Description    : calculating right words in sentence
//
/*----------------------------------------------------------------------------*/
int fnAddWordToStruct(Book*ptrsBook,char*cptrLine,int *iptrIndex){

	int iCountWords = 0;
	int iIndex = *iptrIndex;
	char*cptrToken;
	cptrToken = strtok(cptrLine," ");
	while(cptrToken !=NULL){
		
		if (fnIsWord(cptrToken,strlen(cptrToken)))
		{
			iCountWords += 1;
			strcpy(ptrsBook[iIndex].word,cptrToken);
			++(iIndex);
			cptrToken = strtok(NULL," ");
		}
		else{
			cptrToken = strtok(NULL," ");
		}
	}	
	*iptrIndex = iIndex;
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
        fprintf (stdout, "Failed to open directory \"%s\" : %s\n",
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
                fprintf(stdout, "Failed to fork, for directory \"%s\":%s %d\n", direntp->d_name, strerror(errno), getpid());
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
                    fprintf (stdout, "Error \"%s\" : %s\n", newPath, strerror(errno));
                    fflush(stdout);
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
int MyUsage(int argc,const char* argv[])
{
    if(argc != 2)
    {
        fprintf(stdout,"\n\n\t Usage: %s [DIRECTORY]\n\n  %s\n  %s\n\n",
                        argv[0],
                        "This program is listing files from the given directory with subdirectories.",
                        "Then counting words in every files");
        fflush(stdout);
        return -1;
    }
    return TRUE;
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
pid_t fnWaitChilds(int*iptrStatLoc)
{
    int iRetval;

    while (((iRetval = wait(iptrStatLoc)) == -1) && (errno == EINTR));
    return iRetval;
}