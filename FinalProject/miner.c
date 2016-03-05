
/*----------------------------------------------------------------------------*/
//
//  Author        : Mehmet Mazhar SALIKCI
//	number		  : 111044050
//  Date          : May 19,2015
//  FileName      : "miner.c"
//                
//  Description   : 
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
#include <math.h>
#include <sys/shm.h> 
#include <sys/types.h>
#include <sys/ipc.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/sem.h>

/*----------------------------------------------------------------------------*/
//                                 DEFINES
/*----------------------------------------------------------------------------*/
#define TRUE         1
#define FALSE        0
#define ERROR       -1
#define FILE_ERROR  -2
#define EXIT_SIGHANDLER_FAIL 5
#define EXIT_TIME_GET_FAIL 2

# define TOTAL_WORD_SIZE 500000
# define BUFFSIZE     1024
# define RESULT_SIZE 100
# define FILE_SIZE 100
# define FILE_SIZE_INFO 1000
# define OP_SIZE 12
# define PAR_SIZE 12
# define FIFO_SIZE 12
# define MAX_CHILD 100
# define MAX_CLIENTS 100
# define S_LOG_NAME_MAX 30
# define EXIT_SIGINT 3
# define EXIT_FORK_FAIL 6
# define MILLION 1000000L
# define MAIN_FIFO_SIZE 50
# define WORD_SIZE 50
# define PATH_SIZE 10000
# define MAX_PATH 256
# define KEY_T_CLIENT_REQUEST 12361111

# define SHMSZ_FOR_FILE  2048
# define SHMSZ_FOR_WORD  50000
# define KEY_T_FOR_WORD  12342323
# define KEY_T_FOR_PATH  21405511
# define KEY_T_FOR_PATH_SIZE 12371212
# define KEY_T_FOR_WORDS_SIZE 12381313
# define KEY_T_FOR_STATUS 22222222

#define SEMA_NAME1 "/mysem3"
#define SEMA_NAME2 "/mysem4"

# define FIFO_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
/*----------------------------------------------------------------------------*/
//                                MY STRUCT
/*----------------------------------------------------------------------------*/
typedef struct{

    char chPath[PATH_MAX];
}Path_t;

typedef struct 
{
    char chWord[WORD_SIZE];
    int iWordNumber;
}Words_t;

typedef struct
{   
    int iClientID;
    int iFileNumber;
    int iPoints;  
}InfoClient_t;

typedef struct{ 
    
    char chFilePath[PATH_MAX];
    double iMilSec;

}MyThreadStruct;

typedef struct 
{
    char chFile[PATH_MAX];

}FileData_t;

typedef struct 
{
    char chPath[PATH_MAX];

}Temp_File_Data_t;

typedef struct{

    char word[WORD_SIZE];
    int number;

}Book;

/*----------------------------------------------------------------------------*/
//                                MY GLOBALS
/*----------------------------------------------------------------------------*/
Temp_File_Data_t tempFileArray[FILE_SIZE];
FileData_t dataFolderArray[FILE_SIZE_INFO];

int iCountForFolder = 0;
int iCountForFile = 0;

Book stGlobalArray[TOTAL_WORD_SIZE];

int iIndexTotalWord  = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int iFullWord = 0;

int iClientPoints = 0;

char strLogFile[S_LOG_NAME_MAX];//Log file
char strRandomFIFO[FIFO_SIZE];
char strSignal[FIFO_SIZE];
char MAIN_FIFO[MAIN_FIFO_SIZE];
struct timeval tBegin;
int iClientFIFo;
int fdLogFile;
char strLogFile[S_LOG_NAME_MAX];
pid_t pidParent;
int iRndomFifo;

int ClientStatusID;
int *ClientStatus;
int iSharedIdForPath;
int iSharedIdForWord;
int iSharedIdClientRequest;
InfoClient_t *iRequestNumber;
Path_t *stFilePath;
Words_t *stWordsArr;
int iSizePathID;
int iSizeWordsID;
int *iPathTSize;
int *iWordSize;
sem_t *sem1;
sem_t *sem2;
int count = 0;

