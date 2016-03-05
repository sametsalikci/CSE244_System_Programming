CC = gcc
LIB = -lpthread


wordCount:	wordCount.o 
	$(CC) -o wordCount wordCount.o $(LIB)


wordCount.o: 
	$(CC) -c wordCount.c

clean:
	rm -f wordCount *.o
