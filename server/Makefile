CC = gcc
CFLAGS = -g -Wall
LIBS = -lsqlite3

replica: replica.o wrapper.o
	${CC} $^ ${CFLAGS} -o $@ ${LIBS}

clean:
	rm ./*.o

wrapper.o: wrapper.c
	${CC} -c $^ ${CFLAGS} -o $@ ${LIBS}

replica.o: replica.c
	${CC} -c $^ ${CFLAGS} -o $@ ${LIBS} -I./ -Lwrapper.h