Book*ptrsTemp;
MyThreadStruct *stThread;
char strLogMsg[BUFSIZ];
int iMinerWorkedFiles;
/*----------------------------------------------------------------------------*/
//                                MY FUNCTIONS
/*----------------------------------------------------------------------------*/
int MyUsage(int argc,const char* argv[]);
void fnIsDirOrFileAndFilld(char path[]);
int fnIsDirectory(char *path);
int getPathFileInDirectory(char const root[]);
void fnAllocate(char**ptr,int size);
void fnMyFree(Book*ptrsBook,int size);
int fnCountWordsInTheFile(FILE*pfFilename);
int fnIsWord(const char*cptrWord,int iWordSize);
int fnGetLine(char*cptrLine,int iLineSize);
int fnAddWordToStruct(Book*ptrsBook,char*cptrLine,int *iIndex);
Book*fnFindWordsInTheFile(FILE*pfFilename,Book*ptrsBook,int*iptrSize);
int fnTheSameTwoString(char*ptrStr,char*ptrToken,int iStrSize,int iTokenSize);
int fnSearchWordInTheStruct(Book*ptrsBook,char*ptrSearchStr,int iSize,int*iptrWordCount,int iStart);
Book* fnFindTotalWordsIntheFile(char*pfFilename,int *iptrIndex);
pid_t fnWaitChilds(int*iptrStatLoc);
char*fnIgnoreBackSlashN(char *cptrStr,int iNumberSize);
Book*fnCombineAllOfWords(Book *ptrsBook,int iNumberSize,int *iptrNewsize);
int fnGetWordNumber(Book*ptrsBook,char*ptrSearchStr,int iSize,int*iptrWordCount,int iStart);
void*fnReturnWordCount(void*arg);
void fnGivePoints(Book*book,Words_t *stWordsArr,int *sizeServerWord,int sizeClientWord);
static void fnExitHandler(int iSigNum);
static void fnSigIntHandler(int iSigNum);
long fnTimeDiff(struct timeval tBegin, struct timeval tEnd);
/*----------------------------------------------------------------------------*/
//                                MAIN FUNCTION 
/*----------------------------------------------------------------------------*/
int main(int argc, char const *argv[])
{	
	
    int i;
    time_t tRawFormat;
    struct timeval startTime, endTime;
    double miliSecTime;
    
    pidParent = getpid();
    
    gettimeofday(&startTime, NULL);
    if ( MyUsage(argc,argv) == ERROR ) 
        return ERROR;
    
    int iRequestSize = atoi(argv[1]);
    if (iRequestSize <= 0)
    {
        printf("\nERROR:Argument is negative!! : %s\n\n",argv[1]);
        return ERROR;

    }

    sprintf(strLogFile, "%s%d%s", "MinerLogFile_", pidParent, ".txt");
  
    fdLogFile = open(strLogFile, O_WRONLY | O_APPEND | O_CREAT,
            S_IRUSR | S_IWUSR);

    sprintf(strLogMsg, "INFO: Starting client...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    time(&tRawFormat);
    sprintf(strLogMsg, "INFO: Starting Time: %s\n", ctime(&tRawFormat));
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));
    //create semaphore
    if ((sem1 = sem_open(SEMA_NAME1, 0)) == (sem_t *) -1) {
        fprintf(stderr, "Cannot create semaphore1!..\n");
        exit(1);
    }
    sprintf(strLogMsg, "INFO: connecting semaphore1...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    if ((sem2 = sem_open(SEMA_NAME2, 0)) == (sem_t *) -1) {
        fprintf(stderr, "Cannot create semaphore2!..\n");
        exit(1);
    }

    sprintf(strLogMsg, "INFO: connecting semaphore2...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    for (i = 0; i < FILE_SIZE_INFO; ++i)
    {
        memset(dataFolderArray[i].chFile,'\0',PATH_MAX);
    }
    for (i = 0; i < FILE_SIZE; ++i)
    {
        memset(tempFileArray[i].chPath,'\0',PATH_MAX);
    }

    sprintf(strLogMsg, "INFO: initializing globals arrays...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));
    
     /*
     * Create the shared memory segment for client status.
     */
    if((ClientStatusID = shmget(KEY_T_FOR_STATUS,sizeof(int) * 1,0666)) < 0) {
        perror("Failed to create the shared memory segment for client status!!");
        exit(1);
    }
    if ((ClientStatus = shmat(ClientStatusID, NULL, 0)) == (int*) -1) {
        perror("Failed to attach the memory segment to our data space for client status!!");
        exit(1);
    }
    /*
     * Create the shared memory segment for words size.
     */
    if((iSizeWordsID = shmget(KEY_T_FOR_WORDS_SIZE,sizeof(int) * 1, 0666)) < 0) {
        perror("Failed to create the shared memory segment for words array!!");
        exit(1);
    }
    
     // Now we attach the segment to our data space for words size.
    
    if ((iWordSize = shmat(iSizeWordsID, NULL, 0)) == (int*) -1) {
        perror("Failed to attach the memory segment to our data space for words array!!");
        exit(1);
    }
    sprintf(strLogMsg, "INFO: connecting  shared memory for array size ...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    /*
     * Create the shared memory segment for path size.
     */
    if ((iSizePathID = shmget(KEY_T_FOR_PATH_SIZE,sizeof(int) * 1,0666)) < 0) {
        perror("Failed to create the shared memory segment for file path array!!");
        exit(1);
    }
    /*
     * Now we attach the segment to our data space for path size.
     */
    if ((iPathTSize = shmat(iSizePathID, NULL, 0)) == (int*) -1) {
        perror("Failed to attach the memory segment to our data space for file path array!!");
        exit(1);
    }
    sprintf(strLogMsg, "INFO: connecting  shared memory for path size ...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    /*
     * Create the shared memory segment for file path.
     */
    if ((iSharedIdForPath = shmget(KEY_T_FOR_PATH,sizeof(Path_t) * SHMSZ_FOR_FILE,0666)) < 0) {
        perror("Failed to create the shared memory segment for file path array!!");
        exit(1);
    }
    /*
     * Now we attach the segment to our data space for file path array.
     */
    if ((stFilePath = shmat(iSharedIdForPath, NULL, 0)) == (Path_t*) -1) {
        perror("Failed to attach the memory segment to our data space for file path array!!");
        exit(1);
    }
    sprintf(strLogMsg, "INFO: connecting  shared memory for file path array ...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

     /*
     * Create the shared memory segment for words array.
     */
    if ((iSharedIdForWord = shmget(KEY_T_FOR_WORD,sizeof(Words_t) * SHMSZ_FOR_WORD,0666)) < 0) {
        perror("Failed to create the shared memory segment for words array!!");
        exit(1);
    }
    /*
     * Now we attach the segment to our data space for words array.
     */
    if ((stWordsArr = shmat(iSharedIdForWord, NULL, 0)) == (Words_t*) -1) {
        perror("Failed to attach the memory segment to our data space for words array!!");
        exit(1);
    }
    sprintf(strLogMsg, "INFO: connecting  shared memory for word array ...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    /*
     * Create the shared memory segment for request number.
     */
    if ((iSharedIdClientRequest = shmget(KEY_T_CLIENT_REQUEST,sizeof(InfoClient_t) * 1,0666)) < 0) {
        perror("Failed to create the shared memory segment for request size!!");
        exit(1);
    }
    /*
     * Now we attach the segment to our data space for words array.
     *
    */
    if ((iRequestNumber = shmat(iSharedIdClientRequest, NULL, 0)) == (InfoClient_t*) -1) {
        perror("Failed to attach the memory segment to our data space for words array!!");
        exit(1);
    }
    sprintf(strLogMsg, "INFO: connecting  shared memory for request struct ...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    /*
     * Set signal handler for SIGINT
     */
    if (signal(SIGINT, fnSigIntHandler) == SIG_ERR ) {
        sprintf(strLogMsg, "ERROR: Unable to set signal handler for SIGINT\n");
        write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
        memset(strLogMsg, '\0', BUFSIZ * sizeof(char));
        fnExitHandler(EXIT_SIGHANDLER_FAIL);
    }

    if (gettimeofday(&tBegin, NULL )) {
        sprintf(strLogMsg, "ERROR: Failed to get starting time.\n");
        write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
        memset(strLogMsg, '\0', BUFSIZ * sizeof(char));
        fnExitHandler(2);
    }

    sprintf(strLogMsg, "INFO: setting  CTRL_C signal...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    int iTemp = *iPathTSize;
    sprintf(strLogMsg, "INFO: sending  request for file size...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));
    
    //sending pid o server
    iRequestNumber->iClientID = getpid();
    printf("Connecting to mine...\n");
    
    //Bekleme sırasına giriyor
    *ClientStatus = 1;  
    
    /*Controlling mine size*/
    if (*iPathTSize <=0)
    {
        sprintf(strLogMsg, "INFO: mine has not....\n");
        printf("mine has not!!\n");
        write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
        memset(strLogMsg, '\0', BUFSIZ * sizeof(char));
        return ERROR;
    }
    /*--------------------------------------------------------------------------*/
    /**************************** CRITIC SECTION1 START *******************************/
    /*--------------------------------------------------------------------------*/
    sem_wait(sem1);
    /*istek madenin sayısından kucuk ise madenci istediği kadar alacak*/
    /*istek madenin sayısından buyuk ise madenin sayısı kadar maden alacak*/
    time(&tRawFormat);
    sprintf(strLogMsg, "INFO: Starting Time: %s", ctime(&tRawFormat));
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));
    
    if (iRequestSize <= *iPathTSize)
    {   
        for (i = 0; i < iRequestSize; ++i)
        {
            fnIsDirOrFileAndFilld(stFilePath[--iTemp].chPath);
            memset(stFilePath[iTemp].chPath,'\0',PATH_MAX);
        }   
        
    }else{

        for (i = 0; i < *iPathTSize; ++i)
        {
            fnIsDirOrFileAndFilld(stFilePath[--iTemp].chPath);
            memset(stFilePath[iTemp].chPath,'\0',PATH_MAX);
        }  
    }

    sprintf(strLogMsg, "INFO: receiving files...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));
    *iPathTSize = iTemp;
    
    /*Klssör sayısı alınıyor*/
    for (i = 0; i < iCountForFolder; ++i)
    {
        strcpy(stFilePath[iTemp++].chPath,dataFolderArray[i].chFile);
    }
    sprintf(strLogMsg, "INFO: sending paths in the folders...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    *iPathTSize = iTemp;
    sem_post(sem1);
    sem_close(sem1);
    /*--------------------------------------------------------------------------*/
    /**************************** CRITIC SECTION1 END ****************************/
    /*--------------------------------------------------------------------------*/

    //aldığı madenlerden hic dosya çıkmadığında islem yapmaz ve puan almaz
    iMinerWorkedFiles = iCountForFile;
    if (iCountForFile <=0)
    {   
        printf("I cannot get mine!!\n");
        sprintf(strLogMsg, "INFO:Something came out of the treasure..No File ...\n");
        write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
        memset(strLogMsg, '\0', BUFSIZ * sizeof(char));
        sprintf(strLogMsg, "INFO: Don't get money..\n");
        write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
        memset(strLogMsg, '\0', BUFSIZ * sizeof(char));
        return 0;
    }
    sprintf(strLogMsg, "INFO: starting threads for files...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    sprintf(strLogMsg, "INFO: searching words in files...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    /*--------------------------------------------------------------------------*/
    /**************************** CRITIC SECTION2 START ****************************/
    /*--------------------------------------------------------------------------*/
    sem_wait(sem2);
    iRequestNumber->iFileNumber = 1;
    
    stThread = (MyThreadStruct*)malloc(sizeof(MyThreadStruct) * iCountForFile);
    pthread_t thread[iCountForFile];
    int iError;
    for ( i = 0; i < iCountForFile; ++i)
    {   
        memset(stThread[i].chFilePath,'\0',PATH_MAX * sizeof(char));
        strcpy(stThread[i].chFilePath,tempFileArray[i].chPath);     
    }
    for (i = 0; i < iCountForFile; ++i)
    {
        if (iError = pthread_create(&thread[i],NULL,fnReturnWordCount,&stThread[i]))
        {
          fprintf(stderr, "Failed to create thread %d: %s\n",i+1,strerror(iError));
        }
       
    }
    for (i = 0; i < iCountForFile; ++i)
    {   
        if (iError = pthread_join(thread[i],NULL))
        {   
            fprintf(stderr, "Failed to join thread\n");
            continue;
        }     
    }
    
    int iNumber = 0;
    int iTotalWordsNumber = 0;
    ptrsTemp = fnCombineAllOfWords(stGlobalArray,iIndexTotalWord,&iNumber);

    sprintf(strLogMsg, "INFO: sending result:...\n\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));
    for (i = 0; i < iNumber; ++i)
    {       
        iTotalWordsNumber += ptrsTemp[i].number;
        
    }
    int tempSize = *iWordSize;
    fnGivePoints(ptrsTemp,stWordsArr,&tempSize,iNumber);
    *iWordSize = tempSize;
    iRequestNumber->iPoints += iClientPoints;

    gettimeofday(&endTime, NULL);
    miliSecTime = ((endTime.tv_sec  - startTime.tv_sec) * 1000 + 
                  (endTime.tv_usec - startTime.tv_usec)/1000.0) + 0.5;

    
    time(&tRawFormat);
    sprintf(strLogMsg, "INFO: Stoppiing Time: %s", ctime(&tRawFormat));
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    sprintf(strLogMsg, "INFO: number of files that the miner worked on: %d",iMinerWorkedFiles);
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    sprintf(strLogMsg, "\nMY EXECUTION TIME: %.3lf  miliseconds\n", miliSecTime);
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    sprintf(strLogMsg, "\nINFO: TOTAL UNIQUE WORDS NUMBER:%d\n",iNumber);
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    sprintf(strLogMsg, "\nINFO: TOTAL WORDS NUMBER:%d\n",iTotalWordsNumber);
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    sprintf(strLogMsg, "\nINFO: TOTAL MONEY:%d  TL\n ",iClientPoints);
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    sprintf(strLogMsg, "\nI did all my work. I went out.\n ");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    printf("\n\tMONEY:%d  TL\n",iClientPoints);
    printf("\n\tTOTAL UNIQUE WORDS NUMBER : %d\n",iNumber);
    printf("\n\tTOTAL WORDS NUMBER : %d\n",iTotalWordsNumber);     
    sem_post(sem2);
    sem_close(sem2);
    *ClientStatus = 3;
    /*--------------------------------------------------------------------------*/
    /**************************** CRITIC SECTION2 END ****************************/
    /*--------------------------------------------------------------------------*/
    free(ptrsTemp);
    free(stThread);
    
    shmdt(stFilePath);

    shmdt(stWordsArr);

    shmdt(iRequestNumber);

    shmdt(iPathTSize);

    shmdt(iWordSize);

    close(fdLogFile);
	
    return (EXIT_SUCCESS);
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "fnGivePoints"
// Parameters     : <book> : global word array
//                  <stWordsArr> : words array in the shared memory
//                  <sizeServerWord>: size of array
//                   <sizeClientWord> : size of array   
// Description    : Checks the correctness of the parameters
//
/*----------------------------------------------------------------------------*/
void fnGivePoints(Book*book,Words_t *stWordsArr,int *sizeServerWord,int sizeClientWord){

    int i;
    int j;
    int iStatus = 0;
    int iTemp = *sizeServerWord;
    int iTempClient =  sizeClientWord;
    int iTempCount = *sizeServerWord;
    int temp = 0;
    for (i = 0; i < iTempClient; ++i)
    {
        for (j = 0; j < iTemp; ++j)
        { 
            if (strcmp(stWordsArr[j].chWord,book[i].word) == 0)
            {   
                temp  = book[i].number * 1;
                iClientPoints +=temp;
                iStatus = 1;
                stWordsArr[j].iWordNumber += book[i].number; 
                sprintf(strLogMsg, "word :%s Number: %d  Money: %d\n",book[i].word,book[i].number,temp);
                write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
                memset(strLogMsg, '\0', BUFSIZ * sizeof(char));
            }
        }
        if (iStatus == 0)
        {   
            strcpy(stWordsArr[iTempCount++].chWord,book[i].word);
            temp = book[i].number * 10;
            iClientPoints +=temp;
            stWordsArr[iTempCount].iWordNumber = book[i].number;
            sprintf(strLogMsg, "word :%s Number: %d  Money: %d\n",book[i].word,book[i].number,temp);
            write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
            memset(strLogMsg, '\0', BUFSIZ * sizeof(char));
        }
        iStatus = 0;
    }
    *sizeServerWord = iTempCount;
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
        fprintf(stdout,"\n\n\t Usage: %s [N]\n\n  %s\n\n\n",
                        argv[0],
                        "\n\n\tThis program is listing files from the given directory and compare words and add words.");
        fflush(stdout);
        return -1;
    }
    return TRUE;
}
void fnIsDirOrFileAndFilld(char path[]){


    if (fnIsDirectory(path))
    {
        getPathFileInDirectory(path);
    }else{
        strcpy(tempFileArray[iCountForFile++].chPath,path);
    }
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
int getPathFileInDirectory(char const root[]) {
   
    struct dirent *direntp;
    DIR    *dirp;
   
    if ((dirp = opendir(root)) == NULL) {
        fprintf (stderr, "Failed to open directory \"%s\" : %s\n",
                 direntp->d_name, strerror(errno));
        return 1;
    } 
    while ((direntp = readdir(dirp)) != NULL) {

        char newPath [PATH_MAX] = "";
        char newRoot [PATH_MAX] = "";

        if(direntp->d_name[0]=='.') continue;
        if(direntp->d_name[strlen(direntp->d_name)-1] == '~') continue;

        strcat(newPath, direntp->d_name);
        strcat(newRoot, root);
        strcat(newRoot, "/");
        strcat(newRoot, direntp->d_name);
        strcpy(dataFolderArray[iCountForFolder++].chFile,newRoot);
    }
    while ((closedir(dirp) == -1) && (errno == EINTR)); // force to close directory

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

    pthread_mutex_lock(&mutex);
    struct timeval startTime1,endTime1;
    double miliSecTime;
   
    gettimeofday(&startTime1, NULL);
    
    int index;
    MyThreadStruct *stTemp = (MyThreadStruct*)arg;
    Book*book = fnFindTotalWordsIntheFile(stTemp->chFilePath,&index);

    int i;
    for (i = 0; i < index; ++i){
        
        memset(stGlobalArray[iIndexTotalWord].word,'\0',sizeof(char) * WORD_SIZE);
        strcpy(stGlobalArray[iIndexTotalWord].word,fnIgnoreBackSlashN(book[i].word,strlen(book[i].word)));
        stGlobalArray[iIndexTotalWord].number = book[i].number;
        ++iIndexTotalWord;
    }
    gettimeofday(&endTime1, NULL);
    miliSecTime = ((endTime1.tv_sec  - startTime1.tv_sec) * 1000 + 
                  (endTime1.tv_usec - startTime1.tv_usec)/1000.0) + 0.5;
    stTemp->iMilSec = miliSecTime;
    free(book);
    pthread_mutex_unlock(&mutex);
    return (void*)1;
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
        
        if (fnSearchWordInTheStruct(ptrsTemp,stGlobalArray[i].word,iTempCount,&iIndex1,0) == FALSE)
        {   
            iIndex1 = 0;
            fnGetWordNumber(stGlobalArray,stGlobalArray[i].word,iNumberSize,&iIndex1,i);
            strcpy(ptrsTemp[iTempCount].word,stGlobalArray[i].word);
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
//                  <iptrIndex> : struct index size
// Description    : arrange struct for all words in the file 
//
/*----------------------------------------------------------------------------*/
Book* fnFindTotalWordsIntheFile(char*pfFilename,int *iptrIndex){
    
    FILE *pfInputFile;
    if(( pfInputFile = fopen(pfFilename,"r")) == NULL){
         fprintf(stderr,"Failed to open file for reading: %s\n",pfFilename);
         return NULL;
    }
    Book*ptrsBook;
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
            // strcpy(stGlobalArray[iIndexTotalWord].word,ptrTemp[i].word);
            // stGlobalArray[iIndexTotalWord].number = ptrTemp[i].number;
            // ++iIndexTotalWord;
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
//                  <iSize> : struct size
//                  <iptrWordCount> : total word size in the struct
//                  <iStart> : starter value
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
//                  <iptrSize> : source size
//                  <iTokenSize> : target size
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
//                  <iptrSize> : struct size
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
pid_t fnWaitChilds(int*iptrStatLoc)
{
    int iRetval;

    while (((iRetval = wait(iptrStatLoc)) == -1) && (errno == EINTR));
    return iRetval;
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "fnExitHandler"
// Parameters     : <iSigNum> : num of program's parameters
//           
// Description    : when it catched the signal,it handle things.
//
/*----------------------------------------------------------------------------*/
static void fnExitHandler(int iSigNum) {
    
    struct timeval tEnd;
    long lliElapsedTime = 0;
    char strLogMsg[BUFSIZ];
    int iCntr = 0;
    int iStat;
    time_t tRawFormat;

    if (gettimeofday(&tEnd, NULL )) {
        sprintf(strLogMsg, "ERROR: Failed to get ending time.\n");
        write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
        memset(strLogMsg, '\0', BUFSIZ * sizeof(char));
    }
    free(ptrsTemp);
    free(stThread);

    shmdt(stFilePath);

    shmdt(stWordsArr);

    shmdt(iRequestNumber);
    shmdt(iPathTSize);

    shmdt(iWordSize);
   
    sem_close(sem1);
    sem_close(sem2);

    sprintf(strLogMsg, "INFO: Sent Shutdown signal to me...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    sprintf(strLogMsg, "INFO: deataching all shared memory...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    sprintf(strLogMsg, "INFO: closing all semaphore...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    sprintf(strLogMsg, "INFO: free all dynamic allocation...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    sprintf(strLogMsg, "INFO: I'm died...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    lliElapsedTime = fnTimeDiff(tBegin, tEnd);

    sprintf(strLogMsg, "INFO: MY EXECUTION TIME: %ld msec(s)\n", lliElapsedTime);
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    sprintf(strLogMsg, "INFO: number of files that the miner worked on: %d\n",iMinerWorkedFiles);
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    sprintf(strLogMsg, "INFO: I Cannot get money...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    sprintf(strLogMsg, "INFO: CTRL-C KILLED ME...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    time(&tRawFormat);
    sprintf(strLogMsg, "INFO: End Time: %s", ctime(&tRawFormat));
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    sprintf(strLogMsg, "----   END OF LOG FILE   ----\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));
    close(fdLogFile);

    system(strLogMsg);
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    exit(iSigNum);
}
long fnTimeDiff(struct timeval tBegin, struct timeval tEnd) {
    return ((MILLION * (tEnd.tv_sec - tBegin.tv_sec))
            + (tEnd.tv_usec - tBegin.tv_usec));
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "fnSigIntHandler"
// Parameters     : <iSigNum> : integer value
//             
// Description    : cathes CTRL_C signal
//
/*----------------------------------------------------------------------------*/
static void fnSigIntHandler(int iSigNum) {
    char strLogMsg[BUFSIZ];

    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    if (pidParent != getpid()) {
        kill(getppid(), SIGINT);
        return;
    }

    printf("INFO: SIGINT caught, exiting properly...\n");
    fflush(stdout);

    sprintf(strLogMsg, "INFO: SIGINT caught, exiting properly...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    fnExitHandler(EXIT_SIGINT);
}
/* End of miner.c */