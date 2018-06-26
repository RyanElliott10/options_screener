#include <math.h>
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

long historical_array_size;
long all_options_size;
struct historical_price **historical_price_array; // contains all historical price data from database, freed in gather_tickers
struct option **all_options;                      // contains all options info from database, freed in gather_options

struct parent_stock **gather_tickers(long *pa_size)
{
   int i;
   char previous[TICK_SIZE];
   long price_array_size, parent_array_size;
   struct parent_stock **parent_array; // list of all stock tickers containing lists of their historical prices

   parent_array_size = 0;
   parent_array = safe_malloc(sizeof(struct parent_stock *));
   memset(previous, 0, TICK_SIZE);

   gather_data();

   for (i = 0; i < historical_array_size; i++)
   {
      if (0 != strcmp(historical_price_array[i]->ticker, previous))
      {
         if (i != 0)
            find_curr_stock_price(parent_array[parent_array_size - 1]);

         memset(previous, 0, TICK_SIZE);
         strcpy(previous, historical_price_array[i]->ticker);

         parent_array = safe_realloc(parent_array, ++parent_array_size * (sizeof(struct parent_stock *)));
         parent_array[parent_array_size - 1] = safe_malloc(sizeof(struct parent_stock));

         memset(parent_array[parent_array_size - 1]->ticker, 0, TICK_SIZE);
         strcpy(parent_array[parent_array_size - 1]->ticker, historical_price_array[i]->ticker);
         parent_array[parent_array_size - 1]->prices_array = safe_malloc(sizeof(struct historical_price *));
         parent_array[parent_array_size - 1]->weight = 0;

         price_array_size = 0;
      }

      parent_array[parent_array_size - 1]->prices_array = safe_realloc(parent_array[parent_array_size - 1]->prices_array, ++price_array_size * (sizeof(struct historical_price *)));
      parent_array[parent_array_size - 1]->prices_array[price_array_size - 1] = safe_malloc(sizeof(struct historical_price));
      parent_array[parent_array_size - 1]->price_array_size = price_array_size;

      strcpy(parent_array[parent_array_size - 1]->prices_array[price_array_size - 1]->ticker, historical_price_array[i]->ticker);
      parent_array[parent_array_size - 1]->prices_array[price_array_size - 1]->date = historical_price_array[i]->date;
      parent_array[parent_array_size - 1]->prices_array[price_array_size - 1]->open = historical_price_array[i]->open;
      parent_array[parent_array_size - 1]->prices_array[price_array_size - 1]->low = historical_price_array[i]->low;
      parent_array[parent_array_size - 1]->prices_array[price_array_size - 1]->high = historical_price_array[i]->high;
      parent_array[parent_array_size - 1]->prices_array[price_array_size - 1]->close = historical_price_array[i]->close;
      parent_array[parent_array_size - 1]->prices_array[price_array_size - 1]->volume = historical_price_array[i]->volume;

      // to free up memory since this is memory intensive
      free(historical_price_array[i]);
   }

   free(historical_price_array);

   *pa_size = parent_array_size;
   return parent_array;
}

