/*
 Copyright (©) 2013-2017 Teus Benschop.
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#include "models.h"
#include "sql.h"
#include "shared.h"
#include "exchanges.h"
#include "sqlite.h"
#include "proxy.h"


/*
 CREATE DATABASE IF NOT EXISTS store;
 USE store;
 */


/*
 CREATE TABLE IF NOT EXISTS rates (
 id INT NOT NULL AUTO_INCREMENT,
 stamp TIMESTAMP,
 second INT,
 exchange VARCHAR(255),
 market VARCHAR(255),
 currency VARCHAR(255),
 bid DOUBLE,
 ask DOUBLE,
 rate DOUBLE,
 PRIMARY KEY (id)
 );
*/
 

// This select all available rates from the database.
// The database may contain duplicate rates,
// but the call will return only the newest distinct values.
// If only recent rates are requested,
// the rates will not be older than a few minutes,
// to be sure the rates are all current.
// There have been cases of old coins left in the rates table, coins already removed by the exchange.
// So the rates for that coin will not be fetched from the database, in that case.
void rates_get (SQL & sql, vector <int> & seconds,
                vector <string> & exchanges, vector <string> & markets, vector <string> & coins,
                vector <float> & bids, vector <float> & asks, vector <float> & rates,
                bool only_recent)
{
  seconds.clear ();
  exchanges.clear ();
  markets.clear ();
  coins.clear ();
  bids.clear ();
  asks.clear ();
  rates.clear ();
  
  sql.clear ();
  sql.add ("SELECT second, exchange, market, currency, bid, ask, rate FROM rates");
  if (only_recent) {
    sql.add ("WHERE stamp > NOW() - INTERVAL 5 MINUTE");
  }
  sql.add ("ORDER BY id;");
  map <string, vector <string> > result = sql.query ();
  vector <string> second = result ["second"];
  vector <string> exchange = result ["exchange"];
  vector <string> market = result ["market"];
  vector <string> coin = result ["currency"];
  vector <string> bid = result ["bid"];
  vector <string> ask = result ["ask"];
  vector <string> rate = result ["rate"];
  
  // Multiple rates will have been stored in the database,
  // for one coin/market/exchange.
  // This is done for performance reasons.
  // So the above query will have obtained multiple rates for one coin.
  // We only need one rate per coin.
  // Here is the container to ensure there's only distinct rates.
  unordered_map <string, tuple <int, float, float, float> > distinct_values;
  for (unsigned int i = 0; i < rate.size (); i++) {
    string key = exchange[i] + " " + market [i] + " " + coin [i];
    tuple <int, float, float, float> value =  make_tuple (stoi (second[i]),
                                                          str2float (bid[i]),
                                                          str2float (ask[i]),
                                                          str2float (rate[i]));
    distinct_values [key] = value;
  }
  
  // Iterate over the distinct rates.
  for (auto & element : distinct_values) {
    
    string key = element.first;
    tuple <int, float, float, float> value = element.second;
    
    // Obtain the exchange, the market, and the coin.
    vector <string> exchange_market_coin = explode (key, ' ');
    // Due to errors getting the rates, check there's no extra spaces.
    // It should have three values exactly.
    // Anything else is an error and should be skipped.
    if (exchange_market_coin.size() != 3) continue;
    
    // Obtain the values.
    int second = get <0> (value);
    float bid = get <1> (value);
    float ask = get <2> (value);
    float rate = get <3> (value);
    
    // Store them for the caller.
    seconds.push_back (second);
    exchanges.push_back (exchange_market_coin [0]);
    markets.push_back (exchange_market_coin [1]);
    coins.push_back (exchange_market_coin [2]);
    bids.push_back (bid);
    asks.push_back (ask);
    rates.push_back (rate);
  }
}


mutex rate_get_mutex;
unordered_map <string, unordered_map <string, unordered_map <string, float> > > exchanges_markets_coins_rates_cache;


float rate_get_cached (const string & exchange, const string & market, const string & coin)
{
  // The rate.
  float rate = 0;

  // Thread safety.
  // Also preventing multiple database calls,
  // in case multiple simultaneous requests take place,
  // while the data has not yet been read from the database.
  rate_get_mutex.lock ();
  
  // If the memory cache is empty,
  // read the data from the database,
  // and store it in that cache.
  if (exchanges_markets_coins_rates_cache.empty ()) {
    SQL sql (front_bot_ip_address ());
    vector <int> seconds;
    vector <string> exchanges, markets, coins;
    vector <float> bids, asks, rates;
    rates_get (sql, seconds, exchanges, markets, coins, bids, asks, rates, false);
    for (unsigned int i = 0; i < exchanges.size(); i++) {
      string exchange = exchanges[i];
      string market = markets[i];
      string coin = coins[i];
      float rate = rates[i];
      exchanges_markets_coins_rates_cache [exchange][market][coin] = rate;
    }
  }

  // Read the rate from the memory cache.
  rate = exchanges_markets_coins_rates_cache [exchange][market][coin];

  // Unlock again.
  rate_get_mutex.unlock ();

  // Pass the rate to the caller.
  return rate;
}


/*
 CREATE TABLE IF NOT EXISTS arbitrage (
 id INT NOT NULL AUTO_INCREMENT,
 stamp TIMESTAMP,
 market VARCHAR(255),
 coin VARCHAR(255),
 seller VARCHAR(255),
 ask DOUBLE,
 buyer VARCHAR(255),
 bid DOUBLE,
 percentage INT,
 volume DOUBLE,
 PRIMARY KEY (id)
 );
*/
 

void arbitrage_store (SQL & sql, const vector <arbitrage_item> & items)
{
  // Nothing to store? Ready.
  if (items.empty ()) return;
  
  // Use one large multiple insert statement.
  // That works much faster than doing multiple single insert statements.
  sql.clear ();
  sql.add ("INSERT INTO arbitrage (market, coin, seller, ask, buyer, bid, percentage, volume) VALUES");
  // Iterate over the data and store everything.
  for (unsigned int i = 0; i < items.size(); i++) {
    if (i) sql.add (",");
    sql.add ("(");
    sql.add (items[i].market);
    sql.add (",");
    sql.add (items[i].coin);
    sql.add (",");
    sql.add (items[i].seller);
    sql.add (",");
    sql.add (items[i].ask);
    sql.add (",");
    sql.add (items[i].buyer);
    sql.add (",");
    sql.add (items[i].bid);
    sql.add (",");
    sql.add (items[i].percentage);
    sql.add (",");
    sql.add (items[i].volume);
    sql.add (")");
  }
  sql.add (";");
  sql.execute ();
}


void arbitrage_get (SQL & sql, const string & exchange1, const string & exchange2, const string & market,
                    vector <int> & minutes, vector <string> & coins, vector <string> & volumes) // Can go out eventually when no longer needed.
{
  sql.clear ();
  sql.add ("SELECT coin, SUM(volume) FROM arbitrage WHERE market =");
  sql.add (market);
  sql.add ("AND percentage < 15 AND (seller =");
  sql.add (exchange1);
  sql.add ("OR seller =");
  sql.add (exchange2);
  sql.add (") AND (buyer =");
  sql.add (exchange1);
  sql.add ("OR buyer =");
  sql.add (exchange2);
  sql.add (") GROUP BY coin ORDER BY SUM(volume) ASC;");
  map <string, vector <string> > result = sql.query ();
  coins = result ["coin"];
  volumes = result ["SUM(volume)"];
  (void) minutes;
}


void arbitrage_get (SQL & sql,
                    vector <int> & minutes,
                    vector <string> & markets,
                    vector <string> & sellers,
                    vector <string> & coins,
                    vector <float> & asks,
                    vector <string> & buyers,
                    vector <float> & bids,
                    vector <int> & percentages,
                    vector <float> & volumes) // This new function is there but not yet used. It would replace the old.
{
  sql.clear ();
  sql.add ("SELECT *, TIMESTAMPDIFF(MINUTE,stamp,NOW()) FROM arbitrage ORDER BY id;");
  map <string, vector <string> > result = sql.query ();
  
  vector <string> values;

  minutes.clear ();
  values = result ["TIMESTAMPDIFF(MINUTE,stamp,NOW())"];
  for (auto & v : values) {
    int minute = 0;
    if (!v.empty ()) minute = stoi (v);
    minutes.push_back (minute);
  }

  markets = result ["market"];

  sellers = result ["seller"];

  coins = result ["coin"];

  asks.clear ();
  values = result ["ask"];
  for (auto & v : values) asks.push_back (str2float (v));

  buyers = result ["buyer"];

  bids.clear ();
  values = result ["bid"];
  for (auto & v : values) bids.push_back (str2float (v));

  percentages.clear ();
  values = result ["percentage"];
  for (auto & v : values) {
    int percentage = 0;
    if (!v.empty ()) percentage = stoi (v);
    percentages.push_back (percentage);
  }

  volumes.clear ();
  values = result ["volume"];
  for (auto & v : values) volumes.push_back (str2float (v));
}


/*
 CREATE TABLE IF NOT EXISTS balances (
 id INT NOT NULL AUTO_INCREMENT,
 stamp TIMESTAMP,
 exchange VARCHAR(255),
 coin VARCHAR(255),
 total DOUBLE,
 btc DOUBLE,
 euro DOUBLE,
 PRIMARY KEY (id)
 );
 */


float balances_get_average_hourly_btc_equivalent (SQL & sql, string exchange, string coin, int oldhour, int newhour)
{
  sql.clear ();
  sql.add ("SELECT SUM(btc) FROM balances WHERE TRUE");
  if (!exchange.empty ()) {
    sql.add ("AND exchange =");
    sql.add (exchange);
  }
  if (!coin.empty ()) {
    sql.add ("AND coin =");
    sql.add (coin);
  }
  sql.add ("AND stamp BETWEEN NOW() - INTERVAL");
  sql.add (oldhour);
  sql.add ("HOUR AND NOW() - INTERVAL");
  sql.add (newhour);
  sql.add ("HOUR");
  sql.add ("GROUP BY seconds;");
  map <string, vector <string> > result = sql.query ();
  vector <string> sum_btc = result ["SUM(btc)"];
  float average = 0;
  for (auto btc : sum_btc) {
    if (!btc.empty ()) {
      average += str2float (btc);
    }
  }
  if (!sum_btc.empty ()) average /= sum_btc.size ();
  return average;
}


float balances_get_average_hourly_balance (SQL & sql, string exchange, string coin, int oldhour, int newhour)
{
  sql.clear ();
  sql.add ("SELECT SUM(total) FROM balances WHERE TRUE");
  if (!exchange.empty ()) {
    sql.add ("AND exchange =");
    sql.add (exchange);
  }
  if (!coin.empty ()) {
    sql.add ("AND coin =");
    sql.add (coin);
  }
  sql.add ("AND stamp BETWEEN NOW() - INTERVAL");
  sql.add (oldhour);
  sql.add ("HOUR AND NOW() - INTERVAL");
  sql.add (newhour);
  sql.add ("HOUR");
  sql.add ("GROUP BY seconds;");
  map <string, vector <string> > result = sql.query ();
  vector <string> sum_total = result ["SUM(total)"];
  float average = 0;
  for (auto value : sum_total) {
    if (!value.empty ()) {
      average += str2float (value);
    }
  }
  if (!sum_total.empty ()) average /= sum_total.size ();
  return average;
}


vector <string> balances_get_distinct_daily_coins (SQL & sql)
{
  sql.clear ();
  sql.add ("SELECT DISTINCT coin FROM balances WHERE stamp >= NOW() - INTERVAL 1 DAY ORDER BY coin;");
  map <string, vector <string> > result = sql.query ();
  return result ["coin"];
}


// Reads the most recent total balances from the database.
void balances_get_current (SQL & sql,
                           vector <string> & exchanges,
                           vector <string> & coins,
                           vector <float> & totals)
{
  // Get the most recent second to get the balances for.
  sql.clear ();
  sql.add ("SELECT DISTINCT seconds FROM balances WHERE stamp >= NOW() - INTERVAL 1 HOUR ORDER BY seconds;");
  vector <string> seconds = sql.query () ["seconds"];
  int second = 0;
  for (auto s : seconds) second = stoi (s);
  
  // Get the balances for this second.
  sql.clear ();
  sql.add ("SELECT exchange, coin, total FROM balances WHERE seconds =");
  sql.add (second);
  sql.add (";");
  map <string, vector <string> > result = sql.query ();
  exchanges = result ["exchange"];
  coins = result ["coin"];
  totals.clear ();
  vector <string> values = result ["total"];
  for (auto & v : values) {
    totals.push_back (str2float (v));
  }
}


