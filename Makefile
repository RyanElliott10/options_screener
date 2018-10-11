CC     = clang
CFLAGS = -pedantic -Wall -g
BFLAGS = -lsqlite3
OBJS   = screener.o general_stocks.o options.o safe.o
MAIN   = screener

screener : $(OBJS)
	$(CC) $(OBJS) $(BFLAGS) -o screener

screener.o : screener.c screener.h
	$(CC) $(CFLAGS) -c screener.c

general_stocks.o : general_stocks.c general_stocks.h
	$(CC) $(CFLAGS) -c general_stocks.c

options.o : options.c options.h
	$(CC) $(CFLAGS) -c options.c

safe.o : safe.c safe.h
	$(CC) $(CFLAGS) -c safe.c

clean: 
	@rm *.o $(MAIN)