# scans all NASDAQ, NYSE, AMEX stocks with average volume > 5,000,000 and are optionable
# Use Yahoo Finance's option chain feature
# Check if it's optionable here
# Gathers all data on all calls/puts
# Find place to gather IV, Beta, Theta, Gamma, Vega
# Collect historical IV too
# Store all data in SQLite database
# And all previous prices (whatever you did in the short_squeeze_screener)

# STILL NEEDED: beta/theta/gamma/vega
# DONE: Historical Volatility, Historical stock prices, calls/puts strike, bid x ask, change, expiration, oi, volume, IV

import os
import sys
import csv
import time
import math
import sqlite3
import operator
import requests
import progressbar
import requests_cache
from datetime import date
import concurrent.futures
import urllib.request
from bs4 import BeautifulSoup as soup


class historical_price:
    def __init__(self, date=None, volume=None, open=None, low=None,
                 high=None, close=None):
        self.date = date
        self.volume = volume
        self.open = open
        self.low = low
        self.high = high
        self.close = close


class option:
    def __init__(self, strike=None, dte=None, theta=None, beta=None, vega=None,
                 gamma=None, iv=None, volume=None, open_interest=None, itm=None,
                 bid=None, ask=None, last_price=None, change=None,
                 percent_change=None, expiration=None):
        self.strike = strike
        self.dte = dte
        self.theta = theta
        self.beta = beta
        self.vega = vega
        self.gamma = gamma
        self.volume = volume
        self.open_interest = open_interest
        self.iv = (iv * 100)
        self.itm = itm
        self.bid = bid
        self.ask = ask
        self.last_price = last_price
        self.percent_change = percent_change
        self.change = change
        self.expiration = expiration


class ticker:
    def __init__(self, symbol=None, iv20=None, iv50=None, iv100=None, curr_iv=None):
        self.symbol = symbol
        self.iv20 = iv20
        self.iv50 = iv50
        self.iv100 = iv100

        # formatted as list of historical_price_objs
        self.prices = []

        # formatted as a list of option_objects
        self.calls = []
        self.puts = []

        self.dates = []
        self.date_pages = []

        self.historical_prices_page = None