void balances_get_month (SQL & sql,
                         vector <int> & seconds,
                         vector <string> & exchanges,
                         vector <string> & coins,
                         vector <float> & totals,
                         vector <float> & bitcoins,
                         vector <float> & euros)
{
  sql.clear ();
  sql.add ("SELECT * FROM balances WHERE stamp > NOW() - INTERVAL 31 DAY ORDER BY id;");
  map <string, vector <string> > result = sql.query ();
  
  vector <string> values;

  seconds.clear ();
  values = result ["seconds"];
  for (auto & v : values) {
    int second = 0;
    if (!v.empty ()) second = stoi (v);
    seconds.push_back (second);
  }

  exchanges = result ["exchange"];

  coins = result ["coin"];

  totals.clear ();
  values = result ["total"];
  for (auto & v : values) totals.push_back (str2float (v));

  bitcoins.clear ();
  values = result ["btc"];
  for (auto & v : values) bitcoins.push_back (str2float (v));

  euros.clear ();
  values = result ["euro"];
  for (auto & v : values) euros.push_back (str2float (v));
}


/*
 CREATE TABLE IF NOT EXISTS wallets (
 id INT NOT NULL AUTO_INCREMENT,
 exchange VARCHAR(255),
 coin VARCHAR(255),
 address VARCHAR(255),
 paymentid VARCHAR(255),
 withdrawal VARCHAR(255),
 trading INT,
 balancing BOOLEAN,
 minwithdraw DOUBLE,
 greenlane VARCHAR(255),
 PRIMARY KEY (id)
 );
 */


void wallets_set_deposit_details (SQL & sql, string exchange, string coin_id, string address, string payment_id)
{
  // Be sure than when saving deposit details,
  // not to overwrite existing details like minimum withdrawal amounts, and the like.
  string existing_address = wallets_get_address (sql, exchange, coin_id);
  string existing_payment_id = wallets_get_payment_id (sql, exchange, coin_id);
  if (existing_address.empty ()) {
    if (existing_payment_id.empty ()) {
      // Existing details are not yet there: Insert them.
      wallets_delete_deposit_details (sql, exchange, coin_id);
      sql.clear ();
      sql.add ("INSERT INTO wallets (exchange, coin, address, paymentid) VALUES (");
      sql.add (exchange);
      sql.add (",");
      sql.add (coin_id);
      sql.add (",");
      sql.add (address);
      sql.add (",");
      sql.add (payment_id);
      sql.add (");");
      sql.execute ();
      // Done.
      return;
    }
  }
  // Update the select details.
  sql.clear ();
  sql.add ("UPDATE wallets SET address =");
  sql.add (address);
  sql.add (", paymentid =");
  sql.add (payment_id);
  sql.add ("WHERE exchange =");
  sql.add (exchange);
  sql.add ("AND coin =");
  sql.add (coin_id);
  sql.add (";");
  sql.execute ();
}


void wallets_delete_deposit_details (SQL & sql, string exchange, string coin_id)
{
  sql.clear();
  sql.add ("DELETE FROM wallets WHERE exchange =");
  sql.add (exchange);
  sql.add ("AND coin =");
  sql.add (coin_id);
  sql.add (";");
  sql.execute ();
}


string wallets_get_address (SQL & sql, string exchange, string coin)
{
  sql.clear();
  sql.add ("SELECT address FROM wallets WHERE exchange =");
  sql.add (exchange);
  sql.add ("AND coin =");
  sql.add (coin);
  vector <string> result = sql.query () ["address"];
  if (!result.empty ()) return result.front ();
  return "";
}


string wallets_get_payment_id (SQL & sql, string exchange, string coin)
{
  sql.clear();
  sql.add ("SELECT paymentid FROM wallets WHERE exchange =");
  sql.add (exchange);
  sql.add ("AND coin =");
  sql.add (coin);
  vector <string> result = sql.query () ["paymentid"];
  if (!result.empty ()) return result.front ();
  return "";
}


void wallets_set_trading (SQL & sql, string exchange, string coin_id, int trading)
{
  sql.clear();
  sql.add ("UPDATE wallets SET trading =");
  sql.add (trading);
  sql.add ("WHERE exchange =");
  sql.add (exchange);
  sql.add ("AND coin =");
  sql.add (coin_id);
  sql.add (";");
  sql.execute ();
}


// Returns whether the wallet of $coin_id at $exchange is trading, basically.
// If it is trading, it may be temporarily not trading, but basically it has been set to be trading.
bool wallets_get_trading (SQL & sql, string exchange, string coin_id)
{
  sql.clear();
  sql.add ("SELECT trading FROM wallets WHERE exchange =");
  sql.add (exchange);
  sql.add ("AND coin =");
  sql.add (coin_id);
  sql.add (";");
  vector <string> result = sql.query () ["trading"];
  if (!result.empty ()) return str2bool (result [0]);
  return false;
}


void wallets_set_balancing (SQL & sql, string exchange, string coin_id, bool balancing)
{
  sql.clear();
  sql.add ("UPDATE wallets SET balancing =");
  sql.add (balancing);
  sql.add ("WHERE exchange =");
  sql.add (exchange);
  sql.add ("AND coin =");
  sql.add (coin_id);
  sql.add (";");
  sql.execute ();
}


bool wallets_get_balancing (SQL & sql, string exchange, string coin_id)
{
  sql.clear();
  sql.add ("SELECT balancing FROM wallets WHERE exchange =");
  sql.add (exchange);
  sql.add ("AND coin =");
  sql.add (coin_id);
  sql.add (";");
  vector <string> result = sql.query () ["balancing"];
  if (!result.empty ()) return str2bool (result [0]);
  return false;
}


vector <string> wallets_get_trading_coins (SQL & sql)
{
  sql.clear ();
  sql.add ("SELECT DISTINCT coin FROM wallets WHERE trading;");
  return sql.query () ["coin"];
}


vector <string> wallets_get_balancing_coins (SQL & sql)
{
  sql.clear ();
  sql.add ("SELECT DISTINCT coin FROM wallets WHERE balancing;");
  return sql.query () ["coin"];
}


// This returns the exchanges that are now set to be trading the $coin_id.
// If there's an exchange that's temporarily disabled to be trading, it won't return it right now.
// Once the time to be disabled has expired, this exchange will be included again.
vector <string> wallets_get_trading_exchanges (SQL & sql, string coin_id)
{
  sql.clear ();
  sql.add ("SELECT exchange FROM wallets WHERE coin =");
  sql.add (coin_id);
  // The "trading" field indicates the seconds after which the wallet can trade again.
  // It is used to temporarily disable this wallet for trading.
  // If the field is zero, the wallet is not trading.
  // If it's non-zero, the curren second after the Unix epoch should be larger than the value of the field.
  sql.add ("AND trading AND UNIX_TIMESTAMP() > trading;");
  return sql.query () ["exchange"];
}


vector <string> wallets_get_balancing_exchanges (SQL & sql, string coin_id)
{
  sql.clear ();
  sql.add ("SELECT exchange FROM wallets WHERE coin =");
  sql.add (coin_id);
  sql.add ("AND balancing;");
  return sql.query () ["exchange"];
}


// Not all coins can be withdrawed in fractional amounts.
// Some only do integer amounts.
// Some exchanges add a fractional transfer fee on top.
// This function cares for that.
// If needed, it updates the $amount to withdraw.
void wallets_update_withdrawal_amount (SQL & sql, string exchange, string coin_id, float & amount)
{
  sql.clear ();
  sql.add ("SELECT withdrawal FROM wallets where exchange =");
  sql.add (exchange);
  sql.add ("AND coin =");
  sql.add (coin_id);
  sql.add (";");
  vector <string> result = sql.query () ["withdrawal"];
  if (!result.empty ()) {
    string withdrawal = result [0];
    if (!withdrawal.empty ()) {
      // Deal with integer withdrawal amounts.
      string integer = "integer";
      if (withdrawal.find (integer) != string::npos) {
        amount = floor (amount);
        withdrawal.erase (0, integer.length());
        // Transfer fee can be, e.g.: "+0.025".
        if (!withdrawal.empty()) {
          float transfer_fee = str2float (withdrawal);
          amount += transfer_fee;
        }
      }
    }
  }
}


void wallets_get_disabled (SQL & sql, vector <string> & details)
{
  sql.clear ();
  sql.add ("SELECT exchange, coin, trading, balancing FROM wallets where trading = 0 OR balancing = 0 ORDER BY exchange;");
  map <string, vector <string> > result = sql.query ();
  vector <string> exchange = result ["exchange"];
  vector <string> coin = result ["coin"];
  vector <string> trading = result ["trading"];
  vector <string> balancing = result ["balancing"];
  for (unsigned int i = 0; i < exchange.size(); i++) {
    string detail = exchange[i] + " " + coin[i] + " wallet is ";
    bool is_trading;
    stringstream (trading[i]) >> is_trading;
    bool is_balancing;
    stringstream (balancing[i]) >> is_balancing;
    if (!is_trading && !is_balancing) {
      detail.append ("disabled");
    } else if (!is_trading) {
      detail.append ("not trading");
    } else if (!is_balancing) {
      detail.append ("not balancing");
    }
    details.push_back (detail);
  }
}


string wallets_get_greenlane (SQL & sql, string exchange, string coin)
{
  sql.clear();
  sql.add ("SELECT greenlane FROM wallets WHERE exchange =");
  sql.add (exchange);
  sql.add ("AND coin =");
  sql.add (coin);
  vector <string> result = sql.query () ["greenlane"];
  if (!result.empty ()) return result.front ();
  return "";
}


vector <string> wallets_get_coin_ids (SQL & sql, string exchange)
{
  sql.clear();
  sql.add ("SELECT coin FROM wallets WHERE exchange =");
  sql.add (exchange);
  sql.add (";");
  vector <string> coin_ids = sql.query () ["coin"];
  return coin_ids;
}


// Supply selected details of all recorded wallets.
void wallet_get_details (SQL & sql, vector <string> & exchanges, vector <string> & coin_ids, vector <string> & addresses)
{
  sql.clear();
  sql.add ("SELECT exchange, coin, address FROM wallets;");
  exchanges = sql.query () ["exchange"];
  coin_ids = sql.query () ["coin"];
  addresses = sql.query () ["address"];
}


// This returns the minimum amount of $coin to withdraw from the $exchange.
string wallets_get_minimum_withdrawal_amount (SQL & sql, string exchange, string coin_id)
{
  sql.clear();
  sql.add ("SELECT minwithdraw FROM wallets WHERE exchange =");
  sql.add (exchange);
  sql.add ("AND coin =");
  sql.add (coin_id);
  vector <string> result = sql.query () ["minwithdraw"];
  if (!result.empty ()) {
    string minwithdraw = result[0];
    if (!minwithdraw.empty ()) {
      return minwithdraw;
    }
  }
  return "";
}


// Reads all wallet-related data.
void wallets_get (SQL & sql,
                  vector <int> & ids,
                  vector <string> & exchanges,
                  vector <string> & coins,
                  vector <string> & addresses,
                  vector <string> & paymentids,
                  vector <string> & withdrawals,
                  vector <bool> & tradings,
                  vector <bool> & balancings,
                  vector <string> & minwithdraws,
                  vector <string> & greenlanes,
                  vector <string> & comments)
{
  sql.clear ();
  sql.add ("SELECT * FROM wallets ORDER BY id;");
  map <string, vector <string> > result = sql.query ();

  vector <string> values;

  ids.clear ();
  values = result ["id"];
  for (auto & v : values) ids.push_back (stoi (v));

  exchanges = result ["exchange"];

  coins = result ["coin"];

  addresses = result ["address"];

  paymentids = result ["paymentid"];

  withdrawals = result ["withdrawal"];

  tradings.clear ();
  values = result ["trading"];
  for (auto & v : values) tradings.push_back (str2bool (v));

  balancings.clear ();
  values = result ["balancing"];
  for (auto & v : values) balancings.push_back (str2bool (v));

  minwithdraws = result ["minwithdraw"];

  greenlanes = result ["greenlane"];

  comments = result ["comments"];
}


 /*
 CREATE TABLE IF NOT EXISTS transfers (
 id INT NOT NULL AUTO_INCREMENT,
 stamp TIMESTAMP,
 seconds INT,
 orderid VARCHAR(255),
 withdraw VARCHAR(255),
 coin VARCHAR(255),
 quantity DOUBLE,
 executed INT,
 txid VARCHAR(255),
 deposit VARCHAR(255),
 rxcoin VARCHAR(255),
 address VARCHAR(255),
 paymentid VARCHAR(255),
 arrived INT,
 rxid VARCHAR(255),
 rxquantity DOUBLE,
 visible INT,
 PRIMARY KEY (id)
 );
*/
 
