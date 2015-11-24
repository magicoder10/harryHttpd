all: client server

client: client.o
	gcc -g client.o -o client

server: server.o
	gcc -g server.o -o server -pthread
	
client.o: client.c client.h
	gcc -g -c client.c

server.o: server.c server.h
	gcc -g -c server.c

clientDebug: clientDebug.o
	gcc -g clientDebug.o -o clientDebug

clientDebug.o: client.c client.h
	gcc -g -c -DDEBUG client.c -o clientDebug.o

serverDebug: serverDebug.o
	gcc -g serverDebug.o -o serverDebug -pthread

serverDebug.o: server.c server.h
	gcc -g -c -DDEBUG server.c -o server.o

clean:
	rm -rf *.o
	rm -rf client clientDebug
	rm -rf server serverDebug
	rm -rf .txt