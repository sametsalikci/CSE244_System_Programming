
/*----------------------------------------------------------------------------*/
//
//  Author        : Mehmet Mazhar SALIKCI
//	number		  : 111044050
//  Date          : May 19,2015
//  FileName      : "mine.c"
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

# define RESULT_SIZE 100
# define OP_SIZE 12
# define PAR_SIZE 12
# define FIFO_SIZE 12
# define MAX_CHILD 1000
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
# define KEY_T_FOR_WORD  12342323
# define KEY_T_FOR_PATH  21405511
# define KEY_T_FOR_PATH_SIZE 12371212
# define KEY_T_FOR_WORDS_SIZE 12381313
# define KEY_T_FOR_STATUS 22222222

# define SHMSZ_FOR_FILE  2048
# define SHMSZ_FOR_WORD  50000

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

//For client pids
typedef struct {
    pid_t pidClients[MAX_CLIENTS];
    int iCurrrent;
    int iSize;
} CLIENTLIST_T;

/*----------------------------------------------------------------------------*/
//                                MY GLOBALS
/*----------------------------------------------------------------------------*/
int iCount = 0;
char strLogFile[S_LOG_NAME_MAX];
CLIENTLIST_T clClients;
struct timeval tBegin;
int iServerFIFO;
int fdLogFile;
char strLogFile[S_LOG_NAME_MAX];
pid_t pidParent;
char MAIN_FIFO[MAIN_FIFO_SIZE];
pid_t pidChildList[MAX_CHILD];
int iNumOfChilds;


int ClientStatusID;
int *ClientStatus;

char strLogMsg[BUFSIZ];

