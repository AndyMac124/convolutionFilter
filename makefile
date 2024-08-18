COMPILER = gcc
CFLAGS = -Wall -pedantic
COBJS = matrix.o

EXES = conv_filter

all: ${EXES}

conv_filter:	conv_filter.c ${COBJS}
	${COMPILER} ${CFLAGS} conv_filter.c ${COBJS} -o conv_filter -lm

matrix.o: matrix.c
	${COMPILER} ${CFLAGS} -c matrix.c

run: conv_filter
	./conv_filter

clean:
	rm -f *~ *.o ${EXES}