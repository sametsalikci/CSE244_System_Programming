CC = gcc
LIB = -lpthread


newWordCount:	newWordCount_hw6.o 
	$(CC) -o newWordCount_hw6 newWordCount_hw6.o $(LIB)


newWordCount.o: 
	$(CC) -c newWordCount_hw6.c

clean:
	rm -f newWordCount_hw6 *.o
