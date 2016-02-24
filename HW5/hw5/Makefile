CC = gcc
LIB = -lpthread


newWordCount:	newWordCount.o 
	$(CC) -o newWordCount newWordCount.o $(LIB)


newWordCount.o: 
	$(CC) -c newWordCount.c

clean:
	rm -f newWordCount *.o