// quantity: How much was sent.
// executed: After how many minutes the exchange executed the order.
// arrived: After how many minutes the coins arrived on the other exchange.
// rxquantity: How much the other exchange received.


void transfers_enter_order (SQL & sql, string order_id,
                            string exchange_withdraw, string coin_abbreviation_withdraw,
                            float quantity,
                            string exchange_deposit, string coin_abbreviation_deposit,
                            string address, string paymentid)
{
  sql.clear ();
  sql.add ("INSERT INTO transfers (seconds, orderid, withdraw, coin, quantity, deposit, rxcoin, address, paymentid) VALUES (");
  sql.add (seconds_since_epoch ());
  sql.add (",");
  sql.add (order_id);
  sql.add (",");
  sql.add (exchange_withdraw);
  sql.add (",");
  sql.add (coin_abbreviation_withdraw);
  sql.add (",");
  sql.add (quantity);
  sql.add (",");
  sql.add (exchange_deposit);
  sql.add (",");
  sql.add (coin_abbreviation_deposit);
  sql.add (",");
  sql.add (address);
  sql.add (",");
  sql.add (paymentid);
  sql.add (");");
  sql.execute ();
}


void transfers_get_month (SQL & sql,
                          vector <int> & database_ids,
                          vector <int> & seconds,
                          vector <string> & order_ids,
                          vector <string> & coin_ids,
                          vector <string> & txexchanges,
                          vector <float> & txquantities,
                          vector <string> & txids,
                          vector <string> & executeds,
                          vector <string> & rxexchanges,
                          vector <string> & addresses,
                          vector <string> & visibles,
                          vector <float> & rxquantities,
                          vector <string> & arriveds)
{
  sql.clear ();
  sql.add ("SELECT * FROM transfers WHERE stamp > NOW() - INTERVAL 30 DAY ORDER BY id;");
  map <string, vector <string> > result = sql.query ();
  
  vector <string> values;

  database_ids.clear ();
  values = result ["id"];
  for (auto s : values) database_ids.push_back (str2int (s));

  seconds.clear ();
  values = result ["seconds"];
  for (auto s : values) seconds.push_back (str2int (s));

  order_ids = result ["orderid"];
  
  coin_ids = result ["coin"];
  
  txexchanges = result ["withdraw"];

  txquantities.clear ();
  values = result ["quantity"];
  for (auto s : values) txquantities.push_back (str2float (s));

  txids = result ["txid"];

  executeds = result ["executed"];

  rxexchanges = result ["deposit"];

  addresses = result ["address"];

  visibles = result ["visible"];

  rxquantities.clear ();
  values = result ["rxquantity"];
  for (auto s : values) rxquantities.push_back (str2float (s));

  arriveds = result ["arrived"];
}


void transfers_get_non_executed_orders (SQL & sql,
                                        vector <string> & order_ids,
                                        vector <string> & withdraw_exchanges,
                                        vector <string> & coin_abbreviations,
                                        vector <string> & deposit_exchanges,
                                        vector <float> & quantities)
{
  sql.clear ();
  sql.add ("SELECT orderid, withdraw, coin, deposit, quantity FROM transfers WHERE executed IS NULL;");
  map <string, vector <string> > result = sql.query ();
  order_ids = result ["orderid"];
  withdraw_exchanges = result ["withdraw"];
  coin_abbreviations = result ["coin"];
  deposit_exchanges = result ["deposit"];
  quantities.clear ();
  vector <string> amounts = result ["quantity"];
  for (auto amount : amounts) {
    float value = 0;
    if (!amount.empty ()) value = str2float (amount);
    quantities.push_back (value);
  }
}


void transfers_set_order_executed (SQL & sql, string exchange, string order_id, string txid)
{
  sql.clear ();
  sql.add ("UPDATE transfers SET executed = (");
  sql.add (seconds_since_epoch ());
  sql.add ("- seconds) / 60, txid =");
  sql.add (txid);
  sql.add ("WHERE withdraw =");
  sql.add (exchange);
  sql.add ("AND orderid =");
  sql.add (order_id);
  sql.add (";");
  sql.execute ();
}


// This gets the orders that have not yet arrived at the exchanges where the funds were deposited.
// It excluded, naturally, orders that have not yet been executed by the exchange where they were withdrawn.
void transfers_get_non_arrived_orders (SQL & sql,
                                       vector <string> & order_ids,
                                       vector <string> & withdraw_exchanges,
                                       vector <string> & deposit_exchanges,
                                       vector <string> & coin_abbreviations_withdraw,
                                       vector <string> & coin_abbreviations_deposit,
                                       vector <float> & quantities,
                                       vector <string> & transaction_ids)
{
  sql.clear ();
  sql.add ("SELECT orderid, coin, quantity, withdraw, deposit, rxcoin, rxid FROM transfers WHERE executed IS NOT NULL AND arrived IS NULL;");
  map <string, vector <string> > result = sql.query ();
  order_ids = result ["orderid"];
  withdraw_exchanges = result ["withdraw"];
  deposit_exchanges = result ["deposit"];
  coin_abbreviations_withdraw = result ["coin"];
  coin_abbreviations_deposit = result ["rxcoin"];
  quantities.clear ();
  vector <string> amounts = result ["quantity"];
  for (auto amount : amounts) {
    quantities.push_back (str2float (amount));
  }
  transaction_ids = result ["rxid"];
}


// Set a transfer order as having arrived.
// $exchange: Exchange where it was deposited.
// $order_id: The order ID of the exchange where the withdrawal was made.
// $transaction_id: The txid as seen by the exchange where it was deposited.
// $quantity: The quantity that arrived at the exchange where it was deposited.
void transfers_set_order_arrived (SQL & sql, string exchange, string order_id, string transaction_id, float quantity)
{
  sql.clear ();
  sql.add ("UPDATE transfers SET arrived = (");
  sql.add (seconds_since_epoch ());
  sql.add ("- seconds) / 60, rxid =");
  sql.add (transaction_id);
  sql.add (", rxquantity =");
  sql.add (quantity);
  sql.add ("WHERE deposit =");
  sql.add (exchange);
  sql.add ("AND orderid =");
  sql.add (order_id);
  sql.add (";");
  sql.execute ();
}


int transfers_get_active_orders (SQL & sql, string coin_abbreviation, vector <string> exchanges)
{
  sql.clear ();
  sql.add ("SELECT count(*) FROM transfers WHERE (executed IS NULL OR arrived IS NULL) AND (");
  for (unsigned int i = 0; i < exchanges.size(); i++) {
    if (i) sql.add ("OR");
    sql.add ("withdraw =");
    sql.add (exchanges[i]);
  }
  sql.add (") AND (");
  for (unsigned int i = 0; i < exchanges.size(); i++) {
    if (i) sql.add ("OR");
    sql.add ("deposit =");
    sql.add (exchanges[i]);
  }
  sql.add (") AND coin =");
  sql.add (coin_abbreviation);
  sql.add (";");
  vector <string> results = sql.query () ["count(*)"];
  if (results.empty ()) return 0;
  return stoi (results[0]);
}


int transfers_get_seconds_since_last_order (SQL & sql, string coin_abbreviation, vector <string> exchanges)
{
  sql.clear ();
  sql.add ("SELECT seconds FROM transfers WHERE (");
  for (unsigned int i = 0; i < exchanges.size(); i++) {
    if (i) sql.add ("OR");
    sql.add ("withdraw =");
    sql.add (exchanges[i]);
  }
  sql.add (") AND (");
  for (unsigned int i = 0; i < exchanges.size(); i++) {
    if (i) sql.add ("OR");
    sql.add ("deposit =");
    sql.add (exchanges[i]);
  }
  sql.add (") AND coin =");
  sql.add (coin_abbreviation);
  sql.add ("ORDER BY seconds DESC LIMIT 1;");
  vector <string> results = sql.query () ["seconds"];
  // No previous transfer record: Return one hour ago.
  if (results.empty ()) return 3600;
  // NULL values recorded: Return one hour ago.
  string seconds = results[0];
  if (seconds.empty ()) return 3600;
  // Return how many seconds ago.
  return seconds_since_epoch () - stoi (results[0]);
}


// Get all the transaction IDs of the deposits recorded in the database.
vector <string> transfers_get_rx_ids (SQL & sql)
{
  sql.clear ();
  sql.add ("SELECT rxid FROM transfers;");
  vector <string> rxid = sql.query () ["rxid"];
  return rxid;
}


void transfers_get_order_details (SQL & sql,
                                  const string & orderid,
                                  string & stamp,
                                  int & seconds,
                                  string & withdraw,
                                  string & coin,
                                  float & quantity,
                                  string & txid,
                                  string & deposit,
                                  string & rxcoin,
                                  string & address,
                                  string & rxid)
{
  sql.clear ();
  sql.add ("SELECT stamp, seconds, withdraw, coin, quantity, txid, deposit, rxcoin, address, rxid FROM transfers WHERE orderid =");
  sql.add (orderid);
  sql.add (";");
  map <string, vector <string> > result = sql.query ();
  vector <string> v_stamp = result ["stamp"];
  vector <string> v_seconds = result ["seconds"];
  vector <string> v_withdraw = result ["withdraw"];
  vector <string> v_coin = result ["coin"];
  vector <string> v_quantity = result ["quantity"];
  vector <string> v_txid = result ["txid"];
  vector <string> v_deposit = result ["deposit"];
  vector <string> v_rxcoin = result ["rxcoin"];
  vector <string> v_address = result ["address"];
  vector <string> v_rxid = result ["rxid"];
  if (!v_stamp.empty ()) {
    stamp = v_stamp[0];
    seconds = stoi (v_seconds[0]);
    withdraw = v_withdraw[0];
    coin = v_coin[0];
    quantity = str2float (v_quantity[0]);
    txid = v_txid[0];
    deposit = v_deposit[0];
    rxcoin = v_rxcoin[0];
    address = v_address[0];
    rxid = v_rxid[0];
  }
}


// Gets the amounts of the coins of withdrawal orders that have not yet been executed by the exchanges.
// This is the format how it passes the results:
// container [exchange+coin_abbre] = amount.
void transfers_get_non_executed_amounts (SQL & sql, map <string, float> & exchanges_coins_amounts)
{
  sql.clear ();
  sql.add ("SELECT withdraw, coin, quantity FROM transfers WHERE executed IS NULL;");
  map <string, vector <string> > result = sql.query ();
  vector <string> withdraw = result ["withdraw"];
  vector <string> coin = result ["coin"];
  vector <string> quantity = result ["quantity"];
  for (unsigned int i = 0; i < withdraw.size(); i++) {
    string exchange = withdraw [i];
    string coin_abbrev = coin [i];
    float amount = 0;
    string value = quantity [i];
    if (!value.empty ()) amount = str2float (value);
    // Store it in the map.
    // There may be multiple non-executed identical exchange+coin combinations,
    // so add the amounts all together, so as to give the sum of them all.
    exchanges_coins_amounts [exchange + coin_abbrev] += amount;
  }
}


// Function for unit testing.
void transfers_update_seconds_tests (SQL & sql, int seconds)
{
  sql.clear ();
  sql.add ("UPDATE transfers SET seconds = seconds +");
  sql.add (seconds);
  sql.add ("WHERE orderid = 'unittest-order';");
  sql.execute ();
}


// Function for unit testing.
void transfers_clean_tests (SQL & sql)
{
  sql.clear ();
  sql.add ("DELETE FROM transfers WHERE orderid = 'unittest-order';");
  sql.execute ();
}


// Set a transfer order as being visibile, i.e. the unconfirmed balance is now visible.
void transfers_set_visible (SQL & sql, int rowid, float quantity)
{
  sql.clear ();
  sql.add ("UPDATE transfers SET visible = (");
  sql.add (seconds_since_epoch ());
  sql.add ("- seconds) / 60, rxquantity =");
  sql.add (quantity);
  sql.add ("WHERE id =");
  sql.add (rowid);
  sql.add (";");
  sql.execute ();
}


// Set a transfer order as having arrived.
void transfers_set_arrived (SQL & sql, int rowid)
{
  sql.clear ();
  sql.add ("UPDATE transfers SET arrived = (");
  sql.add (seconds_since_epoch ());
  sql.add ("- seconds) / 60");
  sql.add ("WHERE id =");
  sql.add (rowid);
  sql.add (";");
  sql.execute ();
}


// Set a transfer's order identifier.
void transfers_set_order_id (SQL & sql, int rowid, string order_id)
{
  sql.clear ();
  sql.add ("UPDATE transfers SET orderid =");
  sql.add (order_id);
  sql.add ("WHERE id =");
  sql.add (rowid);
  sql.add (";");
  sql.execute ();
}


