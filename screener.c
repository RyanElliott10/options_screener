#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sqlite3.h>
#include <sys/types.h>

#include "screener.h"
#include "util.h"
#include "safe.h"

long pl_size;
struct historical_price **price_list;

int main(int argc, char *argv[])
{
   int mode, status, tl_size;
   long parent_array_size;
   pid_t pid;
   char **tick_list = NULL;
   struct parent_stock **parent_array;

	mode = REGULAR;

   if ((pid = fork()) == 0) // if child, exec python program
   {
      printf("Collecting stock and option data...\n");
      execlp("python3", "python3", "options_collector.py", (char *)NULL);

      printf("Warning: Unable to gather data\n");
      exit(EXIT_FAILURE);
   }

   waitpid(pid, &status, 0);
   if (status)
   {
      printf("Warning: Unable to gather data\n");
      exit(EXIT_FAILURE);
   }

	if (mode == REGULAR)
	{
		printf("Gathering historical stock prices from database...\n");
		parent_array = gather_tickers(&parent_array_size);  // collects all historical data and stores in structs

		gather_options(parent_array, parent_array_size);  // collects all data from database and stores in structs
		screen_volume_oi_baspread(parent_array, parent_array_size);  // screens for volume/oi requirements, bid x ask spread
		calc_basic_data(parent_array, parent_array_size);

		free_all(tick_list, &tl_size);
	}

   return 0;
}

/* parses argv, decides which mode to use, creates list containing personalized stocks, if necessary */
char **parse_args(int argc, char *argv[], int *mode, int *tl_size)
{
   int i;
   char **tick_list = safe_malloc(sizeof(char *));

   i = 2;
   *tl_size = 0;

   if ((argc > 1) && (argv[1][0] == '-')) // if there is a flag
   {
      switch (argv[1][1]) // determine which flag they inputted
      {
      case 'o': // if they want to only use input stocks
                // fall-through
         *mode = NEW_STOCKS;
      case 'a': // if they want to append stocks
         if (*mode != NEW_STOCKS)
            *mode = APPEND_STOCKS;

         for (; i < argc; i++) // collect all desired tickers
         {
            tick_list = safe_realloc(tick_list, ++(*tl_size) * sizeof(char *));
            tick_list[*tl_size - 1] = safe_malloc(sizeof(char));

            strcpy(tick_list[*tl_size - 1], argv[i]);
         }

         return tick_list;
      default: // if there is a usage error
         fprintf(stderr, "usage: ./screener [ -oa ] [ tickers ]\n");
         exit(EXIT_FAILURE);
      }
   }

   *mode = REGULAR;
   return NULL;
}

/* frees all mallocs made in main, or functions called by main */
void free_all(char **tick_list, int *tl_size)
{
   int i;

   for (i = 0; i < *tl_size; i++)
      free(tick_list[i]);
}
