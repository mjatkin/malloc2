CC := clang
CFLAGS := -Wall -pedantic -std=gnu99
LIBS := -lpthread

SRCS := main.c alloc.c locks.c
OBJS := ${SRCS:.c=.o}
EXE := malloc2.out

SRCDIR := src
OBJDIR := obj
BINDIR := bin

DBGOBJDIR := ${OBJDIR}/debug
DBGBINDIR := ${BINDIR}/debug
DBGEXE := ${BINDIR}/debug/${EXE}
DBGOBJS := ${addprefix ${DBGOBJDIR}/, ${OBJS}}
DBGFLAGS := -D DEBUG -g -O0

RELOBJDIR := ${OBJDIR}/release
RELBINDIR := ${BINDIR}/release
RELEXE := ${BINDIR}/release/${EXE}
RELOBJS := ${addprefix ${RELOBJDIR}/, ${OBJS}}
RELFLAGS := 

.PHONY: all clean debug release init relrun dbgrun

all: init release

release: ${RELEXE}

${RELEXE}: ${RELOBJS}
	${CC} ${RELOBJS} ${LIBS} -o ${RELEXE}

${RELOBJDIR}/main.o: ${SRCDIR}/main.c ${SRCDIR}/alloc.h
	${CC} -c ${CFLAGS} ${RELFLAGS} ${SRCDIR}/main.c -o ${RELOBJDIR}/main.o

${RELOBJDIR}/alloc.o: ${SRCDIR}/alloc.c ${SRCDIR}/alloc.h ${SRCDIR}/list.h ${SRCDIR}/locks.h
	${CC} -c ${CFLAGS} ${RELFLAGS} ${SRCDIR}/alloc.c -o ${RELOBJDIR}/alloc.o

${RELOBJDIR}/locks.o: ${SRCDIR}/locks.c ${SRCDIR}/locks.h
	${CC} -c ${CFLAGS} ${RELFLAGS} ${SRCDIR}/locks.c -o ${RELOBJDIR}/locks.o

debug: ${DBGEXE}

${DBGEXE}: ${DBGOBJS}
	${CC} ${DBGOBJS} ${LIBS} -o ${DBGEXE}

${DBGOBJDIR}/main.o: ${SRCDIR}/main.c ${SRCDIR}/alloc.h
	${CC} -c ${CFLAGS} ${DBGFLAGS} ${SRCDIR}/main.c -o ${DBGOBJDIR}/main.o

${DBGOBJDIR}/alloc.o: ${SRCDIR}/alloc.c ${SRCDIR}/alloc.h ${SRCDIR}/list.h ${SRCDIR}/locks.h
	${CC} -c ${CFLAGS} ${DBGFLAGS} ${SRCDIR}/alloc.c -o ${DBGOBJDIR}/alloc.o

${DBGOBJDIR}/locks.o: ${SRCDIR}/locks.c ${SRCDIR}/locks.h
	${CC} -c ${CFLAGS} ${DBGFLAGS} ${SRCDIR}/locks.c -o ${DBGOBJDIR}/locks.o

relrun: ${RELEXE}
	@./${RELEXE}

dbgrun: ${DBGEXE}
	@./${DBGEXE}

init:
	@mkdir -p ${DBGBINDIR} ${RELBINDIR} ${DBGOBJDIR} ${RELOBJDIR}

clean:
	rm -f ${DBGOBJS}
	rm -f ${DBGEXE}
	rm -f ${RELOBJS}
	rm -f ${RELEXE}

