# options_screener

A stock options screener developed in C and Python. Data is collected via the Python options_collector.py script using BeautifulSoup4 and the Requests library to scrape data from Yahoo! Finance. Collects data ranging from historical price and volume to the pricing, volume, open interest, etc. of call and put options.

This data is then sent through the C program to be assigned weights to the option contracts and essentially weed out the illiquid and unprofitable contracts while propagating the very liquid and likely profitable contracts to the higher ranks.

Nitty Gritty Details