/*
 CREATE TABLE IF NOT EXISTS withdrawals (
 exchange VARCHAR(255),
 orderid VARCHAR(255)
 );
 */
 

void withdrawals_enter (SQL & sql, string exchange, string orderid)
{
  sql.clear ();
  sql.add ("INSERT INTO withdrawals (exchange, orderid) VALUES (");
  sql.add (exchange);
  sql.add (",");
  sql.add (orderid);
  sql.add (");");
  sql.execute ();
}


bool withdrawals_present (SQL & sql, string exchange, string orderid)
{
  sql.clear ();
  sql.add ("SELECT count(*) FROM withdrawals WHERE exchange =");
  sql.add (exchange);
  sql.add ("AND orderid =");
  sql.add (orderid);
  sql.add (";");
  vector <string> results = sql.query () ["count(*)"];
  if (results.empty ()) return false;
  return stoi (results[0]);
}


/*
 CREATE TABLE IF NOT EXISTS trades (
 id INT NOT NULL AUTO_INCREMENT,
 stamp TIMESTAMP,
 market VARCHAR(255),
 coin VARCHAR(255),
 buy VARCHAR(255),
 sell VARCHAR(255),
 quantity DOUBLE,
 marketgain DOUBLE,
 PRIMARY KEY (id)
 );
*/
 

// Store a trade plus its parameters in the database.
// This enabled coin performance monitoring, and exchange performance monitoring, and so on.
void trades_store (SQL & sql, string market, string coin,
                   string buyexchange, string sellexchange,
                   float quantity, float marketgain)
{
  sql.clear ();
  sql.add ("INSERT INTO trades (market, coin, buy, sell, quantity, marketgain) VALUES (");
  sql.add (market);
  sql.add (",");
  sql.add (coin);
  sql.add (",");
  sql.add (buyexchange);
  sql.add (",");
  sql.add (sellexchange);
  sql.add (",");
  sql.add (quantity);
  sql.add (",");
  sql.add (marketgain);
  sql.add (");");
  sql.execute ();
}


// Reads all recent data from the database.
void trades_get_month (SQL & sql,
                       vector <int> & seconds,
                       vector <string> & markets, vector <string> & coins,
                       vector <string> & buyexchanges, vector <string> & sellexchanges,
                       vector <float> & quantities, vector <float> & marketgains)
{
  sql.clear ();
  sql.add ("SELECT *, UNIX_TIMESTAMP(stamp) FROM trades WHERE stamp > NOW() - INTERVAL 30 DAY ORDER BY id;");
  map <string, vector <string> > result = sql.query ();
  
  vector <string> values;
  
  seconds.clear ();
  values = result ["UNIX_TIMESTAMP(stamp)"];
  for (auto s : values) seconds.push_back (str2int (s));

  markets = result ["market"];

  coins = result ["coin"];
  
  buyexchanges = result ["buy"];

  sellexchanges = result ["sell"];

  quantities.clear ();
  values = result ["quantity"];
  for (auto s : values) quantities.push_back (str2float (s));
  
  marketgains.clear ();
  values = result ["marketgain"];
  for (auto s : values) marketgains.push_back (str2float (s));
}


vector <string> trades_get_coins (SQL & sql)
{
  sql.clear ();
  sql.add ("SELECT DISTINCT market FROM trades;");
  return sql.query () ["market"];
}


void trades_get_daily_coin_statistics (SQL & sql, string market, map <string, float> & coins_gains)
{
  sql.clear ();
  sql.add ("SELECT coin, SUM(marketgain) FROM trades WHERE market =");
  sql.add (market);
  sql.add ("AND stamp > NOW() - INTERVAL 1 DAY GROUP BY coin ORDER BY SUM(marketgain) DESC;");
  map <string, vector <string> > result = sql.query ();
  vector <string> coin = result ["coin"];
  vector <string> marketgain = result ["SUM(marketgain)"];
  for (unsigned int i = 0; i < coin.size(); i++) {
    string key= coin[i];
    float value = str2float (marketgain[i]);
    coins_gains [key] = value;
  }
}


void trades_get_daily_exchange_statistics (SQL & sql, string market, map <string, float> & exchange_gain)
{
  sql.clear ();
  sql.add ("SELECT buy, sell, marketgain FROM trades WHERE market =");
  sql.add (market);
  sql.add ("AND stamp > NOW() - INTERVAL 1 DAY;");
  map <string, vector <string> > result = sql.query ();
  vector <string> buy = result ["buy"];
  vector <string> sell = result ["sell"];
  vector <string> marketgain = result ["marketgain"];
  for (unsigned int i = 0; i < buy.size(); i++) {
    float gain = str2float (marketgain[i]);
    gain /= 2;
    exchange_gain [buy[i]] += gain;
    exchange_gain [sell[i]] += gain;
  }
}


// Get the trade statistics.
// $days: How many days to take the stats over.
// $counts: how often a coin was traded.
// $volumes: the total trade volume.
void trades_get_statistics (SQL & sql, const string & exchange, const string & market, int days, map <string, int> & counts, map <string, float> & volumes)
{
  sql.clear ();
  // Get the values from the database.
  sql.add ("SELECT coin, quantity FROM trades WHERE stamp > NOW() - INTERVAL");
  sql.add (days);
  sql.add ("DAY");
  sql.add ("AND market =");
  sql.add (market);
  sql.add ("AND (buy =");
  sql.add (exchange);
  sql.add ("OR sell =");
  sql.add (exchange);
  sql.add (");");
  map <string, vector <string> > result = sql.query ();
  vector <string> coins = result ["coin"];
  vector <string> quantities = result ["quantity"];
  for (unsigned int i = 0; i < coins.size(); i++) {
    string coin = coins[i];
    counts [coin]++;
    float quantity = 0;
    string value = quantities[i];
    if (!value.empty ()) quantity = str2float (value);
    volumes[coin] += quantity;
  }
}


// Takes the exchange and the market, and returns the trade count and the market gain.
void trades_get_statistics_last_24_hours (SQL & sql, const string & exchange, const string & market,
                                          int & count, float & gain)
{
  sql.clear ();
  sql.add ("SELECT COUNT(*), SUM(marketgain) FROM trades WHERE stamp > NOW() - INTERVAL 1 DAY AND market =");
  sql.add (market);
  sql.add ("AND (buy =");
  sql.add (exchange);
  sql.add ("OR sell =");
  sql.add (exchange);
  sql.add (");");
  map <string, vector <string> > result = sql.query ();
  vector <string> counts = result ["COUNT(*)"];
  vector <string> gains = result ["SUM(marketgain)"];
  count = 0;
  if (!counts.empty ()) if (!counts[0].empty ()) count = stoi (counts[0]);
  gain = 0;
  if (!gains.empty ()) if (!gains[0].empty ()) gain = str2float (gains[0]);
}


/*
 CREATE TABLE IF NOT EXISTS failures (
 id INT NOT NULL AUTO_INCREMENT,
 stamp TIMESTAMP,
 type VARCHAR(255),
 exchange VARCHAR(255),
 coin VARCHAR(255),
 quantity DOUBLE,
 message VARCHAR(1024),
 PRIMARY KEY (id)
 );
*/


string failures_type_api_error ()
{
  return "apierror";
}


string failures_type_bot_error ()
{
  return "boterror";
}


string failures_hurried_call_succeeded ()
{
  return "hurriedcallsuccess";
}


string failures_hurried_call_failed ()
{
  return "hurriedcallfail";
}


void failures_store (SQL & sql, string type, string exchange, string coin, float quantity, string message)
{
  sql.clear ();
  sql.add ("INSERT INTO failures (type, exchange, coin, quantity, message) VALUES (");
  sql.add (type);
  sql.add (",");
  sql.add (exchange);
  sql.add (",");
  sql.add (coin);
  sql.add (",");
  sql.add (quantity);
  sql.add (",");
  sql.add (message);
  sql.add (");");
  sql.execute ();
}


void failures_retrieve (SQL & sql, string type,
                        vector <string> & timestamps,
                        vector <string> & exchanges,
                        vector <string> & coins,
                        vector <float> & quantities,
                        vector <string> & messages)
{
  sql.clear ();
  sql.add ("SELECT stamp, exchange, coin, quantity, message FROM failures WHERE stamp > NOW() - INTERVAL 1 DAY AND type =");
  sql.add (type);
  sql.add (";");
  map <string, vector <string> > result = sql.query ();
  timestamps = result ["stamp"];
  exchanges = result ["exchange"];
  coins = result ["coin"];
  quantities.clear ();
  vector <string> values = result ["quantity"];
  for (auto c : values) quantities.push_back (str2float (c));
  messages = result ["message"];
}


void failures_expire (SQL & sql)
{
  sql.clear ();
  sql.add ("DELETE FROM failures WHERE stamp < NOW() - INTERVAL 2 DAY;");
  sql.execute ();
}


/*
CREATE TABLE IF NOT EXISTS mintrade (
id INT NOT NULL AUTO_INCREMENT,
exchange VARCHAR(255),
market VARCHAR(255),
coin VARCHAR(255),
quantity DOUBLE,
PRIMARY KEY (id)
);
*/


// Returns all minimum trade sizes from the database.
// This is the format:
// container [exchange+market+coin] = minimum trade size.
map <string, float> mintrade_get (SQL & sql)
{
  sql.clear ();
  sql.add ("SELECT * FROM mintrade;");
  map <string, vector <string> > result = sql.query ();
  vector <string> exchange = result ["exchange"];
  vector <string> market = result ["market"];
  vector <string> coin = result ["coin"];
  vector <string> quantity = result ["quantity"];
  map <string, float> minimum_trades;
  for (unsigned int i = 0; i < exchange.size (); i++) {
    // Trim the values, because it has been seen that a user had entered a space after a name.
    string key = trim (exchange[i]) + trim (market[i]) + trim (coin[i]);
    float value = str2float (quantity[i]);
    minimum_trades [key] = value;
  }
  return minimum_trades;
}


/*
 CREATE TABLE IF NOT EXISTS pairs (
 id INT NOT NULL AUTO_INCREMENT,
 exchange1 VARCHAR(255),
 exchange2 VARCHAR(255),
 market VARCHAR(255),
 coin VARCHAR(255),
 days INT,
 PRIMARY KEY (id)
 );
 */


void pairs_add (SQL & sql, string exchange1, string exchange2, string market, string coin, int days)
{
  sql.clear ();
  sql.add ("INSERT INTO pairs (exchange1, exchange2, market, coin, days) VALUES (");
  sql.add (exchange1);
  sql.add (",");
  sql.add (exchange2);
  sql.add (",");
  sql.add (market);
  sql.add (",");
  sql.add (coin);
  sql.add (",");
  sql.add (days);
  sql.add (");");
  sql.execute ();
}


void pairs_get (SQL & sql, vector <string> & exchanges1, vector <string> & exchanges2, vector <string> & markets, vector <string> & coins, vector <int> & days)
{
  sql.clear ();
  sql.add ("SELECT * FROM pairs ORDER BY coin, market;");
  map <string, vector <string> > result = sql.query ();
  exchanges1 = result ["exchange1"];
  exchanges2 = result ["exchange2"];
  markets = result ["market"];
  coins = result ["coin"];
  days.clear ();
  vector <string> values = result ["days"];
  for (auto value : values) {
    days.push_back (str2int (value));
  }
}


void pairs_remove (SQL & sql, string exchange1, string exchange2, string market, string coin)
{
  sql.clear ();
  sql.add ("DELETE FROM pairs WHERE exchange1 =");
  sql.add (exchange1);
  sql.add ("AND exchange2 =");
  sql.add (exchange2);
  sql.add ("AND market =");
  sql.add (market);
  sql.add ("AND coin = ");
  sql.add (coin);
  sql.add (";");
  sql.execute ();
}


/*
CREATE TABLE IF NOT EXISTS pausetrades (
id INT NOT NULL AUTO_INCREMENT,
exchange VARCHAR(255),
market VARCHAR(255),
coin VARCHAR(255),
trading INT,
reason VARCHAR(5000),
PRIMARY KEY (id)
);
 */


