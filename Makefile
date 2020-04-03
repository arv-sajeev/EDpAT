CC=gcc 
CFLAGS= -I. -g 
DEPS = edpat.h scripts.h testcase.h variable.h packet.h utils.h print.h

SRC= edpat.o EthPortIO.o scripts.o print.o testcase.o variable.o utils.o packet.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)


all:	edpat.exe

edpat.exe: $(SRC)
	$(CC) -o edpat.exe  $(SRC) -lrt -lpthread

clean:	
	rm edpat.exe $(SRC)
