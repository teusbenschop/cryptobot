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


#ifndef INCLUDED_EXCHANGES_H
#define INCLUDED_EXCHANGES_H


vector <string> exchange_get_names ();
vector <string> exchange_get_coin_ids (string exchange);
string exchange_get_coin_id (const string & exchange, const string & coin);
string exchange_get_coin_abbrev (string exchange, string coin);
string exchange_get_coin_name (string exchange, string coin);
void exchange_get_coins (const string & exchange,
                         const string & market,
                         vector <string> & coin_abbrevs,
                         vector <string> & coin_ids,
                         vector <string> & coin_names,
                         vector <string> & exchange_coin_abbreviations,
                         string & error);
vector <string> exchange_get_coins_per_market_cached (const string & exchange, const string & market);
void exchange_get_wallet_details (string exchange, string coin,
                                  string & address, string & payment_id);
void exchange_get_balances (string exchange,
                            vector <string> & coin_abbrev,
                            vector <string> & coin_ids,
                            vector <float> & total,
                            vector <float> & reserved,
                            vector <float> & unconfirmed);
void exchange_get_balance_cached (const string & exchange, const string & coin,
                                  float * total, float * available, float * reserved, float * unconfirmed);
void exchange_set_balance_cache (const string & exchange, const string & coin,
                                 float total, float available, float reserved, float unconfirmed);
vector <string> exchange_get_coins_with_balances (const string & exchange);
void exchange_get_market_summaries (string exchange, string market,
                                    vector <string> & coin_ids,
                                    vector <float> & bid,
                                    vector <float> & ask,
                                    string * error);
void exchange_get_market_summaries_via_satellite (const string & exchange, const string & market,
                                                  vector <string> & coins,
                                                  vector <float> & bids,
                                                  vector <float> & asks);
void exchange_get_buyers (string exchange,
                          string market,
                          string coin,
                          vector <float> & quantity,
                          vector <float> & rate,
                          bool hurry,
                          string & error);
void exchange_get_buyers_via_satellite (string exchange,
                                        string market,
                                        string coin,
                                        vector <float> & quantities,
                                        vector <float> & rates,
                                        bool hurry,
                                        void * output);
void exchange_get_sellers (string exchange,
                           string market,
                           string coin,
                           vector <float> & quantity,
                           vector <float> & rate,
                           bool hurry,
                           string & error);
void exchange_get_sellers_via_satellite (string exchange,
                                         string market,
                                         string coin,
                                         vector <float> & quantities,
                                         vector <float> & rates,
                                         bool hurry,
                                         void * output);
void exchange_get_open_orders (string exchange,
                               vector <string> & identifier,
                               vector <string> & market,
                               vector <string> & coin_abbreviation,
                               vector <string> & coin_ids,
                               vector <string> & buy_sell,
                               vector <float> & quantity,
                               vector <float> & rate,
                               vector <string> & date,
                               string & error);
void exchange_limit_buy (const string & exchange,
                         const string & market,
                         const string & coin, float quantity, float rate,
                         string & error, string & json, string & order_id,
                         void * output);
void exchange_limit_sell (const string & exchange,
                          const string & market,
                          const string & coin, float quantity, float rate,
                          string & error, string & json, string & order_id,
                          void * output);
bool exchange_cancel_order (string exchange, string order_id);
string exchange_withdrawal_order (const string & exchange,
                                  const string & coin,
                                  float quantity,
                                  const string & address,
                                  const string & paymentid,
                                  string & json,
                                  void * output);
void exchange_withdrawalhistory (string exchange,
                                 string coin,
                                 vector <string> & order_ids,
                                 vector <string> & coin_abbreviations,
                                 vector <string> & coin_ids,
                                 vector <float> & quantities,
                                 vector <string> & addresses,
                                 vector <string> & transaction_ids);
void exchange_deposithistory (string exchange,
                              string coin,
                              vector <string> & order_ids,
                              vector <string> & coin_abbreviations,
                              vector <string> & coin_ids,
                              vector <float> & quantities,
                              vector <string> & addresses,
                              vector <string> & transaction_ids);
string exchange_condense_message (string message);
vector <string> exchange_format_error (string error, string func, string exchange, string market, string coin, string satellite);
float exchange_get_trade_fee (string exchange);
float exchange_get_trade_order_ease_percentage (string exchange);
float exchange_get_sell_balance_factor (const string & exchange);
vector <string> exchange_get_supported_markets (string exchange);
string exchange_get_blockchain_explorer (string coin);
int exchange_parse_date (string exchange, string date);
void exchange_get_minimum_trade_amounts (const string & exchange,
                                         const string & market,
                                         const vector <string> & coins,
                                         map <string, float> & market_amounts,
                                         map <string, float> & coin_amounts);
int exchange_get_hurried_call_passes (const string & exchange);
int exchange_get_hurried_call_fails (const string & exchange);


#endif