void gather_options(struct parent_stock **parent_array, long parent_array_size)
{
   int i, parent_array_index;
   char previous[TICK_SIZE];

   parent_array_index = 0;
   memset(previous, 0, TICK_SIZE);

   gather_options_data();

   // iterate through all_options array
   for (i = 0; i < all_options_size; i++)
   {
      // if it is a brand new ticker
      if (parent_array_index >= parent_array_size)
         printf("Something went wrong...\n");

      // if the current and previous ticker are different, means change in ticker
      if (0 != strcmp(all_options[i]->ticker, previous))
      {
         // to avoid skipping the first index
         if (i != 0)
            parent_array_index++;

         memset(previous, 0, TICK_SIZE);
         strcpy(previous, all_options[i]->ticker);

         parent_array[parent_array_index]->calls = safe_malloc(sizeof(struct option *));
         parent_array[parent_array_index]->puts = safe_malloc(sizeof(struct option *));
         parent_array[parent_array_index]->calls_size = 0;
         parent_array[parent_array_index]->puts_size = 0;
      }

      // if the option is a call
      if (all_options[i]->type == 1)
      {
         parent_array[parent_array_index]->calls = safe_realloc(parent_array[parent_array_index]->calls,
                                                                ++(parent_array[parent_array_index]->calls_size) * sizeof(struct option *));

         parent_array[parent_array_index]->calls[parent_array[parent_array_index]->calls_size - 1] = safe_malloc(sizeof(struct option));
         parent_array[parent_array_index]->calls[parent_array[parent_array_index]->calls_size - 1]->parent = parent_array[parent_array_index];
         copy_option(parent_array[parent_array_index]->calls[parent_array[parent_array_index]->calls_size - 1], all_options[i]);
      }
      else if (all_options[i]->type == 0)
      {
         parent_array[parent_array_index]->puts = safe_realloc(parent_array[parent_array_index]->puts,
                                                               ++(parent_array[parent_array_index]->puts_size) * sizeof(struct option *));

         parent_array[parent_array_index]->puts[parent_array[parent_array_index]->puts_size - 1] = safe_malloc(sizeof(struct option));
         parent_array[parent_array_index]->puts[parent_array[parent_array_index]->puts_size - 1]->parent = parent_array[parent_array_index];
         copy_option(parent_array[parent_array_index]->puts[parent_array[parent_array_index]->puts_size - 1], all_options[i]);
      }

      free(all_options[i]);
   }

   return;
}

void copy_option(struct option *new_option, struct option *old)
{
   memset(new_option->ticker, 0, 10);
   strcpy(new_option->ticker, old->ticker);

   new_option->type = old->type; // call if TRUE, put is FALSE
   new_option->expiration_date = old->expiration_date;
   new_option->days_til_expiration = old->days_til_expiration;
   new_option->strike = old->strike;
   new_option->volume = old->volume;
   new_option->open_interest = old->open_interest;
   new_option->bid = old->bid;
   new_option->ask = old->ask;
   new_option->last_price = old->last_price;
   new_option->percent_change = old->percent_change;
   new_option->in_the_money = old->in_the_money;
   new_option->implied_volatility = old->implied_volatility;
   new_option->iv20 = old->iv20;
   new_option->iv50 = old->iv50;
   new_option->iv100 = old->iv100;

   // protects against NULL values
   if (old->theta && old->beta && old->gamma && old->vega)
   {
      new_option->theta = old->theta;
      new_option->beta = old->beta;
      new_option->gamma = old->gamma;
      new_option->vega = old->vega;
   }

   new_option->weight = old->weight;
}

/* Gathers all options data from database */
void gather_options_data(void)
{
   int rc;
   char *error_msg;
   sqlite3 *db;
   char *sql = "SELECT * FROM optionsData";

   rc = sqlite3_open("optionsData", &db);

   all_options_size = 0;
   all_options = malloc(sizeof(struct option *));

   if (rc != SQLITE_OK)
   {
      fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);

      return;
   }

   rc = sqlite3_exec(db, sql, options_callback, 0, &error_msg);

   sqlite3_close(db);

   return;
}

/* Gathers all historical pricing data from database */
void gather_data(void)
{
   int rc;
   char *error_msg;
   sqlite3 *db;
   char *sql = "SELECT * FROM historicalPrices";

   historical_array_size = 0;
   historical_price_array = malloc(sizeof(struct historical_price *));

   rc = sqlite3_open("historicalPrices", &db);

   if (rc != SQLITE_OK)
   {
      fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);

      return;
   }

   rc = sqlite3_exec(db, sql, historical_price_callback, 0, &error_msg);

   sqlite3_close(db);

   return;
}

/* 
 * Effectively removes options from the list if the volume/open interest isn't up to standards
 * Also screens for bid x ask spread
 */
