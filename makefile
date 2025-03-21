COMPILER = mpicc
CFLAGS = -Wall -pedantic
COBJS = file_handling.o matrix_handling.o conv_filter_utils.o message_handling.o

EXES = conv_filter

all: ${EXES}

conv_filter:	conv_filter.c ${COBJS}
	${COMPILER} ${CFLAGS} conv_filter.c ${COBJS} -o conv_filter -lm

file_handling.o: file_handling.c matrix_handling.h
	${COMPILER} ${CFLAGS} -c file_handling.c

matrix_handling.o: matrix_handling.c
	${COMPILER} ${CFLAGS} -c matrix_handling.c

conv_filter_utils.o: conv_filter_utils.c
	${COMPILER} ${CFLAGS} -c conv_filter_utils.c

message_handling.o: message_handling.c conv_filter_utils.h
	${COMPILER} ${CFLAGS} -c message_handling.c

run: conv_filter
	mpirun -np 4 conv_filter ${ARGS}

clean:
	rm -f *~ *.o ${EXES}