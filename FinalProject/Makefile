# C Compiler
CC=gcc

# main file
MAIN=mine.c

#Client file
CLIENT=miner.c

#executable name1
EXECUTABLE_NAME1=mine.exe

#executable name2
EXECUTABLE_NAME2=miner.exe

# library object files
LIBRARY_OBJECTS=-lm

# library object files
LIBRARY_OBJECTS1=-lpthread

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
	@${CC} ${MAIN} ${LIBRARY_OBJECTS} ${LIBRARY_OBJECTS1} -o ${EXECUTABLE_NAME1}
	@${CC} ${CLIENT} ${LIBRARY_OBJECTS1} -o ${EXECUTABLE_NAME2}

clean:
	@echo Removing Executable Files...
	@${REMOVE} ${EXECUTABLE_NAME1} ${EXECUTABLE_NAME2}
	@echo Removing some other extras...
	
logs:
	@echo Removing LOGs...
	@${REMOVE} ${REMOVE_PARAMS} *.txt

