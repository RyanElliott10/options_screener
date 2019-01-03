#include <math.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sqlite3.h>
#include <sys/types.h>

#include "../include/screener.h"
#include "../include/options.h"
#include "../include/general_stocks.h"
#include "../include/safe.h"

/* Returns TRUE if it is beyond MAX_BID_ASK_ERROR */
int bid_ask_spread(struct option *opt) {
	float bid, ask, dif, mean, error;

	bid = opt->bid;
	ask = opt->ask;

	mean = (bid + ask) / 2;

	dif = ask - bid;
	error = dif / mean;

	if (error > MAX_BID_ASK_ERROR)
		return TRUE;

	opt->weight += error * 50;

	return FALSE;
}

/* Calculates all basic data on calls and puts */
void calc_basic_data(struct ParentStock **parent_array, int parent_array_size, float max_option_price, float min_weight) {
	int outter_i, inner_i;

	for (outter_i = 0; outter_i < parent_array_size; outter_i++) {
		price_trend(parent_array[outter_i]);
		average_perc_change(parent_array[outter_i]);

		for (inner_i = 0; inner_i < parent_array[outter_i]->calls_size; inner_i++) {
			if (parent_array[outter_i]->calls[inner_i] != NULL) {
				perc_from_strike(parent_array[outter_i]->calls[inner_i]);
				perc_from_ivs(parent_array[outter_i]->calls[inner_i]);
				one_std_deviation(parent_array[outter_i]->calls[inner_i]);
				iv_below(parent_array[outter_i]->calls[inner_i]);
				dte_weight(parent_array[outter_i]->calls[inner_i]);
			}
		}

		for (inner_i = 0; inner_i < parent_array[outter_i]->puts_size; inner_i++) {
			if (parent_array[outter_i]->puts[inner_i] != NULL) {
				perc_from_strike(parent_array[outter_i]->puts[inner_i]);
				perc_from_ivs(parent_array[outter_i]->puts[inner_i]);
				one_std_deviation(parent_array[outter_i]->puts[inner_i]);
				iv_below(parent_array[outter_i]->puts[inner_i]);
				dte_weight(parent_array[outter_i]->puts[inner_i]);
			}
		}
	}

	return;
}

/* Finds percent from stock's current price */
void perc_from_strike(struct option *opt) {
	float dif, opt_strike, stock_curr_price;

	opt_strike = opt->strike;
	stock_curr_price = opt->parent->curr_price;

	dif = opt_strike - stock_curr_price;
	opt->perc_from_strike = dif / stock_curr_price * 100;

	if (opt->perc_from_strike <= .05)
		opt->weight += 125 - opt->perc_from_strike;
	else if (opt->perc_from_strike <= .1)
		opt->weight += 75 - opt->perc_from_strike;
	else if (opt->perc_from_strike <= .175)
		opt->weight += 50 - opt->perc_from_strike;

	return;
}

/* Calculates the distance from all of the different IV levels */
void perc_from_ivs(struct option *opt) {
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
void one_std_deviation(struct option *opt) {
	double iv, dte_sqrt;
	float dif, strike, perc, curr_price, range[2];

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

	curr_price = opt->parent->curr_price;
	strike = opt->strike;

	if (curr_price > range[0] && curr_price < range[1]) {
		// if curr = 90, range[0] = 70 range[1] = 110, strike = 74
		// strike - range[0] = 4
		// range = 20
		// dif / range = percent

		dif = strike - range[0];
		perc = dif / opt->one_std_deviation;
		opt->weight += (perc * 100);
	}

	return;
}

/* 
 * Formula:
 * weight = (ivx - iv / ivx) * 100
 */
void iv_below(struct option *opt) {
	float iv, dif;

	iv = opt->implied_volatility;

	if (opt->days_til_expiration <= 30)
		dif = (opt->iv20 - iv) / opt->iv20 * 100;
	else if (opt->days_til_expiration <= 365 / 4)
		dif = (opt->iv50 - iv) / opt->iv50 * 100;
	else
		dif = (opt->iv100 - iv) / opt->iv100 * 100;

	opt->weight += dif;

	return;
}

/* Gives slightly more weight to contracts with more DTE */
void dte_weight(struct option *opt) {
	if (opt->days_til_expiration > 100)
		opt->weight += 100;
	else
		opt->weight += opt->days_til_expiration;
}

int options_callback(void *NotUsed, int argc, char **argv, char **azColName) {
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
	if (argv[16] && argv[17] && argv[18] && argv[19]) {
		all_options[all_options_size - 1]->theta = atof(argv[16]);
		all_options[all_options_size - 1]->beta = atof(argv[17]);
		all_options[all_options_size - 1]->gamma = atof(argv[18]);
		all_options[all_options_size - 1]->vega = atof(argv[19]);
	}

	all_options[all_options_size - 1]->weight = 0;

	return 0;
}

void copy_option(struct option *new_option, struct option *old) {
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
	if (old->theta && old->beta && old->gamma && old->vega) {
		new_option->theta = old->theta;
		new_option->beta = old->beta;
		new_option->gamma = old->gamma;
		new_option->vega = old->vega;
	}

	new_option->weight = old->weight;
}
