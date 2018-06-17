#ifndef _H_SCREENER
#define _H_SCREENER

/* Goes thorugh all tickers, looking for low IV, low prices, but historical prices/IV
 * Target a few months OTM
 * Maybe allow the user to adjust their timeframe
 */


/* DETAILS OF ALGORITHM/HOW OPTIONS ARE SCREENED
 *
 * Everything is weighted!
 * 
 * Ratio of which IV should be below: (heavier weighting if IV is below)
 * How close to the strike price the current price is (heavier weighting for closer options)
 * Find the trend of the stock (heavier weighting for the more a stock has a stable trend)
 * Find support/resistance points (heavier weighing for approaching these levels)
 * Find catalyst event (typically earnings) (heavier weighting for the close the event is, or perhaps closer to experation)
 * Smaller bid/ask spread (heavier weighting)
 * Take into account analysts predictions
 * Profitability/revenue, growth estimates (heavier weighting for the better numbers)
 * High beta (heavier weighting)
 * 
 */

#define REGULAR        1
#define NEW_STOCKS     2
#define APPEND_STOCKS  3

#define FALSE 0
#define TRUE  1

char **parse_args(int argc, char *argv[], int *mode, int *tl_size);
void free_all(char **tick_list, int *tl_size);

#endif
