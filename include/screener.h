#ifndef _H_SCREENER
#define _H_SCREENER

/* 
 * Goes thorugh all tickers, looking for low IV, low prices, but historical prices/IV
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
 * dte (higher weighting for options further out)
 * High beta (heavier weighting)
 */

/*
 * STRUCTURE:
 * 
 * EACH OPTION WILL HAVE A STRUCT WITH ALL OF THE CONTENTS OF THE DATA IN DATAOPTIONS DB
 * THEY WILL HAVE A POINTER TO THE PARENT TICKER WITH LISTS FULL OF HISTORICAL OPEN, CLOSE, HIGH, LOW, VOLUME, DATE ETC.
 * THERE WILL BE ONE LIST IN THE PARENT, WHICH WILL CONTAIN A STRUCT CONTAINING THE DATA, OPEN, CLOSE, HIGH, LOW, VOLUME, ETC. FOR THE DAY
 * EACH PARENT WILL HAVE A LIST OF CALLS AND PUTS, FULL OF THE STRUCTS USED FOR OPTIONS
 */

/*
 * LIKELY FUNCTIONS TO HAVE:
 * 
 * DETERMINE TREND OF PRICES:
 *    - WEIGHT IT SO THAT THE LONGER THE TREND, THE HIGHER THE WEIGHT
 *    - LOOK FOR RECURRING TRENDS
 *    - FOR LATER ADDITION: 
 *       - TRENDS SURROUNDING EARNINGS
 * 
 * DETERMINE HOW FAR OFF FROM IV20, IV50, AND IV100:
 *    - FOR BUYING:
 *       - THE LOWER IT IS, THE MORE WEIGHT IT IS GIVEN
 *       - LOOK FOR ANAMOLIES:
 *          - IF THERE IS ONE RANDOM OPTION WITH LOW/HIGH IV:
 *             - DO SOMETHING DIFFERENT, IDK YET
 *          - IF THE IV IS ACCURATE
 *    - FOR SELLING:
 *       - THE HIGHER IT IS, THE MORE WEIGHT IT IS GIVEN
 *       - LOOK FOR ANAMOLIES:
 *          - IF THERE IS ONE RANDOM OPTION WITH LOW/HIGH IV:
 *             - DO SOMETHING DIFFERENT, IDK YET
 *          - IF THE IV IS ACCURATE
 * 
 * DETERMINE WHAT PERCENT IT IS AWAY FROM STRIKE:
 *    - CLOSER IT IS, HIGHER THE WEIGHTING
 * 
 * DETERMINE IF VOLUME/OPEN INTEREST ARE GOOD ENOUGH
 * 
 * DETERMINE 52-WEEK LOW/HIGH FOR EACH STOCK:
 *    - A HIGHER WEIGHTING FOR THOSE STOCKS WITH LARGE RANGES (LOOK AT PERCENTS)
 * 
 * DETERMINE IF BID x ASK SPREAD IS GOOD ENOUGH:
 *    - USE SOME PERCENTAGE: 
 *       - NO MORE THAN 5% OF THE LAST PRICE BETWEEN BID x ASK, MAYBE
 *          - THIS MAY BE AN ISSUE WITH LOWER PRICED OPTIONS, THOUGH
 *    - THE SMALLER THE SPREAD, THE HIGHER THE WEIGHT
 * 
 * DO SOMETHING TO FIND THAT THING WHERE IT TELLS YOU THE ODDS A STOCK WILL BE WITHIN A PRICE RANGE IN A GIVEN TIME
 * 
 * LOOK AT THE HIGHS/LOWS OF THE STOCK
 * 
 * LOOK AT THE TOP 10 LARGEST VOLUMES OF THE DAY:
 *    - IDK YET, BUT DO SOMETHING WITH THESE
 */

/* 
 * SETUP:
 * 
 * FIRST, DETERMINE IF VOLUME/OPEN INTEREST ARE GOOD ENOUGH, ASSIGN WEIGHTING
 * DETERMINE BID x ASK SPREAD AND IF IT IS GOOD ENOUGH, ASSIGN WEIGHTING
 * THEN DETERMINE WHAT PERCENT THE CURRENT STOCK IS AWAY FROM THE STRIKE PRICE, ASSIGN WEIGHTING
 * DETERMINE TREND OF PRICES, ASSIGN WEIGHTING
 * FIND 52 WEEK HIGH/LOW OF STOCK
 * DETERMINE AVG OPEN, LOW
 * 
 * DETERMINE HOW FAR OFF FROM IV20, IV50, IV100, ASSIGN WEIGHT
 * FIND RANGE THE STOCK WILL BE WITHIN IN A GIVEN TIME
 * 
 * 
 */

#define REGULAR 1
#define NEW_STOCKS 2
#define APPEND_STOCKS 3
#define TICK_SIZE 10
#define MIN_VOL_LENGTH 10

#define FALSE 0
#define TRUE 1

struct option {
   struct ParentStock *parent; // pointer to parent stock
   char ticker[10];             // ticker symbol
   int type;                    // call/put (call = TRUE, put = FALSE)
   long expiration_date;        // in epoch time
   int days_til_expiration;
   float strike;
   long volume;
   long open_interest;
   float bid;
   float ask;
   float last_price;
   float percent_change;
   int in_the_money; // TRUE/FALSE
   float implied_volatility;
   float iv20;
   float iv50;
   float iv100;
   float theta;
   float beta;
   float gamma;
   float vega;
   float weight; // weight for the specigic option
   float perc_from_strike;
   float perc_from_iv20;
   float perc_from_iv50;
   float perc_from_iv100;
   float one_std_deviation;
};

struct ParentStock {
   struct option **calls;                  // list of all calls associated with stock
   struct option **puts;                   // list of all puts associated with stock
   struct HistoricalPrice **prices_array; // list of all prices
   long prices_array_size;
   int calls_size;
   int num_open_calls;
   int puts_size;
   int num_open_puts;
   char ticker[10]; // ticker symbol
   float yearly_high;
   float yearly_low;
   float curr_price;
   float avg_open;
   float avg_close;
   float perc_from_year_high;
   float perc_from_year_low;
   float weight;       // weight to be given to ever option on stock
   float calls_weight; // weight to be given to every call of original stock
   float puts_weight;  // weight to be given to every put of original stock
};

struct HistoricalPrice {
   char ticker[TICK_SIZE];
   long date;
   float open;
   float low;
   float high;
   float close;
   long volume;
};

void print_data(struct ParentStock **parent_array, int parent_array_size, float max_option_price, float min_weight, int fd);
void find_min_vol(struct option **largest_volumes, int *min_vol, int *min_vol_index);
void print_large_volumes(struct ParentStock **parent_array, int parent_array_size);
void free_parent_array(struct ParentStock **parent_array, int parent_array_size);
void find_averages(struct ParentStock **parent_array, int parent_array_size);
int callback(void *NotUsed, int argc, char **argv, char **azColName);
char **parse_args(int argc, char *argv[], int *mode, int *ta_size);
void free_tick_array(char **tick_array, int ta_size);

#endif
