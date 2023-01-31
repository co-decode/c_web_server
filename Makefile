server: server.o hashtable.o
	gcc server.o hashtable.o -o server
server.o: server.c
	gcc -c server.c
hashtable.o: hashtable.c hashtable.h
	gcc -c hashtable.c hashtable.h
clean:
	rm *.o *.gch server
