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


#include "libraries.h"
#include <sql.h>


#ifndef INCLUDED_MODELS_H
#define INCLUDED_MODELS_H


void rates_get (SQL & sql, vector <int> & seconds,
                vector <string> & exchanges, vector <string> & markets, vector <string> & coins,
                vector <float> & bids, vector <float> & asks, vector <float> & rates,
                bool only_recent);
float rate_get_cached (const string & exchange, const string & market, const string & coin);


class arbitrage_item
{
public:
  string market;
  string coin;
  string seller;
  float ask = 0;
  string buyer;
  float bid = 0;
  float percentage = 0;
  float volume = 0;
};

void arbitrage_store (SQL & sql, const vector <arbitrage_item> & items);
void arbitrage_get (SQL & sql, const string & exchange1, const string & exchange2, const string & market,
                    vector <int> & minutes, vector <string> & coins, vector <string> & volumes);
void arbitrage_get (SQL & sql,
                    vector <int> & minutes,
                    vector <string> & markets,
                    vector <string> & sellers,
                    vector <string> & coins,
                    vector <float> & asks,
                    vector <string> & buyers,
                    vector <float> & bids,
                    vector <int> & percentages,
                    vector <float> & volumes);


float balances_get_average_hourly_btc_equivalent (SQL & sql, string exchange, string coin, int oldhour, int newhour);
float balances_get_average_hourly_balance (SQL & sql, string exchange, string coin, int oldhour, int newhour);
vector <string> balances_get_distinct_daily_coins (SQL & sql);
void balances_get_current (SQL & sql,
                           vector <string> & exchanges,
                           vector <string> & coins,
                           vector <float> & totals);
void balances_get_month (SQL & sql,
                         vector <int> & seconds,
                         vector <string> & exchanges,
                         vector <string> & coins,
                         vector <float> & totals,
                         vector <float> & bitcoins,
                         vector <float> & euros);


void wallets_set_deposit_details (SQL & sql, string exchange, string coin_id, string address, string payment_id);
void wallets_delete_deposit_details (SQL & sql, string exchange, string coin_id);
string wallets_get_address (SQL & sql, string exchange, string coin);
string wallets_get_payment_id (SQL & sql, string exchange, string coin);
void wallets_set_trading (SQL & sql, string exchange, string coin_id, int trading);
bool wallets_get_trading (SQL & sql, string exchange, string coin_id);
void wallets_set_balancing (SQL & sql, string exchange, string coin_id, bool balancing);
bool wallets_get_balancing (SQL & sql, string exchange, string coin_id);
vector <string> wallets_get_trading_coins (SQL & sql);
vector <string> wallets_get_balancing_coins (SQL & sql);
vector <string> wallets_get_trading_exchanges (SQL & sql, string coin_id);
vector <string> wallets_get_balancing_exchanges (SQL & sql, string coin_id);
void wallets_update_withdrawal_amount (SQL & sql, string exchange, string coin_id, float & amount);
void wallets_get_disabled (SQL & sql, vector <string> & details);
string wallets_get_greenlane (SQL & sql, string exchange, string coin);
vector <string> wallets_get_coin_ids (SQL & sql, string exchange);
void wallet_get_details (SQL & sql, vector <string> & exchanges, vector <string> & coin_ids, vector <string> & addresses);
string wallets_get_minimum_withdrawal_amount (SQL & sql, string exchange, string coin_id);
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
                  vector <string> & comments);


void transfers_enter_order (SQL & sql, string order_id,
                            string exchange_withdraw, string coin_abbreviation_withdraw,
                            float quantity,
                            string exchange_deposit, string coin_abbreviation_deposit,
                            string address, string paymentid);
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
                          vector <string> & arriveds);
void transfers_get_non_executed_orders (SQL & sql,
                                        vector <string> & order_ids,
                                        vector <string> & withdraw_exchanges,
                                        vector <string> & coin_abbreviations,
                                        vector <string> & deposit_exchanges,
                                        vector <float> & quantities);
