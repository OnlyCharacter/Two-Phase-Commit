CC = gcc
CFLAGS = -g -Wall
LIBS = -lsqlite3
TEST_DIR = ../test
REPLICA1 = ${TEST_DIR}/3334
REPLICA2 = ${TEST_DIR}/3335

$(shell if [ ! -e ${TEST_DIR} ]; then mkdir -p ${TEST_DIR}; fi)
$(shell if [ ! -e ${REPLICA1} ]; then mkdir -p ${REPLICA1}; fi)
$(shell if [ ! -e ${REPLICA2} ]; then mkdir -p ${REPLICA2}; fi)

all: replica server clean
	cp replica ${REPLICA1}
	mv replica ${REPLICA2}
	mv server ${TEST_DIR}

replica: replica.o wrapper.o
	${CC} $^ ${CFLAGS} -o $@ ${LIBS}

server: master_server.c
	${CC} $^ ${CFLAGS} -o $@ ${LIBS}

clean:
	rm ./*.o

wrapper.o: wrapper.c
	${CC} -c $^ ${CFLAGS} -o $@ ${LIBS}

replica.o: replica.c
	${CC} -c $^ ${CFLAGS} -o $@ ${LIBS} -I./ -Lwrapper.h