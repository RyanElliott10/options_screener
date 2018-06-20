# scans all NASDAQ, NYSE, AMEX stocks with average volume > 5,000,000 and are optionable
# Use Yahoo Finance's option chain feature
# Check if it's optionable here
# Gathers all data on all calls/puts
# Find place to gather IV, Beta, Theta, Gamma, Vega
# Collect historical IV too
# Store all data in SQLite database
# And all previous prices (whatever you did in the short_squeeze_screener)

# STILL NEEDED: Historical stock prices, beta/theta/gamma/vega
# DONE: Historical Volatility, calls/puts strike, bid x ask, change, expiration, oi, volume, IV

# timestamp, volume, open, low, high, close
# "thing":[

import os
import sys
import csv
import time
import operator
import requests
import progressbar
import requests_cache
import concurrent.futures
import urllib.request
from bs4 import BeautifulSoup as soup

class historical_price:
	def __init__(self, date=None, open=None, close=None, low=None,
					 high=None, volume=None):
		self.date   = date
		self.open   = open
		self.close  = close
		self.low    = low
		self.high   = high
		self.volume = volume

class option:
	def __init__(self, strike=None, dte=None, theta=None, beta=None, vega=None,
					 gamma=None, iv=None, volume=None, oi=None):
		self.strike = strike
		self.dte    = dte
		self.theta  = theta
		self.beta   = beta
		self.vega   = vega
		self.gamma  = gamma
		self.volume = volume
		self.oi     = oi
		self.iv     = iv

class ticker:
	def __init__(self, symbol=None, iv20=None, iv50=None, iv100=None, curr_iv=None):
		self.symbol = symbol
		self.iv20   = iv20
		self.iv50   = iv50
		self.iv100  = iv100

		# formatted as list of historical_price_objs
		self.prices = []

		# formatted as a list of option_objects
		self.calls = []
		self.puts  = []

		self.dates = []
		self.date_pages = []

		# formatted as timestamp, volume, open, low, high, close
		self.historical_data = [[], [], [], [], [], []]
		self.historical_prices_page = None

