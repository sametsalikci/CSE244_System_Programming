/*----------------------------------------------------------------------------*/
//
//  Author        : Mehmet Mazhar SALIKCI
//	number		  : 111044050
//  Date          : April 15,2015
//  FileName      : "client.c"
//                
//  Description   : Bu program servera belli operationlar icin istek yollar.Servera
//                  istekleri yolladıktan sonra,server ın yolladaığı işlemlerin 
//					sonuclarını ekrana ve logfile yazar.
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
#include <math.h>

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
# define MAIN_FIFO_SIZE 50
//# define MAIN_FIFO "mainFIFO"
# define MAX_CHILD 100
# define S_LOG_NAME_MAX 30
# define EXIT_SIGINT 3
# define EXIT_FORK_FAIL 6
# define MILLION 1000000L

# define FIFO_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
/*----------------------------------------------------------------------------*/
//                                MY GLOBALS
/*----------------------------------------------------------------------------*/
char strLogFile[S_LOG_NAME_MAX];
char strRandomFIFO[FIFO_SIZE];
char strSignal[FIFO_SIZE];
char MAIN_FIFO[MAIN_FIFO_SIZE];
struct timeval tBegin;
int iClientFIFo;
int fdLogFile;
char strLogFile[S_LOG_NAME_MAX];
pid_t pidParent;
int iRndomFifo;

/*----------------------------------------------------------------------------*/
//                                MY STRUCT
/*----------------------------------------------------------------------------*/

//For Parameter
typedef struct{

	char chOper[OP_SIZE];
	int iPar[PAR_SIZE];
	int iSize;
	int iWaitSecond;
	pid_t pidClient;			
}Parameter;

//For operation result
typedef struct{

	char chRes[6][RESULT_SIZE];
	int iResSize;

}CalcResult;
//For random fifo name
typedef struct{

	char chName[FIFO_SIZE];	

}RandomFifoName;

/*----------------------------------------------------------------------------*/
//                                MY FUNCTIONS
/*----------------------------------------------------------------------------*/
int MyUsage(int argc,const char* argv[]);
static void fnExitHandler(int iSigNum);
static void fnSigIntHandler(int iSigNum);
long fnTimeDiff(struct timeval tBegin, struct timeval tEnd);
/*----------------------------------------------------------------------------*/
//                                MAIN FUNCTION 
/*----------------------------------------------------------------------------*/
int main(int argc, char const *argv[])
{	
	//char const *cptrMainFIFO = argv[1];
	Parameter stPar;
	char strLogMsg[BUFSIZ];
	time_t tRawFormat;

	pidParent = getpid();

	if ( MyUsage(argc,argv) == ERROR ) 
    	return ERROR;
    int iTime = atoi(argv[2]);
    if ( iTime < 0 )
    {
    	fprintf(stderr, "ERROR: Time is negative >> %d\n", iTime);
    	exit(0);
    }
	memset(MAIN_FIFO,'\0',sizeof(char) * MAIN_FIFO_SIZE);
	strcpy(MAIN_FIFO,argv[1]);

    sprintf(strLogFile, "%s%d%s", "ClientLogFile_", pidParent, ".txt");
  
	fdLogFile = open(strLogFile, O_WRONLY | O_APPEND | O_CREAT,
			S_IRUSR | S_IWUSR);

	sprintf(strLogMsg, "INFO: Starting client...\n");
	write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
	memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

	time(&tRawFormat);
	sprintf(strLogMsg, "INFO: Starting Time: %s\n", ctime(&tRawFormat));
	write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
	memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

	sprintf(strLogMsg, "INFO: opening mainFIFO: %s\n", MAIN_FIFO);
	write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
	memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

	iClientFIFo = open(MAIN_FIFO,O_RDWR);
	if (iClientFIFo < 0)
	{
		perror("Failed to open mainFIFO");
		return ERROR;
	}
	memset(stPar.iPar,-1,sizeof(int) * PAR_SIZE);
    memset(stPar.chOper,'\0',sizeof(char) * OP_SIZE);
	strcpy(stPar.chOper,argv[3]);
	stPar.iWaitSecond = atoi(argv[2]);
	int i;
	memset(stPar.iPar,-1,PAR_SIZE);
	int j =0;
	for ( i = 4; i < argc; ++i)
	{
		stPar.iPar[j++] = atoi(argv[i]);
	}
	stPar.iSize = argc -4;
	stPar.pidClient = getpid();
	sprintf(strLogMsg, "INFO: writting parameters to: %s\n", MAIN_FIFO);
	write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
	memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

	write(iClientFIFo,&stPar,sizeof(Parameter)-1);
	RandomFifoName stFifoName;
	sprintf(strLogMsg, "INFO: writting parameters to: %s\n", MAIN_FIFO);
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
	while(1){
		if (mkfifo("signal",0666) == -1)
		{	
			strcpy(strSignal,"signal");
			unlink("signal");
			break;
		}
	}
	
	sprintf(strLogMsg, "INFO: reading random fifo name  from  mathServer\n");
	write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
	memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

	memset(stFifoName.chName,'\0',FIFO_SIZE);
	memset(strRandomFIFO,'\0',sizeof(char) * FIFO_SIZE);
	read(iClientFIFo,&stFifoName,sizeof(RandomFifoName)-1);
	strcpy(strRandomFIFO,stFifoName.chName);	
	iRndomFifo = open(stFifoName.chName,O_RDWR);
	if (iRndomFifo < 0)
	{
		perror("Failed to open RandomFifo");
	}

	sprintf(strLogMsg, "INFO: reading results from  mathServer\n");
	write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
	memset(strLogMsg, '\0', BUFSIZ * sizeof(char));
	
	CalcResult stRes;
	read(iRndomFifo,&stRes,sizeof(CalcResult));
	
	sprintf(strLogMsg, "INFO: writting results:\n\n");
	write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
	memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

	for (i = 0; i < stRes.iResSize; ++i)
	{
		printf("%s\n",stRes.chRes[i] );
		sprintf(strLogMsg, "INFO: %s\n",stRes.chRes[i]);
		write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
		memset(strLogMsg, '\0', BUFSIZ * sizeof(char));
	}
	sprintf(strLogMsg, "\nINFO: removing RandomFifo:\n");
	write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
	memset(strLogMsg, '\0', BUFSIZ * sizeof(char));
	unlink(stFifoName.chName);
	
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
    if(argc < 5 || argc > 16)
    {
        fprintf(stdout,"\n\n\t Usage: %s -<mainFifoName> -<waitingTime> -<operationName> -<parametre_1> ...-<parametre_k>;\n\n  %s\n  %s\n\n",
                        argv[0],
                        "This program is sendin request to server for calculation operation.",
                        "Then get the result and display on screen.");
        fflush(stdout);
        return -1;
    }
    return TRUE;
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

	close(iClientFIFo);
	close(iRndomFifo);
	unlink(strRandomFIFO);
	unlink(strSignal);

	sprintf(strLogMsg, "INFO: closed mainFIFO.\n");
	write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
	memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

	sprintf(strLogMsg, "INFO: closed RandomFifo.\n");
	write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
	memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

	sprintf(strLogMsg, "INFO: removed RandomFifo.\n");
	write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
	memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

	sprintf(strLogMsg, "INFO: Sending Shutdown signal to clients...\n");
	write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
	memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

	lliElapsedTime = fnTimeDiff(tBegin, tEnd);

	sprintf(strLogMsg, "INFO: Elapsed Time: %ld msec(s)\n", lliElapsedTime);
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

	sprintf(strLogMsg, "mv -f %s .", strLogFile);
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
/* End of client.c */