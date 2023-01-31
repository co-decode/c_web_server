server: server.o http.o hashtable.o routes.o
	gcc server.o http.o hashtable.o routes.o -o server
server.o: server.c
	gcc -c server.c
http.o: http.c http.h
	gcc -c http.c http.h
hashtable.o: hashtable.c hashtable.h
	gcc -c hashtable.c hashtable.h
routes.o: routes.c routes.h
	gcc -c routes.c routes.h
clean:
	rm *.o *.gch server