void pausetrade_set (SQL & sql,
                     const string & exchange, const string & market, const string & coin,
                     int seconds, const string & reason)
{
  sql.clear ();
  sql.add ("SELECT COUNT(*) FROM pausetrades WHERE exchange =");
  sql.add (exchange);
  sql.add ("AND market =");
  sql.add (market);
  sql.add ("AND coin =");
  sql.add (coin);
  vector <string> results = sql.query () ["COUNT(*)"];
  int count = 0;
  if (!results.empty ()) {
    count = stoi (results[0]);
  }
  sql.clear();
  if (count) {
    sql.add ("UPDATE pausetrades SET trading =");
    sql.add (seconds);
    sql.add ("WHERE exchange =");
    sql.add (exchange);
    sql.add ("AND market =");
    sql.add (market);
    sql.add ("AND coin =");
    sql.add (coin);
  } else {
    sql.add ("INSERT INTO pausetrades (exchange, market, coin, trading, reason) VALUES (");
    sql.add (exchange);
    sql.add (",");
    sql.add (market);
    sql.add (",");
    sql.add (coin);
    sql.add (",");
    sql.add (seconds);
    sql.add (",");
    sql.add (reason);
    sql.add (")");
  }
  sql.add (";");
  sql.execute ();
}


// This returns a container of exchanges + coins that have their trading paused.
// Storage format: exchange+coin.
vector <string> pausetrade_get (SQL & sql)
{
  sql.clear ();
  // The "trading" field indicates the seconds after which the wallet can trade again.
  // It is used to temporarily disable this exchange/coin pair for trading.
  // If the field is zero, the pair is not trading.
  // If it's non-zero, the current second after the Unix epoch should be larger than the value of the field.
  sql.add ("SELECT CONCAT(exchange,market,coin) FROM pausetrades WHERE trading AND UNIX_TIMESTAMP() < trading;");
  return sql.query () ["CONCAT(exchange,market,coin)"];
}


string allrates_database (const string & exchange, const string & market, const string & coin)
{
  return exchange + "_" + market + "_" + coin;
}


string allrates_database (int second)
{
  return "allrates_" + to_string (second);
}


void allrates_add (const string & exchange, const string & market, const string & coin,
                   int second, float bid, float ask)
{
  string database = allrates_database (exchange, market, coin);
  // It tests on file size.
  // If data was read from an empty database, the file size will still be zero.
  // So here, it will still create the table.
  // If just testing for the file's existence,
  // it would consider the database to be already there,
  // and so not create the table.
  bool exists = filesize (SqliteDatabase::path (database));
  SqliteDatabase sql (database);
  if (!exists) {
    sql.add ("CREATE TABLE IF NOT EXISTS pattern (second integer, bid real, ask real);");
    sql.execute ();
  }
  sql.clear ();
  sql.add ("PRAGMA synchronous = OFF;");
  sql.execute ();
  sql.clear ();
  sql.add ("INSERT INTO pattern (second, bid, ask) VALUES (");
  sql.add (second);
  sql.add (",");
  sql.add (bid);
  sql.add (",");
  sql.add (ask);
  sql.add (");");
  sql.execute ();
}


void allrates_add (int second,
                   const vector <string> & exchanges,
                   const vector <string> & markets,
                   const vector <string> & coins,
                   const vector <float> & bids,
                   const vector <float> & asks)
{
  string database = allrates_database (second);
  bool exists = filesize (SqliteDatabase::path (database));
  SqliteDatabase sql (database);
  if (!exists) {
    sql.add ("CREATE TABLE IF NOT EXISTS pattern (exchange text, market text, coin text, bid real, ask real);");
    sql.execute ();
  }
  sql.clear ();
  sql.add ("PRAGMA synchronous = OFF;");
  sql.execute ();
  for (unsigned int i = 0; i < exchanges.size(); i++) {
    sql.clear ();
    sql.add ("INSERT INTO pattern (exchange, market, coin, bid, ask) VALUES (");
    sql.add (exchanges[i]);
    sql.add (",");
    sql.add (markets[i]);
    sql.add (",");
    sql.add (coins[i]);
    sql.add (",");
    sql.add (bids[i]);
    sql.add (",");
    sql.add (asks[i]);
    sql.add (");");
    sql.execute ();
  }
}


bool allrates_exists (const string & exchange, const string & market, const string & coin)
{
  string database = allrates_database (exchange, market, coin);
  bool exists = filesize (SqliteDatabase::path (database));
  return exists;
}


void allrates_get (const string & exchange, const string & market, const string & coin,
                   vector <int> & seconds, vector <float> & bids, vector <float> & asks)
{
  string database = allrates_database (exchange, market, coin);
  bool exists = filesize (SqliteDatabase::path (database));
  if (!exists) return;
  SqliteDatabase sql (database);
  sql.add ("SELECT * FROM pattern ORDER BY second;");
  map <string, vector <string> > result = sql.query ();
  vector <string> v_seconds = result ["second"];
  vector <string> v_bids = result ["bid"];
  vector <string> v_asks = result ["ask"];
  for (unsigned int i = 0; i < v_seconds.size(); i++) {
    int second = stoi (v_seconds[i]);
    seconds.push_back (second);
    float bid = str2float (v_bids[i]);
    bids.push_back (bid);
    float ask = str2float (v_asks[i]);
    asks.push_back (ask);
  }
}


void allrates_get (const string & database,
                   vector <string> & exchanges,
                   vector <string> & markets,
                   vector <string> & coins,
                   vector <float> & bids,
                   vector <float> & asks)
{
  SqliteDatabase sql (database);
  sql.add ("SELECT * FROM pattern;");
  map <string, vector <string> > result = sql.query ();
  exchanges = result ["exchange"];
  markets = result ["market"];
  coins = result ["coin"];
  vector <string> v_bids = result ["bid"];
  vector <string> v_asks = result ["ask"];
  for (unsigned int i = 0; i < v_bids.size(); i++) {
    float bid = str2float (v_bids[i]);
    bids.push_back (bid);
    float ask = str2float (v_asks[i]);
    asks.push_back (ask);
  }
}


void allrates_delete (const string & exchange, const string & market, const string & coin)
{
  string database = allrates_database (exchange, market, coin);
  string path = SqliteDatabase::path (database);
  unlink (path.c_str());
}


void allrates_expire (const string & exchange, const string & market, const string & coin, int days)
{
  // Connect to the database if it exists.
  string database = allrates_database (exchange, market, coin);
  string path = SqliteDatabase::path (database);
  if (!filesize (path)) return;
  SqliteDatabase sql (database);
  
  // Select the minimum second stored.
  sql.clear ();
  sql.add ("SELECT MIN(second) FROM pattern;");
  vector <string> seconds = sql.query () ["MIN(second)"];
  if (seconds.empty ()) return;
  int minimum_second = atoi (seconds[0].c_str());
  if (!minimum_second) return;
  
  // Records older than this second to be deleted.
  int cut_off_second = seconds_since_epoch () - (days * 24 * 3600);

  // If there's nothing to delete, then just quit.
  // The point of this is to not write to the database without a need.
  // Then the expiry routine elsewhere will eventually remove this entire database from disk.
  if (minimum_second > cut_off_second) return;

  // Expire older seconds.
  sql.clear ();
  sql.add ("DELETE FROM pattern WHERE second <");
  sql.add (cut_off_second);
  sql.add (";");
  sql.execute ();
  
  // Compact the database, so its size on disk shrinks.
  sql.clear ();
  sql.add ("VACUUM;");
  sql.execute ();
}


// Storing the rates in the MySQL database takes far more time than storing the rates in SQLite.
// The result is that running the ./rates, currently, takes far more than a minute.
// And that defeats the purpose of storing the rates every minute.


string pattern_type_increase_day_week ()
{
  return "increase-day-week";
}


string pattern_status_buy_uncertain ()
{
  return "buyuncertain";
}


string pattern_status_buy_placed ()
{
  return "buyplaced";
}


string pattern_status_monitor_rate ()
{
  return "monitorrate";
}


string pattern_status_stop_loss ()
{
  return "stoploss";
}


string pattern_status_sell_done ()
{
  return "selldone";
}


/*
 CREATE TABLE IF NOT EXISTS patterns (
 id INT NOT NULL AUTO_INCREMENT,
 stamp TIMESTAMP,
 exchange VARCHAR(255),
 market VARCHAR(255),
 coin VARCHAR(255),
 reason VARCHAR(255),
 PRIMARY KEY (id)
 );
 */


void patterns_store (SQL & sql,
                     const string & exchange, const string & market, const string & coin, const string & reason)
{
  sql.clear ();
  sql.add ("INSERT INTO patterns (exchange, market, coin, reason) VALUES (");
  sql.add (exchange);
  sql.add (",");
  sql.add (market);
  sql.add (",");
  sql.add (coin);
  sql.add (",");
  sql.add (reason);
  sql.add (");");
  sql.execute ();
}


void patterns_get (SQL & sql,
                   vector <string> & exchanges, vector <string> & markets, vector <string> & coins,
                   vector <string> & reasons,
                   bool only_recent)
{
  sql.clear ();
  sql.add ("SELECT * FROM patterns");
  if (only_recent) {
    sql.add ("WHERE stamp > NOW() - INTERVAL 5 MINUTE");
  }
  sql.add ("ORDER BY id;");
  map <string, vector <string> > result = sql.query ();
  exchanges = result ["exchange"];
  markets = result ["market"];
  coins = result ["coin"];
  reasons = result ["reason"];
}


void patterns_delete (SQL & sql,
                      const string & exchange, const string & market, const string & coin, const string & reason)
{
  sql.clear ();
  sql.add ("DELETE FROM patterns WHERE exchange =");
  sql.add (exchange);
  sql.add ("AND market =");
  sql.add (market);
  sql.add ("AND coin =");
  sql.add (coin);
  sql.add ("AND reason =");
  sql.add (reason);
  sql.add (";");
  sql.execute ();
}


/*
 CREATE TABLE IF NOT EXISTS multipath (
 id INT NOT NULL AUTO_INCREMENT,
 stamp TIMESTAMP,
 exchange VARCHAR(255),
 market1name VARCHAR(255),
 market1quantity DOUBLE,
 ask1 DOUBLE,
 coin1name VARCHAR(255),
 coin1quantity DOUBLE,
 order1id VARCHAR(255),
 coin2name VARCHAR(255),
 coin2quantity DOUBLE,
 bid2 DOUBLE,
 market2name VARCHAR(255),
 market2quantity DOUBLE,
 order2id VARCHAR(255),
 market3name VARCHAR(255),
 market3quantity DOUBLE,
 ask3 DOUBLE,
 coin3name VARCHAR(255),
 coin3quantity DOUBLE,
 order3id VARCHAR(255),
 coin4name VARCHAR(255),
 coin4quantity DOUBLE,
 bid4 DOUBLE,
 market4name VARCHAR(255),
 market4quantity DOUBLE,
 order4id VARCHAR(255),
 gain DOUBLE,
 status INT,
 executing BOOLEAN,
 PRIMARY KEY (id)
 );
*/


void multipath_store (SQL & sql, const multipath & path)
{
  sql.clear ();
  sql.add ("INSERT INTO multipath (exchange, market1name, market1quantity, ask1, coin1name, coin1quantity, order1id, coin2name, coin2quantity, bid2, market2name, market2quantity, order2id, market3name, market3quantity, ask3, coin3name, coin3quantity, order3id, coin4name, coin4quantity, bid4, market4name, market4quantity, order4id, gain, status, executing) VALUES (");
  sql.add (path.exchange);
  sql.add (",");
  sql.add (path.market1name);
  sql.add (",");
  sql.add (path.market1quantity);
  sql.add (",");
  sql.add (path.ask1);
  sql.add (",");
  sql.add (path.coin1name);
  sql.add (",");
  sql.add (path.coin1quantity);
  sql.add (",");
  sql.add (path.order1id);
  sql.add (",");
  sql.add (path.coin2name);
  sql.add (",");
  sql.add (path.coin2quantity);
  sql.add (",");
  sql.add (path.bid2);
  sql.add (",");
  sql.add (path.market2name);
  sql.add (",");
  sql.add (path.market2quantity);
  sql.add (",");
  sql.add (path.order2id);
  sql.add (",");
  sql.add (path.market3name);
  sql.add (",");
  sql.add (path.market3quantity);
  sql.add (",");
  sql.add (path.ask3);
  sql.add (",");
  sql.add (path.coin3name);
  sql.add (",");
  sql.add (path.coin3quantity);
  sql.add (",");
  sql.add (path.order3id);
  sql.add (",");
  sql.add (path.coin4name);
  sql.add (",");
  sql.add (path.coin4quantity);
  sql.add (",");
  sql.add (path.bid4);
  sql.add (",");
  sql.add (path.market4name);
  sql.add (",");
  sql.add (path.market4quantity);
  sql.add (",");
  sql.add (path.order2id);
  sql.add (",");
  sql.add (path.gain);
  sql.add (",");
  sql.add (path.status);
  sql.add (",");
  sql.add (path.executing);
  sql.add (");");
  sql.execute ();
}


