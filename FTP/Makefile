.PHONY:clean
CC=g++
CFLAGS=-Wall -g
BIN=miniftpd
OBJS=main.o sysutil.o session.o privparent.o ftpproto.o str.o tunable.o parseconf.o
HEAD=sysutil.h session.h privparent.h ftpproto.h str.h tunable.h parseconf.h
LIBS=-lcrypt

$(BIN) : $(OBJS) $(HEAD)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
#main.o sysutil.o session.o privparent.o ftpproto.o
%.o : %.cpp
	$(CC) $(CFLAGS) -c $< -o $@
#main.o : main.cpp
#	$(CC) $(CFLAGS) -c main.cpp -o main.o
#sysutil.o : sysutil.cpp
#	$(CC) $(CFLAGS) -c sysutil.cpp -o sysutil.o
#session.o : session.cpp
#	$(CC) $(CFLAGS) -c session.cpp -o session.o
#privparent.o : privparent.cpp
#	$(CC) $(CFLAGS) -c privparent.cpp -o privparent.o
#ftpproto.o : ftpproto.cpp
#	$(CC) $(CFLAGS) -c ftpproto.cpp -o ftpproto.o

clean:
	rm -f *.o
