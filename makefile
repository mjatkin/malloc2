CC := gcc
CFLAGS := -Wall -pedantic -std=gnu99

SRCS = main.c alloc.c
OBJS = ${SRCS:.c=.o}
EXE = malloc2.out

SRCDIR := src
OBJDIR := obj
BINDIR := bin

DBGOBJDIR := ${OBJDIR}/debug
DBGBINDIR := ${BINDIR}/debug
DBGOBJS := ${addprefix ${DBGOBJDIR}/, ${OBJS}}
DBGFLAGS := -D DEBUG -g

RELOBJDIR := ${OBJDIR}/release
RELBINDIR := ${BINDIR}/release
RELOBJS := ${addprefix ${RELOBJDIR}/, ${OBJS}}
RELFLAGS := 

.PHONY: all clean debug release init relrun dbgrun

all: init release

release: 
	${CC} -c ${CFLAGS} ${RELFLAGS} ${SRCDIR}/main.c -o ${RELOBJDIR}/main.o
	${CC} -c ${CFLAGS} ${RELFLAGS} ${SRCDIR}/alloc.c -o ${RELOBJDIR}/alloc.o
	${CC} ${RELOBJS} -o ${RELBINDIR}/${EXE}

debug: 
	${CC} -c ${CFLAGS} ${DBGFLAGS} ${SRCDIR}/main.c -o ${DBGOBJDIR}/main.o
	${CC} -c ${CFLAGS} ${DBGFLAGS} ${SRCDIR}/alloc.c -o ${DBGOBJDIR}/alloc.o
	${CC} ${DBGOBJS} -o ${DBGBINDIR}/${EXE}

relrun:
	./${RELBINDIR}/${EXE}

dbgrun:
	./${DBGBINDIR}/${EXE}

init:
	@mkdir -p ${DBGBINDIR} ${RELBINDIR} ${DBGOBJDIR} ${RELOBJDIR}

clean:
	rm -f ${DBGOBJDIR}/*.o
	rm -f ${DBGBINDIR}/*.out
	rm -f ${RELOBJDIR}/*.o
	rm -f ${RELBINDIR}/*.out

