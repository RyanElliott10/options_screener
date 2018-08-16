# options_screener
A stock options screener developed in C and Python. Data is collected via the Python options_collector.py script using BeautifulSoup4 and the Requests library to scrape data from Yahoo! Finance. Collects data ranging from historical price and volume to the pricing, volume, open interest, etc. of call and put options.

This data is then sent through the C program to be assigned weights to the option contracts and essentially weed out the illiquid and unprofitable contracts while propagating the very liquid and likely profitable contracts to the higher ranks.

## Execution Details
The entire screener is ran through the C program, screener.(c/h/o). The program will first prompt the user to decide if they want to collect the recent data. If the user decides to fetch the most current data, screener.c will fork and exec options_collector.py, initalizing the data scraping and storage in a SQLite database. To reduce the run time of the data collection, options_collector.py will utilize website prefetching. Therefore, it is imperative users install the list of required Python libraries prior to execution.

Following the completion of the Python script, screener.c will pull the data from the SQLite database and perform the appropriate screening. Once the screener is complete, the user will be asked two questions: the minimum weight to view and maximum cost of each contract. Any and all contracts that fall within the specified range will be printed to the terminal for the user to review.

## Instructions
### To Compile:
  `$ make [ target ]`
### To Run:
  `$ ./screener`
### Required Python Libraries
  - requests
  - progressbar
  - futures
  - urllib3
  - BeatifulSoup4