void screen_volume_oi_baspread(struct parent_stock **parent_array, int parent_array_size)
{
   /*
    * ALGORITHM/REQUIREMENTS:
    * 
    * if dte <= 30, min_vol = 2000 / (dte / 2)
    * 
    * The closer to the DTE, the more open interest/volume there must be. However, there may never 
    * be any otion that has no volume (NOT OPEN INTEREST) below 10, no matter the DTE.
    * If within one month of expiration, the option must have at least 134 volume on the day
    */
   char removed;
   unsigned short dte;
   unsigned int outter_i, inner_i, volume, open_interest;
   int min_vol;

   for (outter_i = 0; outter_i < parent_array_size; outter_i++)
   {
      parent_array[outter_i]->num_open_calls = parent_array[outter_i]->calls_size;
      parent_array[outter_i]->num_open_puts = parent_array[outter_i]->puts_size;
      large_price_drop(parent_array[outter_i]);

      for (inner_i = 0; inner_i < parent_array[outter_i]->calls_size; inner_i++)
      {
         removed = FALSE;
         volume = parent_array[outter_i]->calls[inner_i]->volume;
         open_interest = parent_array[outter_i]->calls[inner_i]->open_interest;
         dte = parent_array[outter_i]->calls[inner_i]->days_til_expiration;

         if (volume < 10 || open_interest < 100)
            removed = TRUE;
         if (!removed && dte < 30)
         {
            // honestly, this is a random equation. It basically says the closer to the dte,
            // the larger the volume must be, so if dte = 2, volume must be at least 2000
            min_vol = 2000 / (dte / 2);

            if (volume < min_vol)
               removed = TRUE;
         }
         if (!removed && bid_ask_spread(parent_array[outter_i]->calls[inner_i]))
            removed = TRUE;

         if (removed)
         {
            parent_array[outter_i]->calls[inner_i] = NULL;
            parent_array[outter_i]->num_open_calls--;
         }
      }

      for (inner_i = 0; inner_i < parent_array[outter_i]->puts_size; inner_i++)
      {
         removed = FALSE;
         volume = parent_array[outter_i]->puts[inner_i]->volume;
         open_interest = parent_array[outter_i]->puts[inner_i]->open_interest;
         dte = parent_array[outter_i]->puts[inner_i]->days_til_expiration;

         if (volume < 10 || open_interest < 100)
            removed = TRUE;
         if (!removed && dte < 30)
         {
            // honestly, this is a random equation. It basically says the closer to the dte,
            // the larger the volume must be, so if dte = 2, volume must be at least 2000
            min_vol = 1000 / (dte / 2);

            if (volume < min_vol)
               removed = TRUE;
         }

         if (removed)
         {
            parent_array[outter_i]->puts[inner_i] = NULL;
            parent_array[outter_i]->num_open_calls--;
         }
      }
   }

   return;
}

/* Returns TRUE if it is beyond MAX_BID_ASK_ERROR */
int bid_ask_spread(struct option *opt)
{
   float bid, ask, dif, mean, error;

   bid = opt->bid;
   ask = opt->ask;

   mean = (bid + ask) / 2;

   dif = bid - ask;
   error = dif / mean;

   if (error > MAX_BID_ASK_ERROR)
      return TRUE;

   return FALSE;
}

/* Calculates all basic data on calls and puts */
void calc_basic_data(struct parent_stock **parent_array, int parent_array_size)
{
   int outter_i, inner_i;

   for (outter_i = 0; outter_i < parent_array_size; outter_i++)
   {
      for (inner_i = 0; inner_i < parent_array[outter_i]->calls_size; inner_i++)
      {
         if (parent_array[outter_i]->calls[inner_i] != NULL)
         {
            perc_from_strike(parent_array[outter_i]->calls[inner_i]);
            perc_from_ivs(parent_array[outter_i]->calls[inner_i]);
            one_std_deviation(parent_array[outter_i]->calls[inner_i]);
            iv_below(parent_array[outter_i]->calls[inner_i]);

            if ((parent_array[outter_i]->calls[inner_i]->weight + parent_array[outter_i]->calls[inner_i]->parent->weight) > 200)
               printf("%s\t%f\t%f\n", parent_array[outter_i]->calls[inner_i]->ticker, parent_array[outter_i]->calls[inner_i]->strike, parent_array[outter_i]->calls[inner_i]->weight + parent_array[outter_i]->calls[inner_i]->parent->weight);
         }
      }

      for (inner_i = 0; inner_i < parent_array[outter_i]->puts_size; inner_i++)
      {
         if (parent_array[outter_i]->puts[inner_i] != NULL)
         {
            perc_from_strike(parent_array[outter_i]->puts[inner_i]);
            perc_from_ivs(parent_array[outter_i]->puts[inner_i]);
            one_std_deviation(parent_array[outter_i]->puts[inner_i]);
            iv_below(parent_array[outter_i]->puts[inner_i]);

            if ((parent_array[outter_i]->puts[inner_i]->weight + parent_array[outter_i]->puts[inner_i]->parent->weight) > 200)
               printf("%s\t%f\t%f\n", parent_array[outter_i]->puts[inner_i]->ticker, parent_array[outter_i]->puts[inner_i]->strike, parent_array[outter_i]->puts[inner_i]->weight + parent_array[outter_i]->puts[inner_i]->parent->weight);
         }
      }
   }

   return;
}

