CC     = gcc
CFLAGS = -pedantic -Wall -g
OBJS   = screener.o safe.o
MAIN   = screener

screener : $(OBJS)
	$(CC) $(OBJS) -o screener

screener.o : screener.c screener.h
	$(CC) $(CFLAGS) -c screener.c

safe.o : safe.c safe.h
	$(CC) $(CFLAGS) -c safe.c

clean: 
	@rm *.o $(MAIN)