vector <multipath> multipath_get (SQL & sql)
{
  vector <multipath> paths;
  sql.clear ();
  // Get the data ordered by identifier.
  // The purpose of ordering them this way is
  // that older and thus active paths get executed till completion,
  // before it considers newly suggested paths.
  sql.add ("SELECT * FROM multipath ORDER BY id;");
  map <string, vector <string> > result = sql.query ();
  vector <string> id = result ["id"];
  vector <string> exchange = result ["exchange"];
  vector <string> market1name = result ["market1name"];
  vector <string> market1quantity = result ["market1quantity"];
  vector <string> ask1 = result ["ask1"];
  vector <string> coin1name = result ["coin1name"];
  vector <string> coin1quantity = result ["coin1quantity"];
  vector <string> order1id = result ["order1id"];
  vector <string> coin2name = result ["coin2name"];
  vector <string> coin2quantity = result ["coin2quantity"];
  vector <string> bid2 = result ["bid2"];
  vector <string> market2name = result ["market2name"];
  vector <string> market2quantity = result ["market2quantity"];
  vector <string> order2id = result ["order2id"];
  vector <string> market3name = result ["market3name"];
  vector <string> market3quantity = result ["market3quantity"];
  vector <string> ask3 = result ["ask3"];
  vector <string> coin3name = result ["coin3name"];
  vector <string> coin3quantity = result ["coin3quantity"];
  vector <string> order3id = result ["order3id"];
  vector <string> coin4name = result ["coin4name"];
  vector <string> coin4quantity = result ["coin4quantity"];
  vector <string> bid4 = result ["bid4"];
  vector <string> market4name = result ["market4name"];
  vector <string> market4quantity = result ["market4quantity"];
  vector <string> order4id = result ["order4id"];
  vector <string> gains = result ["gain"];
  vector <string> status = result ["status"];
  vector <string> executing = result ["executing"];
  for (unsigned int i = 0; i < exchange.size(); i++) {
    multipath path;
    path.id = str2int (id[i]);
    path.exchange = exchange[i];
    path.market1name = market1name[i];
    path.market1quantity = str2float (market1quantity[i]);
    path.ask1 = str2float (ask1[i]);
    path.coin1name = coin1name[i];
    path.coin1quantity = str2float (coin1quantity[i]);
    path.order1id = order1id[i];
    path.coin2name = coin2name[i];
    path.coin2quantity = str2float (coin2quantity[i]);
    path.bid2 = str2float (bid2[i]);
    path.market2name = market2name[i];
    path.market2quantity = str2float (market2quantity[i]);
    path.order2id = order2id[i];
    path.market3name = market3name[i];
    path.market3quantity = str2float (market3quantity[i]);
    path.ask3 = str2float (ask3[i]);
    path.coin3name = coin3name[i];
    path.coin3quantity = str2float (coin3quantity[i]);
    path.order3id = order3id[i];
    path.coin4name = coin4name[i];
    path.coin4quantity = str2float (coin4quantity[i]);
    path.bid4 = str2float (bid4[i]);
    path.market4name = market4name[i];
    path.market4quantity = str2float (market4quantity[i]);
    path.order4id = order4id[i];
    path.gain = str2float (gains[i]);
    path.status = status[i];
    path.executing = str2bool (executing[i]);
    paths.push_back (path);
  }
  return paths;
}


multipath multipath_get (SQL & sql, int id)
{
  // Generate the SQL.
  sql.clear ();
  sql.add ("SELECT * FROM multipath WHERE id =");
  sql.add (id);
  sql.add (";");
  // Execute the query.
  map <string, vector <string> > result = sql.query ();
  vector <string> ids = result ["id"];
  vector <string> exchange = result ["exchange"];
  vector <string> market1name = result ["market1name"];
  vector <string> market1quantity = result ["market1quantity"];
  vector <string> ask1 = result ["ask1"];
  vector <string> coin1name = result ["coin1name"];
  vector <string> coin1quantity = result ["coin1quantity"];
  vector <string> coin2name = result ["coin2name"];
  vector <string> coin2quantity = result ["coin2quantity"];
  vector <string> bid2 = result ["bid2"];
  vector <string> market2name = result ["market2name"];
  vector <string> market2quantity = result ["market2quantity"];
  vector <string> market3name = result ["market3name"];
  vector <string> market3quantity = result ["market3quantity"];
  vector <string> ask3 = result ["ask3"];
  vector <string> coin3name = result ["coin3name"];
  vector <string> coin3quantity = result ["coin3quantity"];
  vector <string> coin4name = result ["coin4name"];
  vector <string> coin4quantity = result ["coin4quantity"];
  vector <string> bid4 = result ["bid4"];
  vector <string> market4name = result ["market4name"];
  vector <string> market4quantity = result ["market4quantity"];
  vector <string> gains = result ["gain"];
  vector <string> status = result ["status"];
  vector <string> executing = result ["executing"];
  // Default path.
  multipath path;
  // If the path was not found, return the default path.
  if (ids.empty ()) return path;
  // Fill the path with the values read from the database.
  path.id = str2int (ids[0]);
  path.exchange = exchange[0];
  path.market1name = market1name[0];
  path.market1quantity = str2float (market1quantity[0]);
  path.ask1 = str2float (ask1[0]);
  path.coin1name = coin1name[0];
  path.coin1quantity = str2float (coin1quantity[0]);
  path.coin2name = coin2name[0];
  path.coin2quantity = str2float (coin2quantity[0]);
  path.bid2 = str2float (bid2[0]);
  path.market2name = market2name[0];
  path.market2quantity = str2float (market2quantity[0]);
  path.market3name = market3name[0];
  path.market3quantity = str2float (market3quantity[0]);
  path.ask3 = str2float (ask3[0]);
  path.coin3name = coin3name[0];
  path.coin3quantity = str2float (coin3quantity[0]);
  path.coin4name = coin4name[0];
  path.coin4quantity = str2float (coin4quantity[0]);
  path.bid4 = str2float (bid4[0]);
  path.market4name = market4name[0];
  path.market4quantity = str2float (market4quantity[0]);
  path.gain = str2float (gains[0]);
  path.status = status[0];
  path.executing = str2bool (executing[0]);
  // Done.
  return path;
}


void multipath_update (SQL & sql, const multipath & path)
{
  sql.clear ();
  sql.add ("UPDATE multipath SET market1quantity =");
  sql.add (path.market1quantity);
  sql.add (", ask1 =");
  sql.add (path.ask1);
  sql.add (", coin1quantity =");
  sql.add (path.coin1quantity);
  sql.add (", order1id =");
  sql.add (path.order1id);
  sql.add (", coin2quantity =");
  sql.add (path.coin2quantity);
  sql.add (", bid2 =");
  sql.add (path.bid2);
  sql.add (", market2quantity =");
  sql.add (path.market2quantity);
  sql.add (", order2id =");
  sql.add (path.order2id);
  sql.add (", market3quantity =");
  sql.add (path.market3quantity);
  sql.add (", ask3 =");
  sql.add (path.ask3);
  sql.add (", coin3quantity =");
  sql.add (path.coin3quantity);
  sql.add (", order3id =");
  sql.add (path.order3id);
  sql.add (", coin4quantity =");
  sql.add (path.coin4quantity);
  sql.add (", bid4 =");
  sql.add (path.bid4);
  sql.add (", market4quantity =");
  sql.add (path.market4quantity);
  sql.add (", order4id =");
  sql.add (path.order4id);
  sql.add (", gain =");
  sql.add (path.gain);
  sql.add (", status =");
  sql.add (path.status);
  sql.add (", executing =");
  sql.add (path.executing);
  sql.add ("WHERE id =");
  sql.add (path.id);
  sql.add (";");
  sql.execute ();
}


// Return true if both paths are equal with regard to their exchange and coin and market names.
// It does not consider quantities and rates.
bool multipath_equal (const multipath & path1, const multipath & path2)
{
  if (path1.exchange != path2.exchange) return false;
  if (path1.market1name != path2.market1name) return false;
  if (path1.coin1name != path2.coin1name) return false;
  if (path1.coin2name != path2.coin2name) return false;
  if (path1.market2name != path2.market2name) return false;
  if (path1.market3name != path2.market3name) return false;
  if (path1.coin3name != path2.coin3name) return false;
  if (path1.coin4name != path2.coin4name) return false;
  if (path1.market4name != path2.market4name) return false;
  return true;
}


void multipath_expire (SQL & sql)
{
  // Expire multipaths very soon, except the ones with status "done".
  sql.clear ();
  sql.add ("DELETE FROM multipath WHERE stamp < NOW() - INTERVAL 5 HOUR AND status !=");
  sql.add (string (multipath_status::done));
  sql.add (";");
  // Expire the multipaths after many days.
  // So they're left there long enough for statistical purposes.
  sql.execute ();
  sql.clear ();
  sql.add ("DELETE FROM multipath WHERE stamp < NOW() - INTERVAL 40 DAY;");
  sql.execute ();
}


void multipath_delete (SQL & sql, const multipath & path)
{
  sql.clear ();
  sql.add ("DELETE FROM multipath WHERE id =");
  sql.add (path.id);
  sql.add (";");
  sql.execute ();
}


/*
 CREATE TABLE IF NOT EXISTS settings (
 id INT NOT NULL AUTO_INCREMENT,
 host VARCHAR(255),
 setting VARCHAR(255),
 value VARCHAR(255),
 PRIMARY KEY (id)
 );
 */


void settings_set (SQL & sql, const string & host, const string & setting, const string & value)
{
  // Since it deletes first before inserting, use a transaction.
  sql.clear ();
  sql.add ("START TRANSACTION;");
  sql.execute ();
  
  // Delete possible old setting.
  sql.clear();
  sql.add ("DELETE FROM settings WHERE host =");
  sql.add (host);
  sql.add ("AND setting =");
  sql.add (setting);
  sql.add (";");
  sql.execute ();
  // Store the new setting.
  sql.clear ();
  sql.add ("INSERT INTO settings (host, setting, value) VALUES (");
  sql.add (host);
  sql.add (",");
  sql.add (setting);
  sql.add (",");
  sql.add (value);
  sql.add (");");
  sql.execute ();

  // Done.
  sql.clear ();
  sql.add ("COMMIT;");
  sql.execute ();
}


string settings_get (SQL & sql, const string & host, const string & setting)
{
  sql.clear ();
  sql.add ("SELECT value FROM settings WHERE host =");
  sql.add (host);
  sql.add ("AND setting =");
  sql.add (setting);
  sql.add (";");
  vector <string> values = sql.query () ["value"];
  if (values.empty ()) {
    return "";
  }
  return values[0];
}


string books_database (const string & exchange, const string & market, const string & coin)
{
  return "books_" + exchange + "_" + market + "_" + coin;
}


const char * books_buyers_table ()
{
  return "bids";
}


const char * books_sellers_table ()
{
  return "asks";
}


const char * books_buyers_reference_table ()
{
  return "buyref";
}


const char * books_sellers_reference_table ()
{
  return "sellref";
}