/* Finds percent from stock's current price */
void perc_from_strike(struct option *opt)
{
   float dif, opt_strike, stock_curr_price;

   opt_strike = opt->strike;
   stock_curr_price = opt->parent->curr_price;

   dif = opt_strike - stock_curr_price;
   opt->perc_from_strike = dif / stock_curr_price;

   return;
}

/* Calculates the distance from all of the different IV levels */
void perc_from_ivs(struct option *opt)
{
   float dif, opt_hv;

   opt_hv = opt->implied_volatility;

   dif = opt->iv20 - opt_hv;
   opt->perc_from_iv20 = dif / opt->iv20;
   dif = opt->iv50 - opt_hv;
   opt->perc_from_iv50 = dif / opt->iv50;
   dif = opt->iv100 - opt_hv;
   opt->perc_from_iv100 = dif / opt->iv100;

   return;
}

/* Formula: curr_price x iv x SQRT(dte/365) = 1 std deviation */
void one_std_deviation(struct option *opt)
{
   double iv, dte_sqrt;
   float range[2];

   if (opt->days_til_expiration <= 30)
      iv = opt->iv20;
   else if (opt->days_til_expiration <= 365 / 4)
      iv = opt->iv50;
   else
      iv = opt->iv100;

   dte_sqrt = sqrt((double)opt->days_til_expiration / 365);
   opt->one_std_deviation = opt->parent->curr_price * (iv / 100) * dte_sqrt;

   range[0] = opt->parent->curr_price - opt->one_std_deviation;
   range[1] = opt->parent->curr_price + opt->one_std_deviation;

   if (opt->strike > range[0] && opt->strike < range[1])
   {
      opt->weight += 100;  // idk about this one, ryan. you're just being lazy here
   }

   return;
}

/* 
 * Formula:
 * weight = (ivx - iv / ivx) * 100
 */
void iv_below(struct option *opt)
{
   float iv, dif;

   iv = opt->implied_volatility;

   if (opt->days_til_expiration <= 30)
      dif = (opt->iv20 - iv) / opt->iv20;
   else if (opt->days_til_expiration <= 365 / 4)
      dif = (opt->iv50 - iv) / opt->iv50;
   else
      dif = (opt->iv100 - iv) / opt->iv100;

   opt->weight += (dif * 100);

   return;
}

void perc_from_high_low(struct parent_stock *stock)
{

}

void yearly_high_low(struct parent_stock *stock)
{

   return;
}

void price_trend(struct parent_stock *stock)
{

   // stock->calls_weight +=
   // stock->puts_weight -=

   // stock->calls_weight -=
   // stock->puts_weight +=
   return;
}

void avg_stock_close(struct parent_stock *stock)
{
   int i;
   float total;

   total = 0;

   for (i = 0; i < stock->price_array_size; i++)
      total += stock->prices_array[i]->close;

   stock->avg_close = total / i;

   return;
}

