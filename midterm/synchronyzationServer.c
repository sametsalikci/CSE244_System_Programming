/*----------------------------------------------------------------------------*/
//
//  Author        : Mehmet Mazhar SALIKCI
//	number		  : 111044050
//  Date          : April 15,2015
//  FileName      : "synchronyzationServer.c"
//                
//  Description   : Bu program,4 farklı matemayiksel işlemi yapar,işlemleri yapmadan
//					önce clientan gelecek isteği dinler.Clientan gelen isteğe bağlı olarak
//                  işlemi yapmak için bir mathServer oluşturur,Bu mathServer,clientan gelen
//                  işlemin typına bağlı olarak sonucu hesaplar ve bunu geri clienta yollar.
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
# define MAX_CHILD 100
# define MAX_CLIENTS 100
# define S_LOG_NAME_MAX 30
# define EXIT_SIGINT 3
# define EXIT_FORK_FAIL 6
//# define MAIN_FIFO "mainFIFO"
# define MILLION 1000000L
# define MAIN_FIFO_SIZE 50

# define FIFO_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
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

//For operation Result
typedef struct{

	char chRes[6][RESULT_SIZE];
	int iResSize;

}CalcResult;

//For random fifo name
typedef struct{

	char chName[FIFO_SIZE];	

}RandomFifoName;
//For client pids
typedef struct {
	pid_t pidClients[MAX_CLIENTS];
	int iCurrrent;
	int iSize;
} CLIENTLIST_T;

