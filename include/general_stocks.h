#ifndef _H_GENERAL_STOCKS
#define _H_GENERAL_STOCKS

#include "screener.h"

long historical_array_size;
long all_options_size;
struct HistoricalPrice **historical_price_array;
struct option **all_options;

// collecting historical prices
int historical_price_callback(void *NotUsed, int argc, char **argv, char **azColName);
struct ParentStock **gather_tickers(long *pa_size);
void gather_data(void);

// historical price functionality
void average_perc_change(struct ParentStock *stock);
void perc_from_high_low(struct ParentStock *stock);
void large_price_drop(struct ParentStock *stock);
void price_trend(struct ParentStock *stock);

// write function to find all stocks that have had a 7.5% drop or gain in one or two days the past 10 days
//

void find_curr_stock_price(struct ParentStock *stock);
void avg_stock_close(struct ParentStock *stock);

#endif