void large_price_drop(struct parent_stock *stock)
{
   int i, starting_index;
   float change, previous;

   starting_index = (stock -> price_array_size <= 100 ? 0 : stock->price_array_size - 30);

   previous = 0;

   for (i = starting_index; i < stock->price_array_size; i++)
   {
      if (i == 0 || i == starting_index)
      {
         previous = stock->prices_array[i]->close;
         continue;
      }

      change = (previous - stock->prices_array[i]->close) / previous;

      change = (change < 0 ? change *= -1 : change);  // since abs() only works on ints

      if (change >= 7.5)
         stock->weight += 100 * (change / 7.5);
   }

   // finding total change over the period
   change = (stock->prices_array[starting_index]->close - stock->prices_array[i-1]->close) / stock->prices_array[starting_index]->close;
   change = (change < 0 ? change *= -1 : change);  // since abs() only works on ints

   if (change >= 10)
      stock->weight += 100 * (change / 10);

   return;
}

void find_curr_stock_price(struct parent_stock *stock)
{
   int size = stock->price_array_size - 1;

   stock->curr_price = stock->prices_array[size]->close;

   return;
}

int historical_price_callback(void *NotUsed, int argc, char **argv, char **azColName)
{
   historical_price_array = realloc(historical_price_array, ++historical_array_size * (sizeof(struct historical_price *)));
   historical_price_array[historical_array_size - 1] = malloc(sizeof(struct historical_price));

   memset(historical_price_array[historical_array_size - 1]->ticker, 0, 10);
   strcpy(historical_price_array[historical_array_size - 1]->ticker, argv[0]);
   historical_price_array[historical_array_size - 1]->date = atof(argv[1]);
   historical_price_array[historical_array_size - 1]->open = atof(argv[2]);
   historical_price_array[historical_array_size - 1]->low = atof(argv[3]);
   historical_price_array[historical_array_size - 1]->high = atof(argv[4]);
   historical_price_array[historical_array_size - 1]->close = atof(argv[5]);
   historical_price_array[historical_array_size - 1]->volume = atof(argv[6]);

   return 0;
}

int options_callback(void *NotUsed, int argc, char **argv, char **azColName)
{
   all_options = realloc(all_options, ++all_options_size * (sizeof(struct option *)));
   all_options[all_options_size - 1] = malloc(sizeof(struct option));

   memset(all_options[all_options_size - 1]->ticker, 0, 10);
   strcpy(all_options[all_options_size - 1]->ticker, argv[0]);

   all_options[all_options_size - 1]->type = ((0 == strcmp(argv[1], "Call")) ? TRUE : FALSE); // call if TRUE, put is FALSE
   all_options[all_options_size - 1]->expiration_date = atof(argv[2]);
   all_options[all_options_size - 1]->days_til_expiration = atof(argv[3]);
   all_options[all_options_size - 1]->strike = atof(argv[4]);
   all_options[all_options_size - 1]->volume = atof(argv[5]);
   all_options[all_options_size - 1]->open_interest = atof(argv[6]);
   all_options[all_options_size - 1]->bid = atof(argv[7]);
   all_options[all_options_size - 1]->ask = atof(argv[8]);
   all_options[all_options_size - 1]->last_price = atof(argv[9]);
   all_options[all_options_size - 1]->percent_change = atof(argv[10]);
   all_options[all_options_size - 1]->in_the_money = (strcmp(argv[11], "True") ? TRUE : FALSE);
   all_options[all_options_size - 1]->implied_volatility = atof(argv[12]);
   all_options[all_options_size - 1]->iv20 = atof(argv[13]);
   all_options[all_options_size - 1]->iv50 = atof(argv[14]);
   all_options[all_options_size - 1]->iv100 = atof(argv[15]);

   // protects against NULL values
   if (argv[16] && argv[17] && argv[18] && argv[19])
   {
      all_options[all_options_size - 1]->theta = atof(argv[16]);
      all_options[all_options_size - 1]->beta = atof(argv[17]);
      all_options[all_options_size - 1]->gamma = atof(argv[18]);
      all_options[all_options_size - 1]->vega = atof(argv[19]);
   }

   all_options[all_options_size - 1]->weight = 0;

   return 0;
}