/*----------------------------------------------------------------------------*/
//                                MY GLOBALS
/*----------------------------------------------------------------------------*/
static volatile int keepRunning = 1;
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
/*----------------------------------------------------------------------------*/
//                                MY FUNCTIONS
/*----------------------------------------------------------------------------*/
char*fnOperation1(int iNumA,int iNumB,int iNumC);
char*fnOperation2(int iNumA,int iNumB);
char*fnOperation3(int iNumA,int iNumB,int iNumC);
char*fnOperation4(int iNumA,int iNumB,int iNumC,int iNumD);
int fnCalculationOperation(Parameter stStruct,int iFifoFd);
char*fnCreateFifoName();
int MyUsage(int argc,const char* argv[]);
static void fnExitHandler(int iSigNum) ;
static void fnSigIntHandler(int iSigNum) ;
pid_t fnWaitChilds(int* iptrStatLoc);
void fnCheckChilds(void);
int fnIsKnownClient(const pid_t pidClient);
long fnTimeDiff(struct timeval tBegin, struct timeval tEnd);
/*----------------------------------------------------------------------------*/
//                                MAIN FUNCTION 
/*----------------------------------------------------------------------------*/
int main(int argc, char const *argv[])
{	
	int pidChildPid;
	RandomFifoName stFifoName;
   	char*cptrTempFifoName;
   	char strLogMsg[BUFSIZ];
	char cClientCommand = '\0';
	time_t tRawFormat;
	pid_t pidServingChild = 0;
	pid_t pidParentOfClient = 0;
	//char const *cptrMainFifo = argv[1];

	pidParent = getpid();
	iNumOfChilds = 0;
	clClients.iSize = MAX_CLIENTS;
	clClients.iCurrrent = 0;
	pidParent = getpid();
	
	if ( MyUsage(argc,argv) == ERROR ) 
    	return 0;
    memset(MAIN_FIFO,'\0',sizeof(char) * MAIN_FIFO_SIZE);
	strcpy(MAIN_FIFO,argv[1]);

    sprintf(strLogFile, "%s%d%s", "SSLogFile_", pidParent, ".txt");
  
	fdLogFile = open(strLogFile, O_WRONLY | O_APPEND | O_CREAT,
			S_IRUSR | S_IWUSR);

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

	sprintf(strLogMsg, "INFO: Starting lsserver...\n");
	write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
	memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

	time(&tRawFormat);
	sprintf(strLogMsg, "INFO: Starting Time: %s", ctime(&tRawFormat));
	write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
	memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

	sprintf(strLogMsg, "INFO: Creating FIFO: %s\n", MAIN_FIFO);
	write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
	memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

	if (mkfifo(MAIN_FIFO,0666) == -1)
	{
        perror("Failed to create mainFIFO");
        return ERROR;
	}

	sprintf(strLogMsg, "INFO: FIFO: %s is ready.\n", MAIN_FIFO);
	write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
	memset(strLogMsg, '\0', BUFSIZ * sizeof(char));
    while(1)
    {	
	
	    int iServerFIFO = open(MAIN_FIFO,O_RDWR);
	    if (iServerFIFO < 0)
    	{
	    	perror("Failed to open mainFIFO");
	    	return ERROR;
    	}
       	Parameter stPar;
       	memset(stPar.iPar,-1,sizeof(int) * PAR_SIZE);
       	memset(stPar.chOper,'\0',sizeof(char) * OP_SIZE);
        while(read(iServerFIFO,&stPar,sizeof(Parameter)) > 0 && (strncmp(stPar.chOper,"operation",9)) == 0){//Read struct
	        
	        printf("Connected child:%d\n\n",stPar.pidClient);

	        if (mkfifo("signal",0666) == -1)
			{
        		//perror("Failed to create sinal");
			}
			
	        int pipefd[2];
	        if(pipe(pipefd) < 0){
	            perror("Failed to create pipe");
	            break;
	        }
	        
	        if ((pidChildPid = fork()) == -1)
	    	{	
	    		sprintf(strLogMsg, "ERROR: failed to call fork()\n");
				write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
				memset(strLogMsg, '\0', BUFSIZ * sizeof(char));
				exit(EXIT_FORK_FAIL);
	            break;
	    	}
	    	else if (pidChildPid == 0)
	    	{	
	    		cptrTempFifoName = fnCreateFifoName();
	    		if (mkfifo(cptrTempFifoName,FIFO_PERMS) == -1)
			    {
			        perror("Failed to create RandomFifo");
			        exit(0);
			    }
			    int iRndFifo = open(cptrTempFifoName,O_RDWR);
			    if (iRndFifo < 0)
			    {
			    	perror("Failed to open RandomFifo");
			    	exit(0);
			    }
	            close(pipefd[0]);
	            write(pipefd[1], cptrTempFifoName,sizeof(char) * FIFO_SIZE);
	            close(pipefd[1]);
	            sleep(stPar.iWaitSecond);
	            fnCalculationOperation(stPar,iRndFifo);
	            printf("Sending result to child:%d\n",stPar.pidClient);
	            close(iRndFifo);
	    		exit(EXIT_SUCCESS);
	    	}else{
		    	close(pipefd[1]);
		        char chTempFifoName[FIFO_SIZE];
		        read(pipefd[0],chTempFifoName, sizeof(char) * FIFO_SIZE);
		        memset(stFifoName.chName,'\0',FIFO_SIZE);
		        strcpy(stFifoName.chName,chTempFifoName);
		        close(pipefd[0]);
	        	write(iServerFIFO,&stFifoName, sizeof(RandomFifoName));
		    	close(iServerFIFO);
		    }
		    if (!fnIsKnownClient(stPar.pidClient)) {
				/* Add new client to list */
				clClients.pidClients[clClients.iCurrrent] = stPar.pidClient;
				(clClients.iCurrrent) = ((clClients.iCurrrent) + 1);
			}

			if (iNumOfChilds < MAX_CHILD) {
				pidChildList[iNumOfChilds] = pidChildPid;
				++iNumOfChilds;

				sprintf(strLogMsg,
						"INFO: Client Connected: serving child: %d client pid:%d\n",
						pidChildPid, stPar.pidClient);

				write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));

				fnCheckChilds(); /* clean up child list */

			} else {

				sprintf(strLogMsg,
						"ERROR: Too many children to handle. Killing last one...\n");
				write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
				kill(pidChildPid, SIGINT);

			}

        }

    }
    unlink(MAIN_FIFO);
    unlink("signal");
	return (EXIT_SUCCESS);
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "fnCreateFifoName"
// Parameters     : no parameters
// Description    : create random fifo name
//
/*----------------------------------------------------------------------------*/
char*fnCreateFifoName(){

	char*cptrFifoName = (char*)malloc(sizeof(char) * FIFO_SIZE);
	sprintf(cptrFifoName,"FIFO_%ld",(long)getpid());
	return cptrFifoName;
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "fnGetWordNumber"
// Parameters     : <stStruct> : a struct parameter 
//                  <iFifoFd> : file discripter
//                  
// Description    : calculation operation 
//
/*----------------------------------------------------------------------------*/
int fnCalculationOperation(Parameter stStruct,int iFifoFd){

	char *cptrResult;
	CalcResult stTemp;
	if (strcmp(stStruct.chOper,"operation1") == 0)
	{
		if (stStruct.iSize % 3 == 0)
		{	
			int i;
			int j = 0;
			for ( i = 0; i < stStruct.iSize; i += 3)
			{
				cptrResult = fnOperation1(stStruct.iPar[i],stStruct.iPar[i+1],stStruct.iPar[i+2]);
				strcpy(stTemp.chRes[j++],cptrResult);
				free(cptrResult);
			}
			stTemp.iResSize = j;
			write(iFifoFd,&stTemp,sizeof(CalcResult));		
			
			return TRUE;
		}
		else{
			strcpy(stTemp.chRes[0],"ERROR: Parameter");
			stTemp.iResSize = 1;
			write(iFifoFd,&stTemp,sizeof(CalcResult));
			return FALSE;
		}
	}
	/**********************************************************************************/
	if (strcmp(stStruct.chOper,"operation2") == 0)
	{	
		if (stStruct.iSize % 2 == 0)
		{
			int i;
			int j = 0;
			for ( i = 0; i < stStruct.iSize; i += 2)
			{
				cptrResult = fnOperation2(stStruct.iPar[i],stStruct.iPar[i+1]);
				strcpy(stTemp.chRes[j++],cptrResult);
				free(cptrResult);
			}
			stTemp.iResSize = j;
			write(iFifoFd,&stTemp,sizeof(CalcResult));			
			
			return TRUE;
		}
		else{
			strcpy(stTemp.chRes[0],"ERROR: Parameter");
			stTemp.iResSize = 1;
			write(iFifoFd,&stTemp,sizeof(CalcResult));
			return FALSE;
		}
	}
	/**********************************************************************************/
	if (strcmp(stStruct.chOper,"operation3") == 0)
	{
		if (stStruct.iSize % 3 == 0)
		{	
			int i;
			int j = 0;
			for ( i = 0; i < stStruct.iSize; i += 3)
			{
				cptrResult = fnOperation3(stStruct.iPar[i],stStruct.iPar[i+1],stStruct.iPar[i+2]);
				strcpy(stTemp.chRes[j++],cptrResult);
				free(cptrResult);
			}			
			stTemp.iResSize = j;
			write(iFifoFd,&stTemp,sizeof(CalcResult));
			return TRUE;	
		}
		else{
			strcpy(stTemp.chRes[0],"ERROR: Parameter");
			stTemp.iResSize = 1;
			write(iFifoFd,&stTemp,sizeof(CalcResult));
			return FALSE;
		}
	}
	/**********************************************************************************/
	if (strcmp(stStruct.chOper,"operation4") == 0)
	{
		if (stStruct.iSize % 4 == 0)
		{	
			int i;
			int j = 0;
			for ( i = 0; i < stStruct.iSize; i += 4)
			{
				cptrResult = fnOperation4(stStruct.iPar[i],stStruct.iPar[i+1],stStruct.iPar[i+2],stStruct.iPar[i+3]);
				strcpy(stTemp.chRes[j++],cptrResult);
				free(cptrResult);
			}			
			stTemp.iResSize = j;
			write(iFifoFd,&stTemp,sizeof(CalcResult));
			return TRUE;
		}
		else{
			strcpy(stTemp.chRes[0],"ERROR: Parameter");
			stTemp.iResSize = 1;
			write(iFifoFd,&stTemp,sizeof(CalcResult));
			return FALSE;
		}
	}
	strcpy(stTemp.chRes[0],"ERROR: operation name!!");
	stTemp.iResSize = 1;
	write(iFifoFd,&stTemp,sizeof(CalcResult));
	return ERROR;
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "fnOperation1"
// Parameters     : <iNumA> : integer value
//                  <iNumB> : integer value
//                  <iNumC> : integer value
//             
// Description    : calculation operation1 and retun result as string
//
/*----------------------------------------------------------------------------*/
char*fnOperation1(int iNumA,int iNumB,int iNumC){

	char*cptrResult = (char*)malloc(sizeof(char)*RESULT_SIZE);
	if (iNumC == 0)
	{
		sprintf(cptrResult,"EXCEPTION: %s"," division by zero");
		return cptrResult;
	}
	double dbRes1 = pow(iNumA,2) + pow(iNumB,2);
	double dbRes2 = sqrt(dbRes1);
	if (iNumC < 0)
	{
		iNumC = iNumC * -1;
	}
	double dbRes = dbRes2 / iNumC;
	sprintf(cptrResult,"Result: %f",dbRes);
	return cptrResult; 
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "fnOperation2"
// Parameters     : <iNumA> : integer value
//                  <iNumB> : integr value
//                  
// Description    : calculation operation2 and retun result as string
//
/*----------------------------------------------------------------------------*/
char*fnOperation2(int iNumA,int iNumB){

	char*cptrResult = (char*)malloc(sizeof(char)*RESULT_SIZE);

	double dbRes1 = iNumA + iNumB;
	if (dbRes1 < 0)
	{
		sprintf(cptrResult,"EXCEPTION: %s","sum of parameters is negative");
		return cptrResult;
	}
	double dbRes = sqrt(dbRes1);
	sprintf(cptrResult,"Result: %f",dbRes);
	return cptrResult;
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "fnOperation3"
// Parameters     : <iNumA> : integer value
//                  <iNumB> : integer value
//                  <iNumC> : integer value
//                  
// Description    : calculation operation3 and retun result as string
//
/*----------------------------------------------------------------------------*/
char* fnOperation3(int iNumA,int iNumB,int iNumC){

	char*cptrResult = (char*)malloc(sizeof(char)*RESULT_SIZE);
	double dbDelta = pow(iNumB,2) - 4 * iNumC * iNumA;
	if (dbDelta < 0)
	{
		sprintf(cptrResult,"EXCEPTION:%s","Delta is negative");
		return cptrResult;
	}
	double dbRoot1;
	if (iNumA == 0)
	{
		dbRoot1 = -iNumC / iNumB;
		sprintf(cptrResult,"Result: root1= %f  :  %s ",dbRoot1,"root2 = Doesn't exist root2");
		return cptrResult;
	}
	dbRoot1 = (-iNumB + sqrt(dbDelta)) / (2 * iNumA);
	double dbRoot2 = (-iNumB - sqrt(dbDelta)) / (2 * iNumA);
	sprintf(cptrResult,"Result: root1= %f  : root2 = %f ",dbRoot1,dbRoot2);
	return cptrResult;
}
/*----------------------------------------------------------------------------*/
//
// Function Name  : "fnOperation4"
// Parameters     : <iNumA> : integer value
//                  <iNumB> : integer value
//                  <iNumC> : integer value
//                  <iNumD> : integer value
// Description    : calculation operation4 and retun result as string
//
/*----------------------------------------------------------------------------*/
char*fnOperation4(int iNumA,int iNumB,int iNumC,int iNumD){

	char*cptrResult = (char*)malloc(sizeof(char)*RESULT_SIZE);
	if (iNumC == 0 && iNumD == 0)
	{
		sprintf(cptrResult,"EXCEPTION: %s","Function is undefined");
		return cptrResult;
	}
	if (iNumC == 0 && iNumA == 0)
	{
		sprintf(cptrResult,"EXCEPTION: %s","Inverse of the function is undefined");
		return cptrResult;
	}

	sprintf(cptrResult,"Result: [ (%d)X + (%d) ] / [ (%d)X + (%d) ]",-iNumD,iNumB,iNumC,-iNumA);
	return cptrResult;
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

	while (0 < fnWaitChilds(NULL ))
		;

	if (gettimeofday(&tEnd, NULL )) {
		sprintf(strLogMsg, "ERROR: Failed to get ending time.\n");
		write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
		memset(strLogMsg, '\0', BUFSIZ * sizeof(char));
	}

	close(iServerFIFO);
	unlink(MAIN_FIFO);

	sprintf(strLogMsg, "INFO: FIFO file closed.\n");
	write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
	memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

	sprintf(strLogMsg, "INFO: Sending Shutdown signal to clients...\n");
	write(fdLogFile, strLogMsg, strlen(strLogMsg) * sizeof(char));
	memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

	for (iCntr = 0; iCntr < clClients.iCurrrent; ++iCntr) {
		kill(clClients.pidClients[iCntr], SIGUSR1);
	}

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
	/*sprintf(strLogMsg, "cp -f %s .", strLogFile);*/
	system(strLogMsg);
	memset(strLogMsg, '\0', BUFSIZ * sizeof(char));

	/* kill any reamining childs */
	for (iCntr = 0; iCntr < iNumOfChilds; ++iCntr) {
		waitpid(pidChildList[iCntr], &iStat, WNOHANG);
		if (!WIFEXITED(iStat)) {
			kill(pidChildList[iCntr], SIGINT);
		}
	}
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
//					If client pid exists returns 1,* otherwise 0.
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
//					<tEnd> : end time value
//             
// Description    : Returns difference between tBegin and tEnd
//
/*----------------------------------------------------------------------------*/
long fnTimeDiff(struct timeval tBegin, struct timeval tEnd) {
	return ((MILLION * (tEnd.tv_sec - tBegin.tv_sec))
			+ (tEnd.tv_usec - tBegin.tv_usec));
}
/* End of lsserver.c */