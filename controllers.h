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


#ifndef INCLUDED_CONTROLLERS_H
#define INCLUDED_CONTROLLERS_H


void order_transfer (void * output,
                     string exchange_withdraw, string exchange_deposit,
                     string coin, float quantity, bool manual);
void monitor_withdrawals ();
void monitor_poloniex_withdrawals ();
void monitor_deposits_history ();
void monitor_deposits_balances ();
void get_all_balances ();
void record_all_balances ();
void redistribute_all_coins (vector <tuple <string, string, float> > & withdrawals);
void redistribute_coin_processor (void * output,
                                  string coin,
                                  bool carrier,
                                  vector <string> exchanges,
                                  int active_transfer_count, int last_transfer_elapsed_seconds,
                                  vector <string> & exchanges_withdraw,
                                  vector <string> & exchanges_deposit,
                                  vector <float> & transfer_quantities);
void redistribute_coin (string coin, vector <tuple <string, string, float> > & withdrawals);
void show_balances (void * output, vector <string> exchanges, vector <string> coins);
void temporarily_pause_trading (string exchange, string market, string coin, int minutes, string reason);
void monitor_foreign_withdrawals ();
void daily_book_keeper (bool mail);
void failures_reporter (bool mail);
void orders_reporter ();
vector <string> get_trading_coins (const vector <pair <string, string> > & exchange_coin_wallets,
                                   const vector <tuple <string, string, string> > & exchange_market_coin_triplets);
vector <string> get_trading_markets (const string & coin,
                                     const vector <pair <string, string> > & exchange_coin_wallets,
                                     const vector <tuple <string, string, string> > & exchange_market_coin_triplets);
vector <string> get_trading_exchanges (const string & market, const string & coin,
                                       const vector <pair <string, string> > & exchange_coin_wallets,
                                       const vector <tuple <string, string, string> > & exchange_market_coin_triplets);
float get_minimum_withdrawal_amount (string coin, string exchange);
bool is_dust_trade (string market, float quantity, float rate);
void fix_dust_trades (string market, vector <float> & quantities, vector <float> & rates);
bool trade_size_too_low (const map <string, float> & minimum_trade_sizes,
                         const string & exchange, const string & market, const string & coin,
                         float quantity);
void fix_too_low_trades (const map <string, float> & minimum_trade_sizes,
                         const string & exchange, const string & market, const string & coin,
                         vector <float> & quantities, vector <float> & rates);
void fix_rate_for_quantity (float quantity, vector <float> & quantities, vector <float> & rates);
bool order_book_is_good (const vector <float> & quantities, const vector <float> & rates, bool nearly);
void get_market_summaries_hurry (const vector <string> exchanges,
                                 const string & market,
                                 const string & coin,
                                 map <string, vector <float> > & buyer_quantities,
                                 map <string, vector <float> > & buyer_rates,
                                 map <string, vector <float> > & seller_quantities,
                                 map <string, vector <float> > & seller_rates,
                                 void * output);
bool multipath_investigate (void * output,
                            const map <string, float> & minimum_trade_sizes,
                            void * path);
void multipath_processor (void * output,
                          void * path,
                          const map <string, float> & minimum_trade_sizes,
                          const vector <float> & quantities1, const vector <float> & asks1,
                          const vector <float> & quantities2, const vector <float> & bids2,
                          const vector <float> & quantities3, const vector <float> & asks3,
                          const vector <float> & quantities4, const vector <float> & bids4,
                          float multiplier);
void multipath_calculate (void * path);
void multipath_output (void * output, void * path);
bool multipath_clash (void * path, vector <string> & exchanges_markets_coins);
bool multipath_paused (void * path, const vector <string> & exchanges_markets_coins, bool output);

void multipath_place_limit_order (void * output,
                                  void * path,
                                  int step,
                                  mutex & update_mutex,
                                  map <string, float> minimum_trade_sizes,
                                  vector <string> & paused_trades);
void multipath_verify_limit_order (void * output, void * path, int step);
void multipath_verify_balance (void * output,
                               void * path,
                               int step,
                               mutex & update_mutex,
                               int & timer);
float multipath_minimum_required_gain (int trading_step_count);

void pattern_prepare (void * output,
                      const vector <int> & seconds,
                      const vector <float> & asks,
                      const vector <float> & bids,
                      unordered_map <int, float> & minute_asks,
                      unordered_map <int, float> & minute_bids,
                      unordered_map <int, float> & hour_asks,
                      unordered_map <int, float> & hour_bids);
void pattern_prepare (void * output,
                      const string & exchange, const string & market, const string & coin,
                      unordered_map <int, float> & minute_asks,
                      unordered_map <int, float> & minute_bids,
                      unordered_map <int, float> & hour_asks,
                      unordered_map <int, float> & hour_bids);
void pattern_prepare (void * output,
                      const string & exchange, const string & market, const string & coin,
                      int hours,
                      unordered_map <int, float> & minute_asks,
                      unordered_map <int, float> & minute_bids,
                      unordered_map <int, float> & hour_asks,
                      unordered_map <int, float> & hour_bids);
void pattern_smooth (unordered_map <int, float> & asks, unordered_map <int, float> & bids,
                     unordered_map <int, float> & smoothed_rates, unsigned int window);
float pattern_change (unordered_map <int, float> & asks, unordered_map <int, float> & bids, int start, int window);
bool pattern_coin_has_regular_recent_price_movements (unordered_map <int, float> & hour_asks,
                                                      unordered_map <int, float> & hour_bids);
void patterns_simulate_sale (int buy_time, int max_open_time,
                             float buy_price, float sell_price,
                             unordered_map <int, float> & time_asks,
                             unordered_map <int, float> & time_bids,
                             int & real_open_time, bool & sold_in_time);
bool pattern_increase_day_week_detector (void * output,
                                         int detection_hour,
                                         unordered_map <int, float> & hour_asks,
                                         unordered_map <int, float> & hour_bids);
bool pattern_increase_day_week_simulator (void * output,
                                          unordered_map <int, float> & hour_asks,
                                          unordered_map <int, float> & hour_bids,
                                          const string & reason);
vector <pair <float, float> > chart_balances (vector <string> exchanges,
                                              string coin,
                                              int days,
                                              bool btc,
                                              bool eur);
vector <pair <float, float> > chart_rates (string exchange, string market, string coin, int days);
bool locate_trade_order_id (void * output,
                            const string & exchange, const string & market, const string & coin, float rate,
                            string & order_id);
bool get_cached_approximate_rate (const string & exchange, const string & market, const string & coin,
                                  float & ask, float & bid);
bool get_cached_approximate_buyers (const string & exchange,
                                    const string & market,
                                    const string & coin,
                                    vector <float> & quantities,
                                    vector <float> & rates);
bool get_cached_approximate_sellers (const string & exchange,
                                     const string & market,
                                     const string & coin,
                                     vector <float> & quantities,
                                     vector <float> & rates);
float bitcoin2euro (float bitcoins);


#endif