void transfers_set_order_executed (SQL & sql, string exchange, string order_id, string txid);
void transfers_get_non_arrived_orders (SQL & sql,
                                       vector <string> & order_ids,
                                       vector <string> & withdraw_exchanges,
                                       vector <string> & deposit_exchanges,
                                       vector <string> & coin_abbreviations_withdraw,
                                       vector <string> & coin_abbreviations_deposit,
                                       vector <float> & quantities,
                                       vector <string> & transaction_ids);
void transfers_set_order_arrived (SQL & sql, string exchange, string order_id, string transaction_id, float quantity);
int transfers_get_active_orders (SQL & sql, string coin_abbreviation, vector <string> exchanges);
int transfers_get_seconds_since_last_order (SQL & sql, string coin_abbreviation, vector <string> exchanges);
vector <string> transfers_get_rx_ids (SQL & sql);
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
                                  string & rxid);
void transfers_get_non_executed_amounts (SQL & sql, map <string, float> & exchanges_coins_amounts);
void transfers_update_seconds_tests (SQL & sql, int seconds);
void transfers_clean_tests (SQL & sql);
void transfers_set_visible (SQL & sql, int rowid, float quantity);
void transfers_set_arrived (SQL & sql, int rowid);
void transfers_set_order_id (SQL & sql, int rowid, string order_id);

void withdrawals_enter (SQL & sql, string exchange, string orderid);
bool withdrawals_present (SQL & sql, string exchange, string orderid);


void trades_store (SQL & sql,
                   string market, string coin,
                   string buyexchange, string sellexchange,
                   float quantity, float marketgain);
void trades_get_month (SQL & sql,
                       vector <int> & seconds,
                       vector <string> & markets, vector <string> & coins,
                       vector <string> & buyexchanges, vector <string> & sellexchanges,
                       vector <float> & quantities, vector <float> & marketgains);
vector <string> trades_get_coins (SQL & sql);
void trades_get_daily_coin_statistics (SQL & sql, string market, map <string, float> & coins_gains);
void trades_get_daily_exchange_statistics (SQL & sql, string market, map <string, float> & exchange_gain);
void trades_get_statistics (SQL & sql, const string & exchange, const string & market, int days, map <string, int> & counts, map <string, float> & volumes);
void trades_get_statistics_last_24_hours (SQL & sql, const string & exchange, const string & market,
                                          int & count, float & gain);


string failures_type_api_error ();
string failures_type_bot_error ();
string failures_hurried_call_succeeded ();
string failures_hurried_call_failed ();
void failures_store (SQL & sql, string type, string exchange, string coin, float quantity, string message);
void failures_retrieve (SQL & sql, string type,
                        vector <string> & timestamps,
                        vector <string> & exchanges,
                        vector <string> & coins,
                        vector <float> & quantities,
                        vector <string> & messages);
void failures_expire (SQL & sql);


map <string, float> mintrade_get (SQL & sql);


void pairs_add (SQL & sql, string exchange1, string exchange2, string market, string coin, int days);
void pairs_get (SQL & sql, vector <string> & exchanges1, vector <string> & exchanges2, vector <string> & markets, vector <string> & coins, vector <int> & days);
void pairs_remove (SQL & sql, string exchange1, string exchange2, string market, string coin);


void pausetrade_set (SQL & sql,
                     const string & exchange, const string & market, const string & coin,
                     int seconds, const string & reason);
vector <string> pausetrade_get (SQL & sql);


string allrates_database (const string & exchange, const string & market, const string & coin);
string allrates_database (int second);
void allrates_add (const string & exchange, const string & market, const string & coin,
                   int second, float bid, float ask);
void allrates_add (int second,
                   const vector <string> & exchanges,
                   const vector <string> & markets,
                   const vector <string> & coins,
                   const vector <float> & bids,
                   const vector <float> & asks);
bool allrates_exists (const string & exchange, const string & market, const string & coin);
void allrates_get (const string & exchange, const string & market, const string & coin,
                   vector <int> & seconds, vector <float> & bids, vector <float> & asks);
