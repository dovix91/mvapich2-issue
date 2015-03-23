.PHONY: clean

CC      := g++
CFLAGS  := -Wall -g -std=c++11
IFLAGS  := -I/opt/mvapich2-2.1rc2/include/
LFLAGS  := -L/opt/mvapich2-2.1rc2/lib/
LDFLAGS := ${LDFLAGS} -lpthread -lmpi

all: bug_test

bug_test: bug_test.cpp net/NetworkManager.h net/mpi/NetworkManagerMPIBlockingMultithread.cpp net/mpi/NetworkManagerMPIBlockingMultithread.h util/sys/ConditionVariable.h util/sys/Mutex.h util/sys/ConditionVariable.cpp util/sys/Mutex.cpp util/queue/SyncedQueue.h
	${CC} ${CFLAGS} ${IFLAGS} ${LFLAGS} ${LDFLAGS} bug_test.cpp net/mpi/NetworkManagerMPIBlockingMultithread.cpp util/sys/ConditionVariable.cpp util/sys/Mutex.cpp -o bug_test

clean:
	rm -f bug_test
	