// This will store the order book.
// $buyers: Whether storing the buyers.
// $sellers: Whether storing the sellers.
// $exchange $market $coin: Where the order book applies to.
// $price: The reference price to store along with the order book.
// $quantities $rates: The contents of the order book.
// This will store the order books, not in MySQL online,
// but in SQLite locally.
// This is faster. And it does not burden the MySQL database at the front bot.
// And since the approximate order books are used for the analyzers running at the back bots,
// there is no need to send the information over the wires to the front bot.
void order_books_set (bool buyers, bool sellers,
                      const string & exchange,
                      const string & market,
                      const string & coin,
                      float price,
                      const vector <float> & quantities,
                      const vector <float> & rates)
{
  string database = books_database (exchange, market, coin);
  // It tests on file size.
  // If data was read from an empty database, the file size will still be zero.
  // So here, it will still create the table.
  // If just testing for the file's existence,
  // it would consider the database to be already there,
  // and so not create the table.
  bool exists = filesize (SqliteDatabase::path (database));

  // Connect to the database.
  SqliteDatabase sql (database);

  // Create the tables if they don't exist yet.
  if (!exists) {
    sql.clear ();
    sql.add ("CREATE TABLE IF NOT EXISTS");
    sql.add (books_buyers_table ());
    sql.add ("(quantity real, rate real);");
    sql.execute ();
    sql.clear ();
    sql.add ("CREATE TABLE IF NOT EXISTS");
    sql.add (books_sellers_table ());
    sql.add ("(quantity real, rate real);");
    sql.execute ();
    sql.clear ();
    sql.add ("CREATE TABLE IF NOT EXISTS");
    sql.add (books_buyers_reference_table ());
    sql.add ("(price real);");
    sql.execute ();
    sql.clear ();
    sql.add ("CREATE TABLE IF NOT EXISTS");
    sql.add (books_sellers_reference_table ());
    sql.add ("(price real);");
    sql.execute ();
  }

  sql.clear ();
  sql.add ("PRAGMA synchronous = OFF;");
  sql.execute ();

  // Delete existing quantities and rates before storing the new.
  // It uses the SQLite truncate optimization.
  sql.clear ();
  sql.add ("DELETE FROM");
  if (buyers) sql.add (books_buyers_table ());
  if (sellers) sql.add (books_sellers_table ());
  sql.add (";");
  sql.execute ();

  // Store the new quantities and rates.
  for (unsigned int i = 0; i < quantities.size(); i++) {
    sql.clear ();
    sql.add ("INSERT INTO");
    if (buyers) sql.add (books_buyers_table ());
    if (sellers) sql.add (books_sellers_table ());
    sql.add ("(quantity, rate) VALUES (");
    sql.add (quantities[i]);
    sql.add (",");
    sql.add (rates[i]);
    sql.add (");");
    sql.execute ();
  }

  // In the same manner, store the reference price.
  sql.clear ();
  sql.add ("DELETE FROM");
  if (buyers) sql.add (books_buyers_reference_table ());
  if (sellers) sql.add (books_sellers_reference_table ());
  sql.add (";");
  sql.execute ();
  sql.clear ();
  sql.add ("INSERT INTO");
  if (buyers) sql.add (books_buyers_reference_table ());
  if (sellers) sql.add (books_sellers_reference_table ());
  sql.add ("(price) VALUES (");
  sql.add (price);
  sql.add (");");
  sql.execute ();
  
  // It would be good practise, normally, to do a VACUUM now and then.
  // But it appears that this is not needed in this case.
  // The file sizes were observed for some days.
  // The file sizes of all order book databases remain 20480 for all of them.
  // So there's no need to VACUUM the databases.
}


void order_books_get (bool buyers, bool sellers,
                      const string & exchange,
                      const string & market,
                      const string & coin,
                      float & price,
                      vector <float> & quantities,
                      vector <float> & rates)
{
  // Clear all output containers.
  price = 0;
  quantities.clear ();
  rates.clear ();
  
  string database = books_database (exchange, market, coin);
  // It tests on file size.
  // If data was read from an empty database, the file size will still be zero.
  // So here, it will still create the table.
  // If just testing for the file's existence,
  // it would consider the database to be already there,
  // and so not create the table.
  bool exists = filesize (SqliteDatabase::path (database));
  
  // If the database does not exist, there's no data: Done.
  if (!exists) return;
  
  // Open the database.
  SqliteDatabase sql (database);
  
  // Read the quantities and the rates.
  sql.add ("SELECT * FROM");
  if (buyers) sql.add (books_buyers_table ());
  if (sellers) sql.add (books_sellers_table ());
  sql.add ("ORDER BY rowid;");
  map <string, vector <string> > result = sql.query ();
  vector <string> v_quantities = result ["quantity"];
  vector <string> v_rates = result ["rate"];
  for (unsigned int i = 0; i < v_quantities.size(); i++) {
    float quantity = str2float (v_quantities[i]);
    quantities.push_back (quantity);
    float rate = str2float (v_rates[i]);
    rates.push_back (rate);
  }
  
  // Read the reference price.
  sql.clear ();
  sql.add ("SELECT * FROM");
  if (buyers) sql.add (books_buyers_reference_table ());
  if (sellers) sql.add (books_sellers_reference_table ());
  sql.add (";");
  result = sql.query ();
  vector <string> v_prices = result ["price"];
  if (!v_prices.empty ()) {
    price = str2float (v_prices[0]);
  }
}


void buyers_set (const string & exchange,
                 const string & market,
                 const string & coin,
                 float price,
                 const vector <float> & quantities,
                 const vector <float> & rates)
{
  order_books_set (true, false, exchange, market, coin, price, quantities, rates);
}


void sellers_set (const string & exchange,
                  const string & market,
                  const string & coin,
                  float price,
                  const vector <float> & quantities,
                  const vector <float> & rates)
{
  order_books_set (false, true, exchange, market, coin, price, quantities, rates);
}


void buyers_get (const string & exchange,
                 const string & market,
                 const string & coin,
                 float & price,
                 vector <float> & quantities,
                 vector <float> & rates)
{
  order_books_get (true, false, exchange, market, coin, price, quantities, rates);
}


void sellers_get (const string & exchange,
                  const string & market,
                  const string & coin,
                  float & price,
                  vector <float> & quantities,
                  vector <float> & rates)
{
  order_books_get (false, true, exchange, market, coin, price, quantities, rates);
}


void buyers_sellers_delete (const string & exchange, const string & market, const string & coin)
{
  string database = books_database (exchange, market, coin);
  string path = SqliteDatabase::path (database);
  unlink (path.c_str());
}


/*
 CREATE TABLE IF NOT EXISTS coinsmarket (
 id INT NOT NULL AUTO_INCREMENT,
 exchange VARCHAR(255),
 market VARCHAR(255),
 coins VARCHAR(10000),
 PRIMARY KEY (id)
 );
 */


// Reads all coins from the database.
// Container $coins has all coins supported by the market at the exchange, separated by spaces.
void coins_market_get (SQL & sql, vector <string> & exchanges, vector <string> & markets, vector <string> & coins)
{
  sql.clear ();
  sql.add ("SELECT exchange, market, coins FROM coinsmarket;");
  map <string, vector <string> > result = sql.query ();
  exchanges = result ["exchange"];
  markets = result ["market"];
  coins = result ["coins"];
}


// Writes one record to the database.
// $exchange, $market: The exchange-market combination.
// $coins: All coins supported by the $market at the $exchange, separated by spaces.
void coins_market_set (SQL & sql, const string & exchange, const string & market, const string & coins)
{
  // Delete this record.
  sql.clear ();
  sql.add ("DELETE FROM coinsmarket WHERE exchange =");
  sql.add (exchange);
  sql.add ("AND market =");
  sql.add (market);
  sql.add (";");
  sql.execute ();
  // If no coins given: Ready.
  // This means that the database record will remain deleted.
  if (coins.empty ()) return;
  // Store the new record.
  sql.clear();
  sql.add ("INSERT INTO coinsmarket (exchange, market, coins) VALUES (");
  sql.add (exchange);
  sql.add (",");
  sql.add (market);
  sql.add (",");
  sql.add (coins);
  sql.add (")");
  sql.add (";");
  sql.execute ();
}


/*
 CREATE TABLE IF NOT EXISTS optimizer (
 id INT NOT NULL AUTO_INCREMENT,
 stamp TIMESTAMP,
 exchange VARCHAR(255),
 coin VARCHAR(255),
 rate DOUBLE,
 projected DOUBLE,
 realized DOUBLE,
 PRIMARY KEY (id)
 );
 */


void optimizer_get (SQL & sql,
                    vector <int> & ids,
                    vector <string> & exchanges,
                    vector <string> & coins,
                    vector <float> & starts,
                    vector <float> & forecasts,
                    vector <float> & realizeds)
{
  sql.clear ();
  sql.add ("SELECT * FROM optimizer ORDER BY id;");
  map <string, vector <string> > result = sql.query ();

  vector <string> values;

  ids.clear ();
  values = result ["id"];
  for (auto s : values) ids.push_back (str2int (s));

  exchanges = result ["exchange"];
  
  coins = result ["coin"];
  
  starts.clear ();
  values = result ["start"];
  for (auto s : values) starts.push_back (str2float (s));

  forecasts.clear ();
  values = result ["forecast"];
  for (auto s : values) forecasts.push_back (str2float (s));
  
  realizeds.clear ();
  values = result ["realized"];
  for (auto s : values) realizeds.push_back (str2float (s));
}


void optimizer_add (SQL & sql, const string & exchange, const string & coin, float start, float forecast)
{
  sql.clear ();
  sql.add ("INSERT INTO optimizer (exchange, coin, start, forecast) VALUES (");
  sql.add (exchange);
  sql.add (",");
  sql.add (coin);
  sql.add (",");
  sql.add (start);
  sql.add (",");
  sql.add (forecast);
  sql.add (");");
  sql.execute ();
}


void optimizer_update (SQL & sql, int identifier, float realized)
{
  sql.clear();
  sql.add ("UPDATE optimizer SET realized =");
  sql.add (realized);
  sql.add ("WHERE id =");
  sql.add (identifier);
  sql.add (";");
  sql.execute ();
}


void optimizer_delete (SQL & sql, int identifier)
{
  sql.clear ();
  sql.add ("DELETE FROM optimizer WHERE id =");
  sql.add (identifier);
  sql.add (";");
  sql.execute ();
}


/*
 CREATE TABLE IF NOT EXISTS weights (
 id INT NOT NULL AUTO_INCREMENT,
 stamp TIMESTAMP,
 exchange VARCHAR(255),
 market VARCHAR(255),
 coin VARCHAR(255),
 weights VARCHAR(2048),
 sum_absolute_residuals DOUBLE,
 max_residual_percents DOUBLE,
 avg_residual_percents DOUBLE,
 PRIMARY KEY (id)
 );
 */


void weights_get (SQL & sql, const string & exchange, const string & market, const string & coin,
                  string & weights,
                  float & sum_absolute_residuals,
                  float & maximum_residual_percentage,
                  float & average_residual_percentage)
{
  sql.clear ();
  sql.add ("SELECT * FROM weights WHERE exchange =");
  sql.add (exchange);
  sql.add ("AND market =");
  sql.add (market);
  sql.add ("AND coin =");
  sql.add (coin);
  sql.add (";");
  map <string, vector <string> > result = sql.query ();
  if (!result.empty ()) {
    weights = result ["weights"].front ();
    sum_absolute_residuals = str2float (result ["sum_absolute_residuals"].front ());
    maximum_residual_percentage = str2float (result ["max_residual_percents"].front ());
    average_residual_percentage = str2float (result ["avg_residual_percents"].front ());
  } else {
    weights.clear ();
    sum_absolute_residuals = numeric_limits<float>::max();
    maximum_residual_percentage = numeric_limits<float>::max();
    average_residual_percentage = numeric_limits<float>::max();
  }
}


void weights_save (SQL & sql, const string & exchange, const string & market, const string & coin,
                   const string weights,
                   float & sum_absolute_residuals,
                   float & maximum_residual_percentage,
                   float & average_residual_percentage)
{
  weights_delete (sql, exchange, market, coin);
  sql.clear ();
  sql.add ("INSERT INTO weights (exchange, market, coin, weights, sum_absolute_residuals, max_residual_percents, avg_residual_percents) VALUES (");
  sql.add (exchange);
  sql.add (",");
  sql.add (market);
  sql.add (",");
  sql.add (coin);
  sql.add (",");
  sql.add (weights);
  sql.add (",");
  sql.add (sum_absolute_residuals);
  sql.add (",");
  sql.add (maximum_residual_percentage);
  sql.add (",");
  sql.add (average_residual_percentage);
  sql.add (");");
  sql.execute ();
}


void weights_delete (SQL & sql, const string & exchange, const string & market, const string & coin)
{
  sql.clear ();
  sql.add ("DELETE FROM weights WHERE exchange =");
  sql.add (exchange);
  sql.add ("AND market =");
  sql.add (market);
  sql.add ("AND coin =");
  sql.add (coin);
  sql.add (";");
  sql.execute ();
}


string weights_encode (float hourly, float daily, float weekly,
                       unordered_map <int, float> & curves)
{
  Object main_object;
  main_object << "hourly" << hourly;
  main_object << "daily" << daily;
  main_object << "weekly" << weekly;
  Object curve_object;
  for (auto element : curves) {
    curve_object << to_string (element.first) << element.second;
  }
  main_object << "curves" << curve_object;
  return main_object.json ();
}


void weights_decode (string json,
                     float & hourly, float & daily, float & weekly,
                     unordered_map <int, float> & curve)
{
  hourly = 1;
  daily = 1;
  weekly = 1;
  if (json.empty ()) return;
  Object main;
  main.parse (json);
  if (main.has<Number>("hourly")) {
    hourly = main.get<Number>("hourly");
  }
  if (main.has<Number>("daily")) {
    daily = main.get<Number>("daily");
  }
  if (main.has<Number>("weekly")) {
    weekly = main.get<Number>("weekly");
  }
  if (main.has<Object>("curves")) {
    Object curve_object = main.get<Object>("curves");
    map <string, Value *> json_map = curve_object.kv_map ();
    for (auto key_value : json_map) {
      string hour = key_value.first;
      float weight = key_value.second->get<Number>();
      curve [str2int (hour)] = weight;
    }
  }
}


