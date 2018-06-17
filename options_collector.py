# scans all NASDAQ, NYSE, AMEX stocks with average volume > 5,000,000 and are optionable
# Use Yahoo Finance's option chain feature
# Check if it's optionable here
# Gathers all data on all calls/puts
# Find place to gather IV, Beta, Theta, Gamma, Vega
# Store all data in SQLite database
# Collect historical IV too
# And all previous prices (whatever you did in the short_squeeze_screener)

import os
import sys

print("Hi from Python")
