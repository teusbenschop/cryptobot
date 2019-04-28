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


#ifndef INCLUDED_TRADESATOSHI_H
#define INCLUDED_TRADESATOSHI_H


#include "libraries.h"


string tradesatoshi_get_exchange ();
vector <string> tradesatoshi_get_coin_ids ();
string tradesatoshi_get_coin_id (string coin);
string tradesatoshi_get_coin_abbrev (string coin);
string tradesatoshi_get_coin_name (string coin);
void tradesatoshi_get_coins (const string & market,
                             vector <string> & coin_abbrevs,
                             vector <string> & coin_ids,
                             vector <string> & names,
                             vector <string> & statuses,
                             string & error);
void tradesatoshi_get_market_summaries (string market,
                                        vector <string> & coins,
                                        vector <float> & bids,
                                        vector <float> & asks,
                                        string & error);
void tradesatoshi_get_buyers (string market,
                              string coin,
                              vector <float> & quantities,
                              vector <float> & rates,
                              string & error,
                              bool hurry);
void tradesatoshi_get_sellers (string market,
                               string coin,
                               vector <float> & quantities,
                               vector <float> & rates,
                               string & error,
                               bool hurry);
void tradesatoshi_get_wallet_details (string coin, string & json, string & error,
                                      string & address, string & payment_id);
void tradesatoshi_get_balances (vector <string> & coin_abbrev,
                                vector <string> & coin_ids,
                                vector <float> & total,
                                vector <float> & reserved,
                                vector <float> & unconfirmed,
                                string & error);
void tradesatoshi_get_balance (const string & coin, float * total, float * available, float * reserved, float * unconfirmed);
void tradesatoshi_set_balance (const string & coin, float total, float available, float reserved, float unconfirmed);
vector <string> tradesatoshi_get_coins_with_balances ();
string tradesatoshi_withdrawal_order (const string & coin,
                                      float quantity,
                                      const string & address,
                                      const string & paymentid,
                                      string & json,
                                      string & error);
void tradesatoshi_get_open_orders (vector <string> & identifiers,
                                   vector <string> & markets,
                                   vector <string> & coin_abbreviations,
                                   vector <string> & coin_ids,
                                   vector <string> & buy_sells,
                                   vector <float> & amounts,
                                   vector <float> & rates,
                                   vector <string> & dates,
                                   string & error);
bool tradesatoshi_cancel_order (const string & order_id,
                                string & error);
string tradesatoshi_limit_buy (const string & market,
                               const string & coin,
                               float quantity,
                               float rate,
                               string & json,
                               string & error);
string tradesatoshi_limit_sell (const string & market,
                                const string & coin,
                                float quantity,
                                float rate,
                                string & json,
                                string & error);
void tradesatoshi_withdrawalhistory (const string & coin,
                                     vector <string> & order_ids,
                                     vector <string> & coin_abbreviations,
                                     vector <string> & coin_ids,
                                     vector <float> & quantities,
                                     vector <string> & addresses,
                                     vector <string> & transaction_ids,
                                     string & error);
void tradesatoshi_deposithistory (const string & coin,
                                  vector <string> & order_ids,
                                  vector <string> & coin_abbreviations,
                                  vector <string> & coin_ids,
                                  vector <float> & quantities,
                                  vector <string> & addresses,
                                  vector <string> & transaction_ids,
                                  string & error);
int tradesatoshi_get_hurried_call_passes ();
int tradesatoshi_get_hurried_call_fails ();


#endif