/*
 CREATE TABLE IF NOT EXISTS surfer (
 id INT NOT NULL AUTO_INCREMENT,
 stamp TIMESTAMP,
 coin VARCHAR(255),
 trade VARCHAR(255),
 status VARCHAR(255),
 rate DOUBLE,
 amount DOUBLE,
 remark VARCHAR(10000),
 PRIMARY KEY (id)
 );
 */


string surfer_buy ()
{
  return "buy";
}


string surfer_sell ()
{
  return "sell";
}


void surfer_get (SQL & sql,
                 vector <int> & ids,
                 vector <int> & stamps,
                 vector <string> & coins,
                 vector <string> & trades,
                 vector <string> & statuses,
                 vector <float> & rates,
                 vector <float> & amounts,
                 vector <string> & remarks)
{
  sql.clear ();
  sql.add ("SELECT *, UNIX_TIMESTAMP(stamp) FROM surfer ORDER BY id;");
  map <string, vector <string> > result = sql.query ();
  
  vector <string> values;
  
  ids.clear ();
  values = result ["id"];
  for (auto s : values) ids.push_back (str2int (s));
  
  stamps.clear ();
  values = result ["UNIX_TIMESTAMP(stamp)"];
  for (auto s : values) stamps.push_back (str2int (s));

  coins = result ["coin"];

  trades = result ["trade"];

  statuses = result ["status"];

  rates.clear ();
  values = result ["rate"];
  for (auto s : values) rates.push_back (str2float (s));
  
  amounts.clear ();
  values = result ["amount"];
  for (auto s : values) amounts.push_back (str2float (s));

  remarks = result ["remark"];
}


void surfer_add (SQL & sql, const string & coin, const string & trade, const string & status)
{
  sql.clear ();
  sql.add ("INSERT INTO surfer (coin, trade, status) VALUES (");
  sql.add (coin);
  sql.add (",");
  sql.add (trade);
  sql.add (",");
  sql.add (status);
  sql.add (");");
  sql.execute ();
}


void surfer_update (SQL & sql, int identifier, const string & status, float rate, float amount)
{
  sql.clear();
  sql.add ("UPDATE surfer SET");
  sql.add ("status =");
  sql.add (status);
  sql.add (",");
  sql.add ("rate =");
  sql.add (rate);
  sql.add (",");
  sql.add ("amount =");
  sql.add (amount);
  sql.add ("WHERE id =");
  sql.add (identifier);
  sql.add (";");
  sql.execute ();
}


void surfer_update (SQL & sql, int identifier, const string & status, const string & remark)
{
  sql.clear();
  sql.add ("UPDATE surfer SET");
  sql.add ("status =");
  sql.add (status);
  sql.add (",");
  sql.add ("remark =");
  sql.add (remark);
  sql.add ("WHERE id =");
  sql.add (identifier);
  sql.add (";");
  sql.execute ();
}


void surfer_delete (SQL & sql, int identifier)
{
  sql.clear ();
  sql.add ("DELETE FROM surfer WHERE id =");
  sql.add (identifier);
  sql.add (";");
  sql.execute ();
}


/*
 CREATE TABLE IF NOT EXISTS bought (
 id INT NOT NULL AUTO_INCREMENT,
 stamp TIMESTAMP,
 exchange VARCHAR(255),
 market VARCHAR(255),
 coin VARCHAR(255),
 rate DOUBLE,
 PRIMARY KEY (id)
 );
 */


void bought_get (SQL & sql,
                 vector <int> & ids,
                 vector <int> & hours,
                 vector <string> & exchanges,
                 vector <string> & markets,
                 vector <string> & coins,
                 vector <float> & rates)
{
  sql.clear ();
  sql.add ("SELECT *, TIMESTAMPDIFF(HOUR,stamp,NOW()) FROM bought ORDER BY id;");
  map <string, vector <string> > result = sql.query ();
  
  vector <string> values;
  
  ids.clear ();
  values = result ["id"];
  for (auto s : values) ids.push_back (str2int (s));

  hours.clear ();
  values = result ["TIMESTAMPDIFF(HOUR,stamp,NOW())"];
  for (auto & v : values) {
    int hour = 0;
    if (!v.empty ()) hour = stoi (v);
    hours.push_back (hour);
  }

  exchanges = result ["exchange"];
  
  markets = result ["market"];

  coins = result ["coin"];
  
  rates.clear ();
  values = result ["rate"];
  for (auto s : values) rates.push_back (str2float (s));
}


void bought_add (SQL & sql, string exchange, string market, string coin, float rate)
{
  sql.clear ();
  sql.add ("INSERT INTO bought (exchange, market, coin, rate) VALUES (");
  sql.add (exchange);
  sql.add (",");
  sql.add (market);
  sql.add (",");
  sql.add (coin);
  sql.add (",");
  sql.add (rate);
  sql.add (");");
  sql.execute ();
}


void bought_delete (SQL & sql, int identifier)
{
  sql.clear ();
  sql.add ("DELETE FROM bought WHERE id =");
  sql.add (identifier);
  sql.add (";");
  sql.execute ();
}


/*
 CREATE TABLE IF NOT EXISTS sold (
 id INT NOT NULL AUTO_INCREMENT,
 stamp TIMESTAMP,
 exchange VARCHAR(255),
 market VARCHAR(255),
 coin VARCHAR(255),
 quantity DOUBLE,
 buyrate DOUBLE,
 sellrate DOUBLE,
 PRIMARY KEY (id)
 );
 */


// Store the details of a sale in the database.
void sold_store (SQL & sql, string exchange, string market, string coin, float quantity, float buyrate, float sellrate)
{
  sql.clear ();
  sql.add ("INSERT INTO sold (exchange, market, coin, quantity, buyrate, sellrate) VALUES (");
  sql.add (exchange);
  sql.add (",");
  sql.add (market);
  sql.add (",");
  sql.add (coin);
  sql.add (",");
  sql.add (quantity);
  sql.add (",");
  sql.add (buyrate);
  sql.add (",");
  sql.add (sellrate);
  sql.add (");");
  sql.execute ();
}


// Get all details of all pattern-based sales.
void sold_get (SQL & sql, vector <int> & ids,
               vector <int> & hours,
               vector <string> & exchanges, vector <string> & markets, vector <string> & coins,
               vector <float> & quantities, vector <float> & buyrates, vector <float> & sellrates)
{
  sql.clear ();
  sql.add ("SELECT *, TIMESTAMPDIFF(HOUR,stamp,NOW()) FROM sold ORDER BY id;");
  map <string, vector <string> > result = sql.query ();
  vector <string> values;
  ids.clear ();
  values = result ["id"];
  for (auto & v : values) ids.push_back (stoi (v));
  hours.clear ();
  values = result ["TIMESTAMPDIFF(HOUR,stamp,NOW())"];
  for (auto & v : values) {
    int hour = 0;
    if (!v.empty ()) hour = stoi (v);
    hours.push_back (hour);
  }
  exchanges = result ["exchange"];
  markets = result ["market"];
  coins = result ["coin"];
  quantities.clear ();
  values = result ["quantity"];
  for (auto & v : values) quantities.push_back (str2float (v));
  buyrates.clear ();
  values = result ["buyrate"];
  for (auto & v : values) buyrates.push_back (str2float (v));
  sellrates.clear ();
  values = result ["sellrate"];
  for (auto & v : values) sellrates.push_back (str2float (v));
}


// Delete a sale record.
void sold_delete (SQL & sql, int id)
{
  sql.clear ();
  sql.add ("DELETE FROM sold WHERE id =");
  sql.add (id);
  sql.add (";");
  sql.execute ();
}


/*
 CREATE TABLE IF NOT EXISTS comments (
 id INT NOT NULL AUTO_INCREMENT,
 stamp TIMESTAMP,
 coin VARCHAR(255),
 comment VARCHAR(10000),
 PRIMARY KEY (id)
 );
 */


void comments_store (SQL & sql, string coin, string comment)
{
  sql.clear ();
  sql.add ("INSERT INTO comments (coin, comment) VALUES (");
  sql.add (coin);
  sql.add (",");
  sql.add (comment);
  sql.add (");");
  sql.execute ();
}


void comments_get (SQL & sql,
                   string coin,
                   vector <int> & ids,
                   vector <string> & stamps,
                   vector <string> & coins,
                   vector <string> & comments)
{
  sql.clear ();
  sql.add ("SELECT * FROM comments");
  if (!coin.empty ()) {
    sql.add ("WHERE coin =");
    sql.add (coin);
  }
  sql.add ("ORDER BY id;");
  map <string, vector <string> > result = sql.query ();
  vector <string> values;
  ids.clear ();
  values = result ["id"];
  for (auto & v : values) ids.push_back (stoi (v));
  stamps = result ["stamp"];
  coins = result ["coin"];
  comments = result ["comment"];
}


void comments_delete (SQL & sql, int id)
{
  sql.clear ();
  sql.add ("DELETE FROM comments WHERE id =");
  sql.add (id);
  sql.add (";");
  sql.execute ();
}


/*
 CREATE TABLE IF NOT EXISTS conversions (
 id INT NOT NULL AUTO_INCREMENT,
 one VARCHAR(255),
 two VARCHAR(255),
 PRIMARY KEY (id)
 );
 */


// Convert Bitcoin Cash new address to old address:
// https://explorer.bitcoin.com/bch

string conversions_run (SQL & sql, string s)
{
  sql.clear ();
  sql.add ("SELECT * FROM conversions;");
  map <string, vector <string> > result = sql.query ();
  vector <string> one = result ["one"];
  vector <string> two = result ["two"];
  string converted;
  for (unsigned int i = 0; i < one.size(); i++) {
    if (!converted.empty ()) continue;
    if (s == one[i]) converted = two[i];
    if (!converted.empty ()) continue;
    if (s == two[i]) converted = one[i];
  }
  if (converted.empty ()) converted = s;
  return converted;
}


/*
 CREATE TABLE IF NOT EXISTS availables (
 id INT NOT NULL AUTO_INCREMENT,
 stamp TIMESTAMP,
 second INT,
 seller VARCHAR(255),
 buyer VARCHAR(255),
 market VARCHAR(255),
 coin VARCHAR(255),
 bidsize DOUBLE,
 asksize DOUBLE,
 realized DOUBLE,
 PRIMARY KEY (id)
 );
 */


void availables_store (SQL & sql,
                       string stamp,
                       int second,
                       string seller,
                       string buyer,
                       string market,
                       string coin,
                       float bidsize,
                       float asksize,
                       float realized)
{
  sql.clear ();
  sql.add ("INSERT INTO availables (");
  if (!stamp.empty ()) sql.add ("stamp,");
  sql.add ("second, seller, buyer, market, coin, bidsize, asksize, realized) VALUES (");
  if (!stamp.empty ()) {
    sql.add (stamp);
    sql.add (",");
  }
  sql.add (second);
  sql.add (",");
  sql.add (seller);
  sql.add (",");
  sql.add (buyer);
  sql.add (",");
  sql.add (market);
  sql.add (",");
  sql.add (coin);
  sql.add (",");
  sql.add (bidsize);
  sql.add (",");
  sql.add (asksize);
  sql.add (",");
  sql.add (realized);
  sql.add (");");
  sql.execute ();
}


void availables_get (SQL & sql,
                     vector <int> & ids,
                     vector <string> & stamps,
                     vector <int> & seconds,
                     vector <string> & sellers,
                     vector <string> & buyers,
                     vector <string> & markets,
                     vector <string> & coins,
                     vector <float> & bidsizes,
                     vector <float> & asksizes,
                     vector <float> & realizeds)
{
  sql.clear ();
  sql.add ("SELECT * FROM availables ORDER BY id;");
  map <string, vector <string> > result = sql.query ();
  vector <string> values;
  
  ids.clear ();
  values = result ["id"];
  for (auto & v : values) ids.push_back (str2int (v));

  stamps = result ["stamp"];

  seconds.clear ();
  values = result ["second"];
  for (auto & v : values) seconds.push_back (str2int (v));

  sellers = result ["seller"];

  buyers = result ["buyer"];

  markets = result ["market"];

  coins = result ["coin"];

  bidsizes.clear ();
  values = result ["bidsize"];
  for (auto & v : values) bidsizes.push_back (str2float (v));

  asksizes.clear ();
  values = result ["asksize"];
  for (auto & v : values) asksizes.push_back (str2float (v));

  realizeds.clear ();
  values = result ["realized"];
  for (auto & v : values) realizeds.push_back (str2float (v));
}


void availables_delete (SQL & sql, int id)
{
  sql.clear ();
  sql.add ("DELETE FROM availables WHERE id =");
  sql.add (id);
  sql.add (";");
  sql.execute ();
}
