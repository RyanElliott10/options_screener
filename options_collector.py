# scans all NASDAQ, NYSE, AMEX stocks with average volume > 5,000,000 and are optionable
# Use Yahoo Finance's option chain feature
# Check if it's optionable here
# Gathers all data on all calls/puts
# Find place to gather IV, Beta, Theta, Gamma, Vega
# Collect historical IV too
# Store all data in SQLite database
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
from bs4 import BeautifulSoup as soup

def download_files():
	urls = [
		"https://www.nasdaq.com/screening/companies-by-name.aspx?letter=0&exchange=nasdaq&render=download",
		"https://www.nasdaq.com/screening/companies-by-name.aspx?letter=0&exchange=nyse&render=download",
		"https://www.nasdaq.com/screening/companies-by-name.aspx?letter=0&exchange=amex&render=download"]
	names = ["nasdaq.csv", "nyse.csv", "amex.csv"]

	valid_tickers = []

	for i in range(3):
		resp = requests.get(urls[i])

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

def gather_data(tickers):
	url = "http://www.optionstrategist.com/calculators/free-volatility-data"

	curr_page = soup(requests.get(url, headers={'User-Agent': 'Custom'}).text, 'html.parser')
	pre = curr_page.findAll("pre")

	info_list = extract_individuals(pre)

	# formatted as: 
	# key: ticker
	# value: list (20 day, 50 day, 100 day)
	ticker_dict = extract_hv(info_list)

	return ticker_dict

def extract_hv(info_list):
	ticker_dict = {}

	for info_obj in info_list:
		count = 0
		text = ""
		ticker = ""
		ontext = True

		for char in info_obj:
			# sets ticker, intializes dictionary
			if ((ord(char) == 32) and (count == 0)):
				ticker = text
				ticker_dict[text] = [None, None, None]
				text = ""
				ontext = False
				count += 1
			elif ((ord(char) == 32) and (count > 0) and ontext):
				ticker_dict[ticker][count-1] = float(text)
				text = ""
				ontext = False
				count += 1

				if (count == 4):
					break

			if (ord(char) != 32 and (char is not '$')):
				text += char
				ontext = True
	
	return ticker_dict
		

def extract_individuals(pre):
	text = ""
	wait = False
	count = 0
	info_list = []

	for char in pre[0].text:
		# 27 is the number of newline chars before first ticker
		if (count < 27 and char is '\n'):
			count += 1
		elif (count >= 27):		# true meat, where we collect the tickers
			# gets rid of extraneous data
			if (char is '@' or char is '*'):
				wait = True
			# if data should be appended
			if (not wait):
				text += char
			#resets wait
			if (wait and char is '\n'):
				wait = False
			elif (char is '\n'):
				info_list.append(text[:len(text)-1])
				text = ""

	return info_list

def main():
	del_list = []

	tickers = download_files()
	ticker_dict = gather_data(tickers)

	# logic to remove anything not in either of the lists
	for ticker in tickers:
		if (ticker not in ticker_dict):
			del_list.append(ticker)
	for ticker in del_list:
		tickers.remove(ticker)
	
	del_list = []
	for ticker in ticker_dict:
		if (ticker not in tickers):
			del_list.append(ticker)
	for ticker in del_list:
		del ticker_dict[ticker]

	# print(len(ticker_dict))

if __name__ == "__main__":
    main()