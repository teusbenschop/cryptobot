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


#ifndef INCLUDED_TRADERS_H
#define INCLUDED_TRADERS_H


void limit_trade_follow_up (void * output, string exchange, string market, string coin, const string & error, const string & json, string order_id, float quantity, float rate, string direction, vector <string> & exchanges);
void pattern_monitor_rates (map <string, float> minimum_trade_sizes,
                            vector <string> redistributing_exchanges_plus_coins);
void pattern_buy_coin_now (vector <string> exchanges,
                           string coin,
                           mutex & update_mutex,
                           map <string, float> & minimum_trade_sizes,
                           vector <string> & paused_trades);
void set_likely_coin_bought_price (string exchange, string market, string coin);
void redistribute_coin_bought_price (string original_exchange, string target_exchange, string market, string coin, float price);
float arbitrage_difference_percentage (string exchange1, string exchange2);
void arbitrage_processor (void * output, string market, string coin, float & quantity,
                          float ask_quantity, float bid_quantity,
                          float & minimum_ask, float & maximum_bid,
                          float asking_exchange_balance,
                          float bidding_exchange_balance,
                          string asking_exchange, string bidding_exchange,
                          bool & market_balance_too_low, bool & coin_balance_too_low,
                          map <string, float> minimum_trade_sizes,
                          vector <float> & buyers_quantities, vector <float> & buyers_rates,
                          vector <float> & sellers_quantities, vector <float> & sellers_rates);


#endif


