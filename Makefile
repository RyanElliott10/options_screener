CC     = gcc
CFLAGS = -pedantic -Wall -g
BFLAGS = -lsqlite3
OBJS   = screener.o util.o safe.o
MAIN   = screener

screener : $(OBJS)
	$(CC) $(OBJS) $(BFLAGS) -o screener

screener.o : screener.c screener.h
	$(CC) $(CFLAGS) -c screener.c

util.o : util.c util.h
	$(CC) $(CFLAGS) -c util.c

safe.o : safe.c safe.h
	$(CC) $(CFLAGS) -c safe.c

clean: 
	@rm *.o $(MAIN)