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


#ifndef INCLUDED_YOBIT_H
#define INCLUDED_YOBIT_H


#include "libraries.h"


string yobit_get_exchange ();
vector <string> yobit_get_coin_ids ();
string yobit_get_coin_id (string coin);
string yobit_get_coin_abbrev (string coin);
string yobit_get_coin_name (string coin);
void yobit_get_coins (const string & market, vector <string> & coin_abbreviations, string & error);
void yobit_get_market_summaries (string market,
                                 vector <string> & coin_ids,
                                 vector <float> & bid,
                                 vector <float> & ask,
                                 string & error);
void yobit_get_buyers (string market,
                       string coin,
                       vector <float> & quantity,
                       vector <float> & rate,
                       string & error,
                       bool hurry);
void yobit_get_sellers (string market,
                        string coin,
                        vector <float> & quantity,
                        vector <float> & rate,
                        string & error,
                        bool hurry);
void yobit_get_wallet_details (string coin, string & json, string & error,
                               string & address, string & payment_id);
void yobit_get_balances (vector <string> & coin_abbrev,
                         vector <string> & coin_ids,
                         vector <float> & total,
                         vector <float> & reserved,
                         vector <float> & unconfirmed,
                         string & error);
void yobit_get_balance (const string & coin, float * total, float * available, float * reserved, float * unconfirmed);
void yobit_set_balance (const string & coin, float total, float available, float reserved, float unconfirmed);
vector <string> yobit_get_coins_with_balances ();
string yobit_limit_buy (string market,
                        string coin,
                        float quantity,
                        float rate,
                        string & json,
                        string & error);
string yobit_limit_sell (string market,
                         string coin,
                         float quantity,
                         float rate,
                         string & json,
                         string & error);
void yobit_get_open_orders (vector <string> & identifier,
                            vector <string> & market,
                            vector <string> & coin_abbreviation,
                            vector <string> & coin_ids,
                            vector <string> & buy_sell,
                            vector <float> & amount,
                            vector <float> & rate,
                            vector <string> & date,
                            string & error);
bool yobit_cancel_order (string order_id,
                         string & error);
string yobit_withdrawal_order (string coin,
                               float quantity,
                               string address,
                               string paymentid,
                               string & json,
                               string & error);
void yobit_withdrawalhistory (vector <string> & order_ids,
                              vector <string> & coin_abbreviations,
                              vector <string> & coin_ids,
                              vector <float> & quantities,
                              vector <string> & addresses,
                              vector <string> & transaction_ids,
                              string & error);
void yobit_deposithistory (vector <string> & order_ids,
                           vector <string> & coin_abbreviations,
                           vector <string> & coin_ids,
                           vector <float> & quantities,
                           vector <string> & addresses,
                           vector <string> & transaction_ids,
                           string & error);
int yobit_get_hurried_call_passes ();
int yobit_get_hurried_call_fails ();


#endif
