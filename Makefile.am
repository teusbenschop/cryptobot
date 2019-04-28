noinst_LIBRARIES = libcryptobot.a

libcryptobot_a_SOURCES = bitfinex.cpp bittrex.cpp bl3p.cpp controllers.cpp cryptopia.cpp exchanges.cpp generators.cpp io.c jsonxx.cpp kraken.cpp locks.c models.cpp poloniex.cpp proxy.cpp shared.cpp simulators.cpp sql.cpp sqlite.cpp yobit.cpp traders.cpp tradesatoshi.cpp html.cpp pugixml.cpp

bin_PROGRAMS = arbitrage balances buy buyers chart comment cryptobot currencies deposits download eurbtc expire keeper lab learn mintrade multipath orders pair patterns rates report satellite sell sellers surfer transfer optimize unittests wallet website withdrawals

arbitrage_SOURCES = arbitrage.cpp
arbitrage_LDADD = libcryptobot.a

balances_SOURCES = balances.cpp
balances_LDADD = libcryptobot.a

buy_SOURCES = buy.cpp
buy_LDADD = libcryptobot.a

buyers_SOURCES = buyers.cpp
buyers_LDADD = libcryptobot.a

chart_SOURCES = chart.cpp
chart_LDADD = libcryptobot.a

comment_SOURCES = comment.cpp
comment_LDADD = libcryptobot.a

cryptobot_SOURCES = cryptobot.cpp
cryptobot_LDADD = libcryptobot.a

currencies_SOURCES = currencies.cpp
currencies_LDADD = libcryptobot.a

deposits_SOURCES = deposits.cpp
deposits_LDADD = libcryptobot.a

download_SOURCES = download.cpp
download_LDADD = libcryptobot.a

eurbtc_SOURCES = eurbtc.cpp
eurbtc_LDADD = libcryptobot.a

expire_SOURCES = expire.cpp
expire_LDADD = libcryptobot.a

keeper_SOURCES = keeper.cpp
keeper_LDADD = libcryptobot.a

lab_SOURCES = lab.cpp
lab_LDADD = libcryptobot.a

learn_SOURCES = learn.cpp
learn_LDADD = libcryptobot.a

mintrade_SOURCES = mintrade.cpp
mintrade_LDADD = libcryptobot.a

multipath_SOURCES = multipath.cpp
multipath_LDADD = libcryptobot.a

orders_SOURCES = orders.cpp
orders_LDADD = libcryptobot.a

optimize_SOURCES = optimize.cpp
optimize_LDADD = libcryptobot.a

pair_SOURCES = pair.cpp
pair_LDADD = libcryptobot.a

patterns_SOURCES = patterns.cpp
patterns_LDADD = libcryptobot.a

satellite_SOURCES = satellite.cpp
satellite_LDADD = libcryptobot.a

rates_SOURCES = rates.cpp
rates_LDADD = libcryptobot.a

report_SOURCES = report.cpp
report_LDADD = libcryptobot.a

sell_SOURCES = sell.cpp
sell_LDADD = libcryptobot.a

sellers_SOURCES = sellers.cpp
sellers_LDADD = libcryptobot.a

surfer_SOURCES = surfer.cpp
surfer_LDADD = libcryptobot.a

transfer_SOURCES = transfer.cpp
transfer_LDADD = libcryptobot.a

unittests_SOURCES = unittests.cpp
unittests_LDADD = libcryptobot.a

wallet_SOURCES = wallet.cpp
wallet_LDADD = libcryptobot.a

website_SOURCES = website.cpp
website_LDADD = libcryptobot.a

withdrawals_SOURCES = withdrawals.cpp
withdrawals_LDADD = libcryptobot.a

AM_CFLAGS = -Wall -Wextra -pedantic -g
AM_CFLAGS += $(OPENSSL_CFLAGS)

AM_CXXFLAGS = -Wall -Wextra -pedantic -std=c++11 -fno-var-tracking -g
AM_CXXFLAGS += $(CURL_CFLAGS)
AM_CXXFLAGS += $(OPENSSL_CFLAGS)
AM_CXXFLAGS += `mysql_config --cflags`
## AM_CXXFLAGS += $(BOOST_CPPFLAGS)
AM_CXXFLAGS += $(SQLITE_CFLAGS)

LIBS = -Wall -lpthread -L. -g -rdynamic -ldl
LIBS += $(CURL_LIBS)
LIBS += $(OPENSSL_LIBS)
LIBS += `mysql_config --libs`
## LIBS += $(BOOST_ASIO_LIB)
## LIBS += $(BOOST_SYSTEM_LIB)
LIBS += $(SQLITE_LIBS)

CLEANFILES = *.gz *~

