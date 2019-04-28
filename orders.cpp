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
#include "models.h"
#include "bittrex.h"
#include "cryptopia.h"
#include "bl3p.h"
#include "shared.h"
#include "exchanges.h"


int main (int argc, char *argv[])
{
  initialize_program (argc, argv);

  // Gather all the arguments passed to this program.
  vector <string> arguments;
  for (int i = 1; i < argc; i++) {
    arguments.push_back (argv[i]);
  }

  // Whether to cancel all open orders.
  bool cancel = in_array (string ("cancel"), arguments);

  // Whether an exchange is passed, to operate on only.
  vector <string> exchanges = exchange_get_names ();
  string one_exchange;
  for (auto exchange : exchanges) {
    if (in_array (exchange, arguments)) one_exchange = exchange;
  }

  // Statistical information.
  map <string, int> total_orders;
  map <string, float> total_coins;
  for (auto exchange : exchanges) {

    // Do this exchange only?
    if (!one_exchange.empty ()) {
      if (exchange != one_exchange) continue;
    }

    // Whitespace.
    cout << endl;
    
    // Read orders.
    map <string, float> market_coins;
    map <string, int> market_orders;
    vector <string> identifier;
    vector <string> market;
    vector <string> dummy;
    vector <string> coin_ids;
    vector <string> buy_sell;
    vector <float> quantity;
    vector <float> rate;
    vector <string> date;
    string error;
    exchange_get_open_orders (exchange, identifier, market, dummy, coin_ids, buy_sell, quantity, rate, date, error);
    for (unsigned int i = 0; i < identifier.size(); i++) {
      string coin_identifier = coin_ids [i];
      string market_id = market[i];
      cout << exchange << " " << buy_sell[i] << " " << float2string (quantity[i]) << " " << coin_identifier << " at rate " << float2string (rate[i]) << " total " << float2string (quantity[i] * rate[i]) << " " << market_id << endl;
      market_coins [market_id] += (quantity[i] * rate[i]);
      market_orders [market_id]++;
      // Cancel this order?
      if (cancel) {
        bool cancelled = exchange_cancel_order (exchange, identifier[i]);
        if (cancelled) {
          cout << "Cancelled" << endl;
        } else {
          cout << "Failed to cancel this order" << endl;
        }
      }
    }
    if (!identifier.empty()) {
      for (auto element : market_coins) {
        string market = element.first;
        float coins = element.second;
        cout << exchange << " has " << to_string (market_orders[market]) << " orders " << float2string (coins) << " " << market << endl;
        total_orders [market] += market_orders[market];
        total_coins [market] += coins;
      }
    }
  }

  // Whitespace.
  cout << endl;
  
  // Output totals.
  for (auto element : total_orders) {
    string market = element.first;
    cout << "Total " << to_string (element.second) << " orders " << float2string (total_coins[market]) << " " << market << endl;
  }

  cout << endl;
  cout << "To cancel all orders: ./orders cancel" << endl;
  
  finalize_program ();
  return EXIT_SUCCESS;
}
