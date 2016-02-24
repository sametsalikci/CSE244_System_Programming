rm synchronyzationServer
rm client.exe
rm *.o
gcc -c synchronyzationServer.c
gcc synchronyzationServer.o -lm -o synchronyzationServer
gcc -c client.c
gcc client.o -o client.exe
