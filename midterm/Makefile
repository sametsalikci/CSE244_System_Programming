# C Compiler
CC=gcc

# main file
MAIN=synchronyzationServer.c

#Client file
CLIENT=client.c

#executable name1
EXECUTABLE_NAME1=synchronyzationServer.exe

#executable name2
EXECUTABLE_NAME2=client.exe

# library object files
LIBRARY_OBJECTS=-lm

# cc paramters for general use
GENERAL_PARAMS=-c

#Some other variables (compatible with Linux at least)
CLEAR=clear
REMOVE=rm

all:
#	@echo [CC] ${LIBRARY_SOURCES}
#	@${CC} -c ${LIBRARY_SOURCES}
	@echo [CC] ${GENERAL_PARAMS} ${MAIN}
	@echo [CC] ${GENERAL_PARAMS} ${CLIENT}
	@${CC} ${MAIN} ${LIBRARY_OBJECTS} -o ${EXECUTABLE_NAME1}
	@${CC} ${CLIENT} -o ${EXECUTABLE_NAME2}

clean:
	@echo Removing Executable Files...
	@${REMOVE} ${EXECUTABLE_NAME1} ${EXECUTABLE_NAME2}
	@echo Removing some other extras...
	
logs:
	@echo Removing LOGs...
	@${REMOVE} ${REMOVE_PARAMS} *.txt