class util:
    def __init__(self):
        self.tickers = []
        self.ticker_dict = {}

    def download_files(self):
        names = [
            "../include/nasdaq.csv",
            "../include/nyse.csv",
            "../include/amex.csv"
        ]
        urls = [
            "https://www.nasdaq.com/screening/companies-by-name.aspx?letter=0&exchange=nasdaq&render=download",
            "https://www.nasdaq.com/screening/companies-by-name.aspx?letter=0&exchange=nyse&render=download",
            "https://www.nasdaq.com/screening/companies-by-name.aspx?letter=0&exchange=amex&render=download"
        ]

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

    def gather_historical_volatility(self):
        url = "http://www.optionstrategist.com/calculators/free-volatility-data"

        curr_page = soup(requests.get(
            url, headers={'User-Agent': 'Custom'}).text, 'html.parser')
        pre = curr_page.findAll("pre")

        info_list = self.extract_individuals(pre)

        # formatted as:
        # key: ticker
        # value: list (20 day, 50 day, 100 day)
        self.extract_historical_volatility(info_list)
        self.clean_dict()

    def extract_historical_volatility(self, info_list):
        """ Parses historical volatility for each ticker """
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
        """ Extracts historical volatility data for each individual ticker """
        text = ""
        wait = False
        count = 0
        info_list = []

        for char in pre[0].text:
            # 27 is the number of newline chars before first ticker
            if (count < 27 and char is '\n'):
                count += 1
            elif (count >= 27):     # true meat, where we collect the tickers
                # gets rid of extraneous data
                if (char is '@' or char is '*'):
                    wait = True
                # if data should be appended
                if (not wait):
                    text += char
                # resets wait
                if (wait and char is '\n'):
                    wait = False
                elif (char is '\n'):
                    info_list.append(text[:len(text)-1])
                    text = ""

        return info_list

    def clean_dict(self):
        """ Removes duplicate entries from dict and list """
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
        """ Finds all pages regarding options data """
        count = 0
        text = ""

        url = "https://finance.yahoo.com/quote/{0}/options?p={0}".format(
            tick.symbol)
        page = soup(requests.get(
            url, headers={'User-Agent': 'Custom'}).text, 'html.parser')

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

    def gather_prices(self):
        """ Gathers the stock's historical prices """
        for tick in self.tickers:
            self.parse_prices_page(tick.historical_prices_page, tick)

    def parse_prices_page(self, page, tick):
        """ Parses each page and appends historical_price objects to the correct list """
        # formatted as timestamp, volume, open, low, high, close
        historical_data = [[], [], [], [], [], []]

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
            try:
                index = page.text.index(index_str)
            except:
                self.tickers.remove(tick)
                return
            num = ""
            start = False

            for char in page.text[index:]:
                if (char.isdigit()):
                    start = True
                if (start):
                    if (char != ']'):
                        if (char == ','):
                            try:
                                historical_data[i].append(float(num))
                                num = ""
                            except:
                                self.tickers.remove(tick)
                                return
                        else:
                            num += char
                    else:
                        historical_data[i].append(float(num))
                        break

            historical_data[i].append(float(num))

        for i in range(len(historical_data[0])):
            hp = historical_price(historical_data[0][i], historical_data[1][i], historical_data[2][i],
                                  historical_data[3][i], historical_data[4][i], historical_data[5][i])
            tick.prices.append(hp)

    def gather_option_data(self):
        """ Parses each options page to collect all data for every option """
        test = []
        data_list = []

        for tick in self.tickers:
            calls = True
            for j, date_page in enumerate(tick.date_pages):
                cont = True
                kw_one = True
                failed = False
                page = date_page.text
                keyword_list = [
                    "percentChange", "openInterest",
                    "change", "impliedVolatility", "volume",
                    "ask", "bid", "lastPrice"
                ]

                try:
                    page.index("calls")
                except:
                    self.tickers.remove(tick)
                    break

                try:
                    index = page.index("percentChange")
                    page = page[index+len("percentChange"):]
                except:
                    self.tickers.remove(tick)
                    break

                if (page.index("strike") < page.index("change")):
                    kw_one = True
                    keyword_list.insert(2, "strike")
                else:
                    kw_one = False
                    keyword_list.insert(3, "strike")

                # parses the entire page
                while (cont):
                    # determines if it should continue searching for options
                    try:
                        index = page.index("percentChange")
                        temp = page.index("raw")
                    except:
                        cont = False
                        break

                    page = page[temp+5:]
                    data_list = []

                    # determines whether we are collecting data for calls or puts
                    try:
                        page.index("puts")
                        calls = True
                    except:
                        calls = False

                    # iterate through the list containing words, parsing the page for the data
                    for i in range(len(keyword_list)):
                        text = ""

                        # will gather correct data for each keyword
                        for char in page:
                            if (char == ','):
                                test.append(text)

                                try:
                                    data_list.append(float(text))
                                except:
                                    failed = True
                                    break
                                break
                            else:
                                text += char

                        # if, for some reason, it cannot convert the text to a float, abort entire option,
                        # never added to calls/puts lists
                        if (failed):
                            break

                        try:
                            temp = page.index(
                                keyword_list[i+1]) + len(keyword_list[i+1]) + 9
                            page = page[temp:]
                        except:
                            break

                    if (failed):
                        failed = False
                        continue

                    self.create_option_obj(tick, data_list, kw_one, j, calls)

    def create_option_obj(self, tick, data_list, kw_one, j, calls):
        """ Determines which formatting the strike/change of the page is and creates appropriate object """
        if (kw_one):
            strike = data_list[2]
            change = data_list[3]
        else:
            strike = data_list[3]
            change = data_list[2]

        ob = option(percent_change=data_list[0], open_interest=data_list[1],
                    change=change, strike=strike, iv=data_list[4],
                    volume=data_list[5], ask=data_list[6], bid=data_list[7],
                    last_price=data_list[8], expiration=tick.dates[j])

        if (calls):
            tick.calls.append(ob)
        else:
            tick.puts.append(ob)

    def calculate_option_data(self):
        """ Calculates dte and itm for every option """
        today = str(date.today())
        pattern = '%Y-%m-%d'
        epoch = float(time.mktime(time.strptime(today, pattern)))
        del_list = []

        for tick in self.tickers:
            # removes empty tickers
            if (len(tick.calls) == 0 and len(tick.puts) == 0):
                del_list.append(tick)
                continue
            try:
                curr_price = tick.prices[len(tick.prices)-1].close
            except:
                del_list.append(tick)
                continue

            for call in tick.calls:
                dte = float(call.expiration) - epoch
                dte = math.ceil(dte / 86400)
                call.dte = dte

                if (call.strike <= curr_price):
                    call.itm = True
                else:
                    call.itm = False
            for put in tick.puts:
                dte = float(put.expiration) - epoch
                dte = math.ceil(dte / 86400)
                put.dte = dte

                if (put.strike >= curr_price):
                    put.itm = True
                else:
                    put.itm = False

        for tick in del_list:
            self.tickers.remove(tick)

    def insert_db(self):
        # for historical prices, the format is:
        # ticker, open, low, high, close, volume
        hist_prices_conn = sqlite3.connect("historicalPrices")
        hist_prices_curs = hist_prices_conn.cursor()
        hist_prices_curs.execute(
            "DROP TABLE IF EXISTS historicalPrices")
        hist_prices_curs.execute(
            "CREATE TABLE IF NOT EXISTS historicalPrices(ticker TEXT, date INTEGER, open REAL, low REAL, high REAL, close REAL, volume INTEGER)")

        # for options, the format is:
        # ticker, type, expiration date, dte, strike, volume, oi, bid, ask, last price, percent change, itm,
        # iv, iv20, iv50, iv100, theta, beta, gamma, vegas
        options_conn = sqlite3.connect("optionsData")
        options_curs = options_conn.cursor()
        options_curs.execute(
            "DROP TABLE IF EXISTS optionsData")
        options_curs.execute(
            "CREATE TABLE IF NOT EXISTS optionsData(ticker TEXT, type TEXT, expirationDate TEXT, dte REAL, strike REAL, " +
            "volume INTEGER, openInterest INTEGER, bid REAL, ask REAL, lastPrice REAL, percentChange REAL, itm TEXT, " +
            "impliedVolatility REAL, iv20 REAL, iv50 REAL, iv100 REAL, theta REAL, beta REAL, gamma REAL, vega REAL)")

        for tick in self.tickers:
            for price in tick.prices:
                hist_prices_curs.execute("INSERT INTO historicalPrices VALUES(?, ?, ?, ?, ?, ?, ?)",
                                            (tick.symbol, price.date, price.open, price.low, price.high, price.close, price.volume))
            hist_prices_conn.commit()

            for call in tick.calls:
                options_curs.execute("INSERT INTO optionsData VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, " +
                                     "?, ?, ?, ?, ?, ?, ?)", (tick.symbol, "Call", call.expiration, call.dte, call.strike, call.volume,
                                                                call.open_interest, call.bid, call.ask, call.last_price, call.percent_change, str(
                                                                call.itm), call.iv, tick.iv20, tick.iv50, tick.iv100, call.theta, call.beta,
                                                                call.gamma, call.vega))
            for put in tick.puts:
                options_curs.execute("INSERT INTO optionsData VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, " +
                                     "?, ?, ?, ?, ?, ?, ?)", (tick.symbol, "Put", put.expiration, put.dte, put.strike, put.volume,
                                                                put.open_interest, put.bid, put.ask, put.last_price, put.percent_change, str(
                                                                put.itm), put.iv, tick.iv20, tick.iv50, tick.iv100, put.theta, put.beta,
                                                                put.gamma, put.vega))
            options_conn.commit()

    def prefetch_webpages(self):
        """ Utility function to prefetch webpages concurrently """
        length = len(self.tickers)

        requests_cache.install_cache(
            'cache', backend='sqlite', expire_after=3600)

        with concurrent.futures.ThreadPoolExecutor(max_workers=4) as executor:
            future_to_tickers = {executor.submit(
                self.get_pages, tick): tick for tick in self.tickers}
            count = 0

            for future in concurrent.futures.as_completed(future_to_tickers):
                if count >= len(self.tickers):
                    count = count - 1

                count += 1
                sys.stdout.write("\rProgress: {0} / {1} webpages".format(count, len(self.tickers)))
        print()

    def get_pages(self, tick):
        """ Forces the prefetcher to load all of the pages and stores them in their appropriate objects """
        url = (
            "https://finance.yahoo.com/quote/{0}/options?p={0}".format(tick.symbol))
        sess = requests.session()
        base = soup(
            sess.get(url, headers={'User-Agent': 'Custom'}).text, 'html.parser')

        url = (
            "https://query1.finance.yahoo.com/v8/finance/chart/{0}?formatted=true&crumb=h.8f9xpa6IF&lang=en-US&region=US&interval=1d&events=div%7Csplit&range=1y&corsDomain=finance.yahoo.com".format(tick.symbol))
        sess = requests.session()
        tick.historical_prices_page = soup(
            sess.get(url, headers={'User-Agent': 'Custom'}).text, 'html.parser')

        self.get_options(tick)

        for date in tick.dates:
            cont = False
            count = 0
            url = (
                "https://query1.finance.yahoo.com/v7/finance/options/{0}?formatted=true&crumb=R6vBcFmQGju&lang=en-US&region=US&date={1}&corsDomain=finance.yahoo.com".format(
                tick.symbol, date))

            while (not cont):
                try:
                    page = soup(
                        sess.get(url, headers={'User-Agent': 'Custom'}).text, 'html.parser')
                    cont = True
                except:
                    if (count == 10):
                        time.sleep(.5)

            tick.date_pages.append(page)

    def main(self):
        remove_list = []

        self.download_files()
        self.gather_historical_volatility()

        self.tickers = []

        for symbol in self.ticker_dict:
            self.tickers.append(ticker(
                symbol, self.ticker_dict[symbol][0], self.ticker_dict[symbol][1], self.ticker_dict[symbol][2]))

        print("Prefetching webpages...")
        self.prefetch_webpages()

        for tick in self.tickers:
            if (len(tick.dates) == 0 or tick.historical_prices_page is None):
                remove_list.append(tick)

        for tick in remove_list:
            self.tickers.remove(tick)

        print("Parsing prices and options data...")
        self.gather_prices()
        self.gather_option_data()
        self.calculate_option_data()

        print("Inserting data into databases...")
        self.insert_db()


if __name__ == "__main__":
    utilobj = util()
    utilobj.main()
