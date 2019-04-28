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


#ifndef INCLUDED_BL3P_H
#define INCLUDED_BL3P_H


#include "libraries.h"


string bl3p_get_exchange ();
vector <string> bl3p_get_coin_ids ();
string bl3p_get_coin_id (string coin);
string bl3p_get_coin_abbrev (string coin);
string bl3p_get_coin_name (string coin);
void bl3p_get_coins (vector <string> & currencies,
                     vector <string> & coin_ids,
                     vector <string> & names,
                     string & error);
void bl3p_get_market_summaries (string market,
                                vector <string> & coin_ids,
                                vector <float> & bid,
                                vector <float> & ask,
                                string & error);
void bl3p_get_open_orders (vector <string> & identifiers,
                           vector <string> & markets,
                           vector <string> & coin_abbrevs,
                           vector <string> & coin_ids,
                           vector <string> & buys_sells,
                           vector <float> & quantities,
                           vector <float> & rates,
                           vector <string> & dates,
                           string & error);
void bl3p_get_balances (vector <string> & coin_abbrev,
                        vector <string> & coin_ids,
                        vector <float> & total,
                        vector <float> & reserved,
                        vector <float> & unconfirmed,
                        string & error);
void bl3p_get_balance (const string & coin, float * total, float * available, float * reserved, float * unconfirmed);
void bl3p_set_balance (const string & coin, float total, float available, float reserved, float unconfirmed);
vector <string> bl3p_get_coins_with_balances ();
void bl3p_get_wallet_details (string coin, string & json, string & error,
                              string & address, string & payment_id);
void bl3p_get_buyers (string market,
                      string coin,
                      vector <float> & quantity,
                      vector <float> & rate,
                      string & error,
                      bool hurry);
void bl3p_get_sellers (string market,
                       string coin,
                       vector <float> & quantity,
                       vector <float> & rate,
                       string & error,
                       bool hurry);
void bl3p_deposithistory (string coin,
                          vector <string> & order_ids,
                          vector <string> & coin_abbreviations,
                          vector <string> & coin_ids,
                          vector <float> & quantities,
                          vector <string> & addresses,
                          vector <string> & transaction_ids,
                          string & error);
int bl3p_get_hurried_call_passes ();
int bl3p_get_hurried_call_fails ();
bool bl3p_cancel_order (string order_id, string & error);
string bl3p_limit_buy (const string & market,
                       const string & coin,
                       float quantity,
                       float rate,
                       string & json,
                       string & error);
string bl3p_limit_sell (const string & market,
                        const string & coin,
                        float quantity,
                        float rate,
                        string & json,
                        string & error);


#endif
