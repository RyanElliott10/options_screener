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

long historical_list_size;
struct historical_price **historical_price_list;

struct parent_stock **gather_tickers(long *pl_size)
{
   int i;
   char previous[TICK_SIZE];
   long price_list_size, parent_list_size;
   struct parent_stock **parent_list;

   parent_list_size = 0;
   parent_list = safe_malloc(sizeof(struct parent_stock *));
   memset(previous, 0, TICK_SIZE);

   gather_data();

   for (i = 0; i < historical_list_size; i++)
   {
      if (0 != strcmp(historical_price_list[i] -> ticker, previous))
      {
         memset(previous, 0, TICK_SIZE);
         strcpy(previous, historical_price_list[i] -> ticker);

         parent_list = safe_realloc(parent_list, ++parent_list_size * (sizeof(struct parent_stock *)));
         parent_list[parent_list_size-1] = safe_malloc(sizeof(struct parent_stock));
         parent_list[parent_list_size-1] -> prices_list = safe_malloc(sizeof(struct historical_price *));
         
         price_list_size = 0;
      }

      parent_list[parent_list_size-1] -> prices_list = safe_realloc(parent_list[parent_list_size-1] -> prices_list, ++price_list_size * (sizeof(struct historical_price *)));
      parent_list[parent_list_size-1] -> prices_list[price_list_size-1] = safe_malloc(sizeof(struct historical_price));
      parent_list[parent_list_size-1] -> price_list_size = price_list_size;

      strcpy(parent_list[parent_list_size-1] -> prices_list[price_list_size-1] -> ticker, historical_price_list[i] -> ticker);
      parent_list[parent_list_size-1] -> prices_list[price_list_size-1] -> date = historical_price_list[i] -> date;
      parent_list[parent_list_size-1] -> prices_list[price_list_size-1] -> open = historical_price_list[i] -> open;
      parent_list[parent_list_size-1] -> prices_list[price_list_size-1] -> low = historical_price_list[i] -> low;
      parent_list[parent_list_size-1] -> prices_list[price_list_size-1] -> high = historical_price_list[i] -> high;
      parent_list[parent_list_size-1] -> prices_list[price_list_size-1] -> close = historical_price_list[i] -> close;
      parent_list[parent_list_size-1] -> prices_list[price_list_size-1] -> volume = historical_price_list[i] -> volume;

      // to free up memory since this is memory intensive
      free(historical_price_list[i]);
   }

   free(historical_price_list);
   
   *pl_size = parent_list_size;
   return parent_list;
}

void gather_data(void)
{
   int rc;
   char *error_msg;
   sqlite3 *db;
   // sqlite3_stmt *res;
   char *sql = "SELECT * FROM historicalPrices";

   historical_list_size = 0;
   historical_price_list = malloc(sizeof(struct historical_price *));

   rc = sqlite3_open("historicalPrices", &db);
   
   if (rc != SQLITE_OK)
   {
      fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      
      return;
   }

   rc = sqlite3_exec(db, sql, callback, 0, &error_msg);

   sqlite3_close(db);
}

void format_historical_prices(struct parent_stock stock)
{

}

void price_trend(struct parent_stock stock)
{


   return;
}

void bid_ask_spread(struct option opt)
{

   
   return;
}

void perc_from_strike(struct option opt)
{

   
   return;
}

void yearly_high_low(struct parent_stock stock)
{

   
   return;
}

void perc_from_ivs(struct option opt)
{

   
   return;
}

void iv_range(struct option opt)
{

   
   return;
}

void avg_open_close(struct parent_stock stock)
{


   return;
}

void find_curr_price(struct parent_stock stock)
{


   return;
}

int callback(void *NotUsed, int argc, char **argv, char **azColName) {
   NotUsed = 0;

   historical_price_list = realloc(historical_price_list, ++historical_list_size * (sizeof(struct historical_price *)));
   historical_price_list[historical_list_size-1] = malloc(sizeof(struct historical_price));

   memset(historical_price_list[historical_list_size-1] -> ticker, 0, 10);
   strcpy(historical_price_list[historical_list_size-1] -> ticker, argv[0]);

   historical_price_list[historical_list_size-1] -> date = atof(argv[1]);
   historical_price_list[historical_list_size-1] -> open = atof(argv[2]);
   historical_price_list[historical_list_size-1] -> low = atof(argv[3]);
   historical_price_list[historical_list_size-1] -> high = atof(argv[4]);
   historical_price_list[historical_list_size-1] -> close = atof(argv[5]);
   historical_price_list[historical_list_size-1] -> volume = atof(argv[6]);
   
   return 0;
}
