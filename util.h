#ifndef _H_UTIL
#define _H_UTIL

#include "screener.h"

// dealing with historical prices/not options
int historical_price_callback(void *NotUsed, int argc, char **argv, char **azColName);
void format_historical_prices(struct parent_stock stock);
struct parent_stock **gather_tickers(long *pa_size);
void yearly_high_low(struct parent_stock stock);
void price_trend(struct parent_stock stock);
void perc_from_strike(struct option opt);
void bid_ask_spread(struct option opt);
void gather_data(void);

// generally, collecting and screening options
void screen_volume_oi(struct parent_stock **parent_array, float parent_array_size);
void gather_options(struct parent_stock **parent_array, long parent_array_size);
int options_callback(void *NotUsed, int argc, char **argv, char **azColName);
void copy_option(struct option *new_option, struct option *old);
void gather_options_data(void);

// sepcific option functionality
void perc_from_ivs(struct option opt);
void iv_range(struct option opt);

void find_curr_price(struct parent_stock stock);
void avg_open_close(struct parent_stock stock);

#endif