int iSharedIdForPath;
int iSharedIdForWord;
int iSharedIdClientRequest;
int iSizePathID;
int iSizeWordsID;
InfoClient_t *iRequestNumber;
Path_t *stFilePath;
Words_t *stWordsArr;
int *iPathTSize;
int *iWordSize;
sem_t *sem1;
sem_t *sem2;
int iTotalPoints = 0;
/*----------------------------------------------------------------------------*/
//                                MY FUNCTIONS
/*----------------------------------------------------------------------------*/
int MyUsage(int argc,const char* argv[]);
int getPathFileInDirectory(char const root[],Path_t*stFile);
void fnListPathInDirectories(char const **argv,int argc,Path_t*stFile);
static void fnExitHandler(int iSigNum) ;
static void fnSigIntHandler(int iSigNum) ;
pid_t fnWaitChilds(int* iptrStatLoc);
void fnCheckChilds(void);
int fnIsKnownClient(const pid_t pidClient);
long fnTimeDiff(struct timeval tBegin, struct timeval tEnd);
void* getPoint(void* arg);
/*----------------------------------------------------------------------------*/
//                                MAIN FUNCTION 
/*----------------------------------------------------------------------------*/
int main(int argc, char const *argv[])
{	
	
    int pidChildPid;
    char*cptrTempFifoName;
    
    char cClientCommand = '\0';
    time_t tRawFormat;
    pid_t pidServingChild = 0;
    pid_t pidParentOfClient = 0;

    pidParent = getpid();
    iNumOfChilds = 0;
    clClients.iSize = MAX_CLIENTS;
    clClients.iCurrrent = 0;
    pidParent = getpid();
    int i;

    if ( MyUsage(argc,argv) == ERROR ) 
        return 0;

    sprintf(strLogFile, "%s%d%s", "MineLogFile_", pidParent, ".txt");
  
    fdLogFile = open(strLogFile, O_WRONLY | O_APPEND | O_CREAT,
            S_IRUSR | S_IWUSR);

    //Create semaphore1
    if ((sem1 = sem_open(SEMA_NAME1,O_CREAT, 0644,1)) == (sem_t *) -1) {
        fprintf(stderr, "Cannot create semaphore1!..\n");
        exit(1);
    }
    sprintf(strLogMsg, "INFO: creating semafor1:...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    //Create semaphore1
    if ((sem2 = sem_open(SEMA_NAME2,O_CREAT, 0644, 1)) == (sem_t *) -1) {
        fprintf(stderr, "Cannot create semaphore2!..\n");
        exit(1);
    }

    sprintf(strLogMsg, "INFO: creating semafor2:...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));
     
    /*
     * Create the shared memory segment for client status.
     */
    if((ClientStatusID = shmget(KEY_T_FOR_STATUS,sizeof(int) * 1, IPC_CREAT | 0666)) < 0) {
        perror("Failed to create the shared memory segment for client status!!");
        exit(1);
    }
    if ((ClientStatus = shmat(ClientStatusID, NULL, 0)) == (int*) -1) {
        perror("Failed to attach the memory segment to our data space for client status!!");
        exit(1);
    }
    *ClientStatus = 0;
     /*
     * Create the shared memory segment for words size.
     */
    if((iSizeWordsID = shmget(KEY_T_FOR_WORDS_SIZE,sizeof(int) * 1, IPC_CREAT | 0666)) < 0) {
        perror("Failed to create the shared memory segment for file path array!!");
        exit(1);
    }
    /*
     * Now we attach the segment to our data space for words size.
    */
    if ((iWordSize = shmat(iSizeWordsID, NULL, 0)) == (int*) -1) {
        perror("Failed to attach the memory segment to our data space for file path array!!");
        exit(1);
    }
    sprintf(strLogMsg, "INFO: creating shared memory for words array size variable:...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));
    *iWordSize = 0;

     /*
     * Create the shared memory segment for path size.
     */
    if ((iSizePathID = shmget(KEY_T_FOR_PATH_SIZE,sizeof(int) * 1, IPC_CREAT | 0666)) < 0) {
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
    *iPathTSize = 0;
    sprintf(strLogMsg, "INFO: creating shared memory for file paths array size variable:...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    /*
     * Create the shared memory segment for file path.
     */
    if ((iSharedIdForPath = shmget(KEY_T_FOR_PATH,SHMSZ_FOR_FILE * sizeof(Path_t), IPC_CREAT | 0666)) < 0) {
        perror("Failed to create the shared memory segment for file path array!!");
        exit(1);
    }
    /*
     * Now we attach the segment to our data space for file path array.
     */
    if ((stFilePath = (Path_t*)shmat(iSharedIdForPath, NULL, 0)) == (Path_t*) -1) {
        perror("Failed to attach the memory segment to our data space for file path array!!");
        exit(1);
    }
    sprintf(strLogMsg, "INFO: creating shared memory for file paths array:...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));
     /*
     * Create the shared memory segment for words array.
     */
    if ((iSharedIdForWord = shmget(KEY_T_FOR_WORD,sizeof(Words_t) * SHMSZ_FOR_WORD, IPC_CREAT | 0666)) < 0) {
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
    sprintf(strLogMsg, "INFO: creating shared memory for words array:...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));
    /*
     * Create the shared memory segment for request number.
     */
    if ((iSharedIdClientRequest = shmget(KEY_T_CLIENT_REQUEST,sizeof(InfoClient_t) * 1, IPC_CREAT |0666)) < 0) {
        perror("Failed to create the shared memory segment for request size!!");
        exit(1);
    }
    sprintf(strLogMsg, "INFO: creating shared memory for client file request <n> varieble:...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));
    /*
     * Now we attach the segment to our data space for words array.
     *
    */
    if ((iRequestNumber = shmat(iSharedIdClientRequest, NULL, 0)) == (InfoClient_t*) -1) {
        perror("Failed to attach the memory segment to our data space for words array!!");
        exit(1);
    }

    sprintf(strLogMsg, "INFO: creating shared memory for client file request <n> varieble:...\n");
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
    sprintf(strLogMsg, "INFO: setting CTRL_C signal:...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));
    

    time(&tRawFormat);
    sprintf(strLogMsg, "INFO: Starting Time: %s", ctime(&tRawFormat));
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    sprintf(strLogMsg, "INFO: watting for clients:\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));
    
    fnListPathInDirectories(argv,argc,stFilePath);
    *iPathTSize = iCount;
    int status = 0;
    printf("\nReady to serve...waitting\n\n");
    int tempID = 0;
    iRequestNumber->iFileNumber = -1;

    while(1){
         
        if (*ClientStatus > 0)
        {   
            sprintf(strLogMsg, "\n\n");
            write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
            memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

            printf("Connected child:%d\n",iRequestNumber->iClientID);
            sprintf(strLogMsg, "INFO: Connected client:...%d\n",iRequestNumber->iClientID);
            write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
            memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

            *ClientStatus = 0;

            printf("Ready to serve...waitting\n\n");

            if(!fnIsKnownClient(iRequestNumber->iClientID)) {
                /* Add new client to list */
                clClients.pidClients[clClients.iCurrrent] = iRequestNumber->iClientID;
                (clClients.iCurrrent) = ((clClients.iCurrrent) + 1);
            }
            if (iNumOfChilds < MAX_CHILD) {
                pidChildList[iNumOfChilds] = iRequestNumber->iClientID;;
                ++iNumOfChilds;

                sprintf(strLogMsg,
                        "INFO: Client Connected: serving pid: %ld client pid:%d\n",
                        (long)getpid(), iRequestNumber->iClientID);

                write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
                sprintf(strLogMsg, "INFO: calculating results...%d\n",iRequestNumber->iClientID);
                write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
                memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

                sprintf(strLogMsg, "INFO: I paid Money to miner : %d \n",iRequestNumber->iClientID);
                write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
                memset(strLogMsg, '\0', BUFSIZ * sizeof(char));
            
                
                sprintf(strLogMsg, "INFO:Sending results... -> client : %d\n",iRequestNumber->iClientID);
                write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
                memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

            } else {

                sprintf(strLogMsg,"ERROR: Too many cliend to handle. Killing last one...\n");
                write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
                kill(iRequestNumber->iClientID, SIGINT);
            } 
        }
        sleep(1);  
    }
	return (EXIT_SUCCESS);
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "fnListPathInDirectories"
// Parameters     : <argv> : path from linux root
//                  <argc> : path from current working directory
//                  <stFile> : path from current working directory
// Description    : Listing the files from directory with subdirectories
//                  multi-process funciton
//
/*----------------------------------------------------------------------------*/
void fnListPathInDirectories(char const **argv,int argc,Path_t *stFile){

	int i;
	
	for (i = 1; i < argc; ++i){
		char root[PATH_MAX]={0x0};
		char path[PATH_MAX]={0x0};
		getcwd(root, PATH_MAX);
		strcpy(path, root);
	    strcat(root,"/");
	    strcat(root,argv[i]);
	    getPathFileInDirectory(root,stFile);
	}
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
int getPathFileInDirectory(char const root[],Path_t*stFile) {
   
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
        //strcat(newRoot, "/");
        strcat(newRoot, direntp->d_name);
        memset(stFile[iCount].chPath,'\0',PATH_MAX);
        strncpy(stFile[iCount++].chPath,newRoot,PATH_MAX);
    }
    while ((closedir(dirp) == -1) && (errno == EINTR)); // force to close directory

    return 0;
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
    if(argc < 2 || argc > 21)
    {
        fprintf(stdout,"\n\n\t Usage: %s [Dir1] [Dir2] ... [DirN]\n\n  %s\n  %s\n\n",
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

    sprintf(strLogMsg, "\nINFO: deataching all shared memory...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    sprintf(strLogMsg, "INFO: removing all shared memory...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));
    
    sem_close(sem1);
    sem_close(sem2);

    sprintf(strLogMsg, "INFO: closing all semaphore...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    sem_unlink(SEMA_NAME1);
    sem_unlink(SEMA_NAME2);

    sprintf(strLogMsg, "INFO: removing all semaphore...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    sprintf(strLogMsg, "INFO: Sending Shutdown signal to clients...\n");
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    sprintf(strLogMsg, "INFO: Number of miner served: %d\n",clClients.iCurrrent);
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    for (iCntr = 0; iCntr < clClients.iCurrrent; ++iCntr) {
        if(clClients.pidClients[iCntr] > 0)
            kill(clClients.pidClients[iCntr], SIGINT);
    }

    lliElapsedTime = fnTimeDiff(tBegin, tEnd);

    int i;
    for ( i = 0; i < *iWordSize; ++i)
    { 
        sprintf(strLogMsg, "Word :%s  Number: %d\n",stWordsArr[i].chWord,stWordsArr[i].iWordNumber);
        write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
        memset(strLogMsg, '\0', BUFSIZ * sizeof(char));
    }
    sprintf(strLogMsg, "INFO: EXECUTION TIME: %ld msec(s)\n", lliElapsedTime);
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    sprintf(strLogMsg, "\nINFO: TOTAL WORDS: %d\n",*iWordSize);
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    sprintf(strLogMsg, "\nINFO: PAID TOTAL MONEY: %d  TL\n\n",iRequestNumber->iPoints);
    write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
    memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

    shmdt(stFilePath);
    shmctl(iSharedIdForPath, IPC_RMID, NULL);

    shmdt(iRequestNumber);
    shmctl(iSharedIdClientRequest, IPC_RMID, NULL);

    shmdt(iPathTSize);
    shmctl(iSizePathID, IPC_RMID, NULL);

    shmdt(iWordSize);
    shmctl(iSizeWordsID, IPC_RMID, NULL);

    shmdt(ClientStatus);
    shmctl(ClientStatusID, IPC_RMID, NULL);

    shmdt(stWordsArr);
    shmctl(iSharedIdForWord, IPC_RMID, NULL);

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
/*----------------------------------------------------------------------------*/
//
// Function Name  : "fnWaitChilds"
// Parameters     : <iptrStatLoc> : child value
//             
// Description    :waits all child proccess to terminate
//
/*----------------------------------------------------------------------------*/
pid_t fnWaitChilds(int* iptrStatLoc) {
    int iRetval;

    while (((iRetval = wait(iptrStatLoc)) == -1) && (errno == EINTR))
        ;
    return iRetval;
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "fnCheckChilds"
// Parameters     : no parameter
//             
// Description    : checks whether there is a child in childlist,if there is not alive,removes them
//
/*----------------------------------------------------------------------------*/
void fnCheckChilds(void) {
    int iCntr = 0;
    int iStat;

    for (iCntr = 0; iCntr < iNumOfChilds; ++iCntr) {
        waitpid(pidChildList[iCntr], &iStat, WNOHANG);
        if (WIFEXITED(iStat)) {
            if (MAX_CLIENTS > iCntr) {
                if (MAX_CLIENTS == (iCntr + 1)) {
                    pidChildList[iCntr] = 0;
                } else {
                    pidChildList[iCntr] = pidChildList[iCntr + 1];
                }
            }
        }
    }
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "fnIsKnownClient"
// Parameters     : <pidClient> : client pid id
//             
// Description    : Checks given lient pid in clCleints list. 
//                  If client pid exists returns 1,* otherwise 0.
//
/*----------------------------------------------------------------------------*/
int fnIsKnownClient(const pid_t pidClient) {
    int iCurr = clClients.iCurrrent;
    int iCntr = 0;

    for (iCntr = 0; iCntr < iCurr; ++iCntr) {
        if (pidClient == clClients.pidClients[iCntr]) {
            return 1;
        }
    }
    return 0;
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "fnIsKnownClient"
// Parameters     : <tBegin> : start time value
//                  <tEnd> : end time value
//             
// Description    : Returns difference between tBegin and tEnd
//
/*----------------------------------------------------------------------------*/
long fnTimeDiff(struct timeval tBegin, struct timeval tEnd) {
    return ((MILLION * (tEnd.tv_sec - tBegin.tv_sec))
            + (tEnd.tv_usec - tBegin.tv_usec));
}
/* End of mine.c */