void allrates_get (const string & database,
                   vector <string> & exchanges,
                   vector <string> & markets,
                   vector <string> & coins,
                   vector <float> & bids,
                   vector <float> & asks);
void allrates_delete (const string & exchange, const string & market, const string & coin);
void allrates_expire (const string & exchange, const string & market, const string & coin, int days);


string pattern_type_increase_day_week ();
string pattern_status_buy_uncertain ();
string pattern_status_buy_placed ();
string pattern_status_monitor_rate ();
string pattern_status_stop_loss ();
string pattern_status_sell_done ();
void patterns_store (SQL & sql,
                     const string & exchange, const string & market, const string & coin, const string & reason);
void patterns_get (SQL & sql,
                   vector <string> & exchanges, vector <string> & markets, vector <string> & coins,
                   vector <string> & reasons,
                   bool only_recent);
void patterns_delete (SQL & sql,
                      const string & exchange, const string & market, const string & coin, const string & reason);


class multipath_status
{
public:
  static constexpr const char * bare = "bare";
  static constexpr const char * profitable = "profitable";
  static constexpr const char * start = "start";
  static constexpr const char * buy1place = "buy1place";
  static constexpr const char * buy1uncertain = "buy1uncertain";
  static constexpr const char * buy1placed = "buy1placed";
  static constexpr const char * balance1good = "balance1good";
  static constexpr const char * sell2place = "sell2place";
  static constexpr const char * sell2uncertain = "sell2uncertain";
  static constexpr const char * sell2placed = "sell2placed";
  static constexpr const char * balance2good = "balance2good";
  static constexpr const char * buy3place = "buy3place";
  static constexpr const char * buy3uncertain = "buy3uncertain";
  static constexpr const char * buy3placed = "buy3placed";
  static constexpr const char * balance3good = "balance3good";
  static constexpr const char * sell4place = "sell4place";
  static constexpr const char * sell4uncertain = "sell4uncertain";
  static constexpr const char * sell4placed = "sell4placed";
  static constexpr const char * balance4good = "balance4good";
  static constexpr const char * done = "done";
  static constexpr const char * error = "error";
  static constexpr const char * unprofitable = "unprofitable";
  static constexpr const char * unrecoverable = "unrecoverable";
};

class multipath
{
public:
  // The ID of this path in the database.
  int id = 0;
  // The exchange where the multipath trade takes place.
  string exchange;
  // Step 1: Buy the coin on the market.
  string market1name;
  float market1quantity = 0;
  float ask1 = 0;
  string coin1name;
  float coin1quantity = 0;
  string order1id;
  // Step 2: Sell the coin, bought in the previous step, on the market.
  string coin2name;
  float coin2quantity = 0;
  float bid2 = 0;
  string market2name;
  float market2quantity = 0;
  string order2id;
  // Step 3: Buy the coin on the market.
  string market3name;
  float market3quantity = 0;
  float ask3 = 0;
  string coin3name;
  float coin3quantity = 0;
  string order3id;
  // Step 4: Sell the coin, bought in the previous step, on the initial market.
  string coin4name;
  float coin4quantity = 0;
  float bid4 = 0;
  string market4name;
  float market4quantity = 0;
  string order4id;
  // The gain made at the end of the whole exercise.
  float gain = 0;
  // The status of this path.
  string status = multipath_status::bare;
  // Flag whether being executed.
  bool executing = false;
};

void multipath_store (SQL & sql, const multipath & path);
vector <multipath> multipath_get (SQL & sql);
multipath multipath_get (SQL & sql, int id);
void multipath_update (SQL & sql, const multipath & path);
bool multipath_equal (const multipath & path1, const multipath & path2);
void multipath_expire (SQL & sql);
void multipath_delete (SQL & sql, const multipath & path);


void settings_set (SQL & sql, const string & host, const string & setting, const string & value);
string settings_get (SQL & sql, const string & host, const string & setting);


void buyers_set (const string & exchange,
                 const string & market,
                 const string & coin,
                 float price,
                 const vector <float> & quantities,
                 const vector <float> & rates);
