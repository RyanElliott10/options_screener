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
#include "options.h"
#include "general_stocks.h"
#include "safe.h"

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

	opt->weight += error * 50;

	return FALSE;
}

/* Calculates all basic data on calls and puts */
void calc_basic_data(struct parent_stock **parent_array, int parent_array_size, float max_option_price, float min_weight)
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

	opt->weight += 100 - (opt->perc_from_strike * 100);

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
	float range[2], strike, curr_price;

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

	// it it's a call
	// if (opt->type == 1)
	// {
	// 	if (curr_price )
	// }
	// else  // put
	// {
	// }

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
