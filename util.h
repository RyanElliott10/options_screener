#ifndef _H_UTIL
#define _H_UTIL

#include "screener.h"

void gather_tickers(void);
void format_historical_prices(struct parent_stock stock);
void gather_data(void);
int callback(void *NotUsed, int argc, char **argv, char **azColName);

void price_trend(struct parent_stock stock);
void bid_ask_spread(struct option opt);
void perc_from_strike(struct option opt);
void yearly_high_low(struct parent_stock stock);

void perc_from_ivs(struct option opt);
void iv_range(struct option opt);

void find_curr_price(struct parent_stock stock);
void avg_open_close(struct parent_stock stock);

#endif
