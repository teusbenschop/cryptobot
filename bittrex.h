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


#ifndef INCLUDED_BITTREX_H
#define INCLUDED_BITTREX_H


#include "libraries.h"


string bittrex_get_exchange ();
vector <string> bittrex_get_coin_ids ();
string bittrex_get_coin_id (string coin);
string bittrex_get_coin_abbrev (string coin);
string bittrex_get_coin_name (string coin);
void bittrex_get_coins (const string & market,
                        vector <string> & coin_abbrevs,
                        vector <string> & coin_ids,
                        vector <string> & names,
                        string & error);
void bittrex_get_market_summaries (string market,
                                   vector <string> & coin_ids,
                                   vector <float> & bid,
                                   vector <float> & ask,
                                   string & error);
void bittrex_get_open_orders (vector <string> & identifier,
                              vector <string> & market,
                              vector <string> & coin_abbreviation,
                              vector <string> & coin_ids,
                              vector <string> & buy_sell,
                              vector <float> & quantity,
                              vector <float> & rate,
                              vector <string> & date,
                              string & error);
void bittrex_get_balances (vector <string> & coin_abbrev,
                           vector <string> & coin_ids,
                           vector <float> & total,
                           vector <float> & reserved,
                           vector <float> & unconfirmed,
                           string & error);
void bittrex_get_balance (const string & coin, float * total, float * available, float * reserved, float * unconfirmed);
void bittrex_set_balance (const string & coin, float total, float available, float reserved, float unconfirmed);
vector <string> bittrex_get_coins_with_balances ();
void bittrex_get_wallet_details (string coin, string & json, string & error,
                                 string & address, string & payment_id);
void bittrex_get_buyers (string market,
                         string coin,
                         vector <float> & quantity,
                         vector <float> & rate,
                         string & error,
                         bool hurry);
void bittrex_get_sellers (string market,
                          string coin,
                          vector <float> & quantity,
                          vector <float> & rate,
                          string & error,
                          bool hurry);
string bittrex_limit_buy (string market,
                          string coin,
                          float quantity,
                          float rate,
                          string & json,
                          string & error);
string bittrex_limit_sell (string market,
                           string coin,
                           float quantity,
                           float rate,
                           string & json,
                           string & error);
bool bittrex_cancel_order (string order_id,
                           string & error);
string bittrex_withdrawal_order (string coin,
                                 float quantity,
                                 string address,
                                 string paymentid,
                                 string & json,
                                 string & error);
void bittrex_withdrawalhistory (vector <string> & order_ids,
                                vector <string> & coin_abbreviations,
                                vector <string> & coin_ids,
                                vector <float> & quantities,
                                vector <string> & addresses,
                                vector <string> & transaction_ids,
                                string & error);
void bittrex_deposithistory (vector <string> & order_ids,
                             vector <string> & coin_abbreviations,
                             vector <string> & coin_ids,
                             vector <float> & quantities,
                             vector <string> & addresses,
                             vector <string> & transaction_ids,
                             string & error);
void bittrex_get_minimum_trade_amounts (const string & market,
                                        const vector <string> & coins,
                                        map <string, float> & amounts,
                                        string & error);
int bittrex_get_hurried_call_passes ();
int bittrex_get_hurried_call_fails ();


#endif
