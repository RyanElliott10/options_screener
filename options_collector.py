# scans all NASDAQ, NYSE, AMEX stocks with average volume > 5,000,000 and are optionable
# Use Yahoo Finance's option chain feature
# Check if it's optionable here
# Gathers all data on all calls/puts
# Find place to gather IV, Beta, Theta, Gamma, Vega
# Store all data in SQLite database
# Collect historical IV too
# And all previous prices (whatever you did in the short_squeeze_screener)
# compare to S&P 500 week-change (Yahoo Finance Stats page)

# FOR OPTIONS CHAIN INFO USE ONE OF THESE TWO, WHICHEVER HAS A STRAIGHT UP XML TO PULL FROM
# https://www.nasdaq.com/symbol/aapl/option-chain

# PLACES TO FIND DATA:
# OPTIONS PRICING, HISTORICAL STOCK PRICES, ANALYSTS PREDCITIONS: YAHOO FINANCE
# HISTORICAL VOLATILITY: http://www.optionstrategist.com/calculators/free-volatility-data

import os
import sys
import csv
import requests
import urllib.request

def download_files():
	urls = [
		"https://www.nasdaq.com/screening/companies-by-name.aspx?letter=0&exchange=nasdaq&render=download",
		"https://www.nasdaq.com/screening/companies-by-name.aspx?letter=0&exchange=nyse&render=download",
		"https://www.nasdaq.com/screening/companies-by-name.aspx?letter=0&exchange=amex&render=download"]
	names = ["nasdaq.csv", "nyse.csv", "amex.csv"]

	valid_tickers = []

	for i in range(3):
		resp = requests.get(urls[i])

		# print(resp.content)
		output = open(names[i], 'wb')
		output.write(resp.content)
		output.close()

	for outter in range(3):
		with open(names[outter], 'rt') as filename:
			reader = csv.reader(filename, 'excel')
			for inner, row in enumerate(reader):
				# only focus on companies with market cap > $1 billion
				if ('B' in row[3]):	
					valid_tickers.append(row[0])

	return sorted(valid_tickers)


def main():
	tickers = download_files()
	print(tickers)

if __name__ == "__main__":
    main()