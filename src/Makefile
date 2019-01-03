CC     = clang
CFLAGS = -pedantic -Wall -g
BFLAGS = -lsqlite3
OBJS   = screener.o general_stocks.o options.o safe.o
MAIN   = screener

screener : $(OBJS)
	$(CC) $(OBJS) $(BFLAGS) -o screener

screener.o : screener.c ../include/screener.h
	$(CC) $(CFLAGS) -c screener.c

general_stocks.o : general_stocks.c ../include/general_stocks.h
	$(CC) $(CFLAGS) -c general_stocks.c

options.o : options.c ../include/options.h
	$(CC) $(CFLAGS) -c options.c

safe.o : safe.c ../include/safe.h
	$(CC) $(CFLAGS) -c safe.c

clean: 
	@rm *.o $(MAIN)