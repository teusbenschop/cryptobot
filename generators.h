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


#ifndef INCLUDED_GENERATORS_H
#define INCLUDED_GENERATORS_H


void generator_load_data ();
void website_index_generator ();
void prospective_coin_item_generator (string coin, int & thread_count);
void prospective_coin_page_generator (int max_threads);
void asset_performance_coin_generator (string coin, int & thread_count);
void asset_performance_page_generator (int max_threads);
void balances_total_bitcoin_generator ();
void balances_total_converted_to_bitcoin_generator ();
void balances_arbitrage_generator ();
void balances_coin_generator (string coin, int & thread_count);
void balances_page_generator (int max_threads);
void website_javascript_copier ();
void performance_reporter (bool mail);
void load_bought_coins_details ();
void delayed_transfers_reporter (bool mail);
void arbitrage_performance_page_generator (int max_threads);
void arbitrage_coin_performance_generator (string exchange1, string exchange2, string market, string coin, int & thread_count);
void arbitrage_opportunities_page_generator (int max_threads);
void arbitrage_coin_opportunity_generator (string exchange1, string exchange2,
                                           string market, string coin,
                                           float volume,
                                           int & thread_count,
                                           int & opportunities_count);
string generator_chart_rates (string exchange1, string exchange2, string market, string coin, int days);
void generator_list_comments (string coin, vector <string> & lines);
string generator_page_timestamp ();
void website_consistency_generator ();
void website_notes_generator ();


#endif