class util:
	def __init__(self):
		self.tickers = []
		self.ticker_dict = {}
	
	def download_files(self):
		urls = [
			"https://www.nasdaq.com/screening/companies-by-name.aspx?letter=0&exchange=nasdaq&render=download",
			"https://www.nasdaq.com/screening/companies-by-name.aspx?letter=0&exchange=nyse&render=download",
			"https://www.nasdaq.com/screening/companies-by-name.aspx?letter=0&exchange=amex&render=download"]
		names = ["nasdaq.csv", "nyse.csv", "amex.csv"]

		for i, url in enumerate(urls):
			resp = requests.get(url)

			output = open(names[i], 'wb')
			output.write(resp.content)
			output.close()

		for name in names:
			with open(name, 'rt') as filename:
				reader = csv.reader(filename, 'excel')
				for inner, row in enumerate(reader):
					# only focus on companies with market cap > $1 billion
					if ('B' in row[3]):	
						self.tickers.append(row[0])

	def gather_hv(self):
		url = "http://www.optionstrategist.com/calculators/free-volatility-data"

		curr_page = soup(requests.get(url, headers={'User-Agent': 'Custom'}).text, 'html.parser')
		pre = curr_page.findAll("pre")

		info_list = self.extract_individuals(pre)

		# formatted as: 
		# key: ticker
		# value: list (20 day, 50 day, 100 day)
		self.extract_hv(info_list)
		self.clean_dict()

	def extract_hv(self, info_list):
		for info_obj in info_list:
			count = 0
			text = ""
			symbol = ""
			ontext = True

			for char in info_obj:
				# sets ticker, intializes dictionary
				if ((ord(char) == 32) and (count == 0)):
					symbol = text
					self.ticker_dict[text] = [None, None, None]
					text = ""
					ontext = False
					count += 1
				elif ((ord(char) == 32) and (count > 0) and ontext):
					self.ticker_dict[symbol][count-1] = float(text)
					text = ""
					ontext = False
					count += 1

					if (count == 4):
						break

				if (ord(char) != 32 and (char is not '$')):
					text += char
					ontext = True

	def extract_individuals(self, pre):
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

	def clean_dict(self):
		del_list = []

		# logic to remove anything not in either of the lists
		for symbol in self.tickers:
			if (symbol not in self.ticker_dict):
				del_list.append(symbol)
		for symbol in del_list:
			self.tickers.remove(symbol)
		
		del_list = []
		for symbol in self.ticker_dict:
			if (symbol not in self.tickers):
				del_list.append(symbol)
		for symbol in del_list:
			del self.ticker_dict[symbol]

	def get_options(self, tick):
		count = 0
		text = ""
		
		url = "https://finance.yahoo.com/quote/{0}/options?p={0}".format(tick.symbol)
		page = soup(requests.get(url, headers={'User-Agent': 'Custom'}).text, 'html.parser')
		
		start_index = page.text.index("expirationDates")
		end_index = page.text.index("hasMiniOptions")

		dates_txt = page.text[start_index+18:end_index-3]

		for char in dates_txt:
			if (char == ','):
				tick.dates.append(text)
				text = ""
			else:
				text += char
		tick.dates.append(text)

	# Utility function to prefetch webpages concurrently
	def prefetch_webpages(self):
		requests_cache.install_cache('cache', backend='sqlite', expire_after=3600)

		with concurrent.futures.ThreadPoolExecutor(max_workers=50) as executor:
			future_to_tickers = {executor.submit(self.get_pages, tick): tick for tick in self.tickers[:10]}
			bar = progressbar.ProgressBar(max_value=len(self.tickers))
			foo = 0

			for future in concurrent.futures.as_completed(future_to_tickers):
				if foo >= len(self.tickers):
					foo = foo - 1

				foo += 1
				bar.update(foo)

		bar.update(len(self.tickers))
		
		print()

	def get_pages(self, tick):
		url = ("https://finance.yahoo.com/quote/{0}/options?p={0}".format(tick.symbol))
		sess = requests.session()
		base = soup(sess.get(url, headers={'User-Agent': 'Custom'}).text, 'html.parser')

		url = ("https://query1.finance.yahoo.com/v8/finance/chart/{0}?formatted=true&crumb=h.8f9xpa6IF&lang=en-US&region=US&interval=1d&events=div%7Csplit&range=1y&corsDomain=finance.yahoo.com".format(tick.symbol))
		sess = requests.session()
		tick.historical_prices_page = soup(sess.get(url, headers={'User-Agent': 'Custom'}).text, 'html.parser')

		self.get_options(tick)

		for date in tick.dates:
			cont = False
			count = 0
			url = "https://query1.finance.yahoo.com/v7/finance/options/{0}?formatted=true&crumb=R6vBcFmQGju&lang=en-US&region=US&date={1}&corsDomain=finance.yahoo.com".format(tick.symbol, date)
			
			while (not cont):
				try:
					page = soup(sess.get(url, headers={'User-Agent': 'Custom'}).text, 'html.parser')
					cont = True
				except:
					if (count == 10):
						time.sleep(.5)
						
			tick.date_pages.append(page)

	def gather_prices(self):
		for tick in self.tickers:
			self.parse_prices_page(tick.historical_prices_page, tick)

	def parse_prices_page(self, page, tick):
		index_list = [
			'"timestamp":[',
			'"volume":[',
			'"open":[',
			'"low":[',
			'"high":[',
			'"adjclose":[{"adjclose":'
		]

		for i in range(6):
			index_str = index_list[i]
			index = page.text.index(index_str)
			num = ""
			start = False

			for char in page.text[index:]:
				if (char.isdigit()):
					start = True
				if (start):
					if (char != ']'):
						if (char == ','):
							tick.historical_data[i].append(float(num))
							num = ""
						else:
							num += char
					else:
						tick.historical_data[i].append(float(num))
						break
			print(tick.historical_data[i])
			
			tick.historical_data[i].append(num)
		
		print(tick.symbol)

	def main(self):
		remove_list = []

		self.download_files()
		self.gather_hv()

		self.tickers = []

		for symbol in self.ticker_dict:
			self.tickers.append(ticker(symbol, self.ticker_dict[symbol][0], self.ticker_dict[symbol][1], self.ticker_dict[symbol][2]))

		print("Prefetching webpages...")
		self.prefetch_webpages()

		for tick in self.tickers:
			if (len(tick.dates) == 0 or tick.historical_prices_page is None):
				remove_list.append(tick)
			
		for tick in remove_list:
			self.tickers.remove(tick)

		self.gather_prices()

if __name__ == "__main__":
	utilobj = util()
	utilobj.main()