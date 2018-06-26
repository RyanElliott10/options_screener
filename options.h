#ifndef _H_OPTIONS
#define _H_OPTIONS

#include "screener.h"

#define MAX_BID_ASK_ERROR 0.075

// collecting options
void screen_volume_oi_baspread(struct parent_stock **parent_array, int parent_array_size);  // done
void gather_options(struct parent_stock **parent_array, long parent_array_size);  // done
int options_callback(void *NotUsed, int argc, char **argv, char **azColName);  // done
void copy_option(struct option *new_option, struct option *old);  // done
void gather_options_data(void);  // done

// sepcific option functionality
void calc_basic_data(struct parent_stock **parent_array, int parent_array_size, float max_option_price, float min_weight);  // done
void one_std_deviation(struct option *opt);  // weight
void perc_from_strike(struct option *opt);  // done
int bid_ask_spread(struct option *opt);  // done
void perc_from_ivs(struct option *opt);  // done
void iv_below(struct option *opt);  // done

#endif