void sellers_set (const string & exchange,
                  const string & market,
                  const string & coin,
                  float price,
                  const vector <float> & quantities,
                  const vector <float> & rates);
void buyers_get (const string & exchange,
                 const string & market,
                 const string & coin,
                 float & price,
                 vector <float> & quantities,
                 vector <float> & rates);
void sellers_get (const string & exchange,
                  const string & market,
                  const string & coin,
                  float & price,
                  vector <float> & quantities,
                  vector <float> & rates);
void buyers_sellers_delete (const string & exchange, const string & market, const string & coin);


void coins_market_get (SQL & sql, vector <string> & exchanges, vector <string> & markets, vector <string> & coins);
void coins_market_set (SQL & sql, const string & exchange, const string & market, const string & coins);


void optimizer_get (SQL & sql,
                    vector <int> & ids,
                    vector <string> & exchanges,
                    vector <string> & coins,
                    vector <float> & starts,
                    vector <float> & forecasts,
                    vector <float> & realizeds);
void optimizer_add (SQL & sql, const string & exchange, const string & coin, float start, float forecast);
void optimizer_update (SQL & sql, int identifier, float realized);
void optimizer_delete (SQL & sql, int identifier);


void weights_get (SQL & sql, const string & exchange, const string & market, const string & coin,
                  string & weights,
                  float & sum_absolute_residuals,
                  float & maximum_residual_percentage,
                  float & average_residual_percentage);
void weights_save (SQL & sql, const string & exchange, const string & market, const string & coin,
                   const string weights,
                   float & sum_absolute_residuals,
                   float & maximum_residual_percentage,
                   float & average_residual_percentage);
void weights_delete (SQL & sql, const string & exchange, const string & market, const string & coin);
string weights_encode (float hourly, float daily, float weekly,
                       unordered_map <int, float> & curves);
void weights_decode (string json,
                     float & hourly, float & daily, float & weekly,
                     unordered_map <int, float> & curve);


class surfer_status
{
public:
  static constexpr const char * entered = "entered";
  static constexpr const char * closed = "closed";
  static constexpr const char * placed = "placed";
};


string surfer_buy ();
string surfer_sell ();
void surfer_get (SQL & sql,
                 vector <int> & ids,
                 vector <int> & stamps,
                 vector <string> & coins,
                 vector <string> & trades,
                 vector <string> & statuses,
                 vector <float> & rates,
                 vector <float> & amounts,
                 vector <string> & remarks);
void surfer_add (SQL & sql, const string & coin, const string & trade, const string & status);
void surfer_update (SQL & sql, int identifier, const string & status, float rate, float amount);
void surfer_update (SQL & sql, int identifier, const string & status, const string & remark);
void surfer_delete (SQL & sql, int identifier);


void bought_get (SQL & sql,
                 vector <int> & hours,
                 vector <int> & ids,
                 vector <string> & exchanges,
                 vector <string> & markets,
                 vector <string> & coins,
                 vector <float> & rates);
void bought_add (SQL & sql, string exchange, string market, string coin, float rate);
void bought_delete (SQL & sql, int identifier);


void sold_store (SQL & sql, string exchange, string market, string coin, float quantity, float buyrate, float sellrate);
void sold_get (SQL & sql, vector <int> & ids,
               vector <int> & hours,
               vector <string> & exchanges, vector <string> & markets, vector <string> & coins,
               vector <float> & quantities, vector <float> & buyrates, vector <float> & sellrates);
void sold_delete (SQL & sql, int id);


void comments_store (SQL & sql, string coin, string comment);
void comments_get (SQL & sql,
                   string coin,
                   vector <int> & ids,
                   vector <string> & stamps,
                   vector <string> & coins,
                   vector <string> & comments);
void comments_delete (SQL & sql, int id);


string conversions_run (SQL & sql, string s);


void availables_store (SQL & sql,
                       string stamp,
                       int second,
                       string seller,
                       string buyer,
                       string market,
                       string coin,
                       float bidsize,
                       float asksize,
                       float realized);
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
                     vector <float> & realizeds);
void availables_delete (SQL & sql, int id);


#endif
