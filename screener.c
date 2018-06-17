#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "screener.h"
#include "safe.h"

int main(int argc, char *argv[])
{
   int mode, status, tl_size;
   pid_t pid;
   char **tick_list;

   // if child, exec python program
   if((pid = fork()) == 0)
   {
      execlp("python", "python", "options_collector.py", (char *)NULL);
      exit(EXIT_FAILURE);
   }

   waitpid(pid, &status, 0);

   tick_list = parse_args(argc, argv, &mode, &tl_size);

   free_all(tick_list, &tl_size);

   return 0;
}

/* 
 * parses argv, decides which mode to use, creates list containing
 * personalized stocks, if necessary
 */
char **parse_args(int argc, char *argv[], int *mode, int *tl_size)
{
   int i;
   char **tick_list = safe_malloc(sizeof(char *));

   i        = 2;
   *tl_size = 0;
   *mode    = 0;  // silences warnings

   // if there is a flag
   if ((argc > 1) && (argv[1][0] == '-'))
   {
      // determine which flag they inputted
      switch (argv[1][1])
      {
         // if they want to only use input stocks
         case 'o':   // fall-through
            *mode = NEW_STOCKS;
         // if they want to append stocks
         case 'a':
            if (*mode != NEW_STOCKS)
               *mode = APPEND_STOCKS;

            // collect all desired tickers
            for (; i < argc; i++)
            {
               tick_list = safe_realloc(tick_list, ++(*tl_size) * sizeof(char *));
               tick_list[*tl_size-1] = safe_malloc(sizeof(char));

               strcpy(tick_list[*tl_size-1], argv[i]);
            }

            return tick_list;
         // if there is a usage error
         default:
            fprintf(stderr, "usage: ./screener [ -o ] [ tickers ]\n");
            fprintf(stderr, "usage: ./screener [ tickers ]\n");
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
