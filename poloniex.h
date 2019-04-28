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


#ifndef INCLUDED_POLONIEX_H
#define INCLUDED_POLONIEX_H


#include "libraries.h"


string poloniex_get_exchange ();
vector <string> poloniex_get_coin_ids ();
string poloniex_get_coin_id (string coin);
string poloniex_get_coin_abbrev (string coin);
string poloniex_get_coin_name (string coin);
void poloniex_get_coins (const string & market,
                         vector <string> & coin_abbrevs,
                         string & error);
void poloniex_get_market_summaries (string market,
                                    vector <string> & coin_ids,
                                    vector <float> & bid,
                                    vector <float> & ask,
                                    string & error);
void poloniex_get_buyers (string market,
                          string coin,
                          vector <float> & quantity,
                          vector <float> & rate,
                          string & error,
                          bool hurry);
void poloniex_get_sellers (string market,
                           string coin,
                           vector <float> & quantity,
                           vector <float> & rate,
                           string & error,
                           bool hurry);
void poloniex_get_wallet_details (string coin, string & json, string & error,
                                  string & address, string & payment_id);
void poloniex_get_balances (vector <string> & coin_abbrev,
                            vector <string> & coin_ids,
                            vector <float> & total,
                            vector <float> & reserved,
                            vector <float> & unconfirmed,
                            string & error);
void poloniex_get_balance (const string & coin, float * total, float * available, float * reserved, float * unconfirmed);
void poloniex_set_balance (const string & coin, float total, float available, float reserved, float unconfirmed);
vector <string> poloniex_get_coins_with_balances ();
string poloniex_withdrawal_order (const string & coin,
                                  float quantity,
                                  const string & address,
                                  const string & paymentid,
                                  string & json,
                                  string & error);
void poloniex_get_open_orders (vector <string> & identifiers,
                               vector <string> & markets,
                               vector <string> & coin_abbreviations,
                               vector <string> & coin_ids,
                               vector <string> & buy_sells,
                               vector <float> & amounts,
                               vector <float> & rates,
                               vector <string> & dates,
                               string & error);
bool poloniex_cancel_order (const string & order_id,
                            string & error);
string poloniex_limit_buy (const string & market,
                           const string & coin,
                           float quantity,
                           float rate,
                           string & json,
                           string & error);
string poloniex_limit_sell (const string & market,
                            const string & coin,
                            float quantity,
                            float rate,
                            string & json,
                            string & error);
void poloniex_withdrawalhistory (const string & coin,
                                 vector <string> & order_ids,
                                 vector <string> & coin_abbreviations,
                                 vector <string> & coin_ids,
                                 vector <float> & quantities,
                                 vector <string> & addresses,
                                 vector <string> & transaction_ids,
                                 string & error);
void poloniex_deposithistory (const string & coin,
                              vector <string> & order_ids,
                              vector <string> & coin_abbreviations,
                              vector <string> & coin_ids,
                              vector <float> & quantities,
                              vector <string> & addresses,
                              vector <string> & transaction_ids,
                              string & error);
int poloniex_get_hurried_call_passes ();
int poloniex_get_hurried_call_fails ();


#endif
