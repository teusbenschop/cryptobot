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
#include "controllers.h"
#include "controllers.h"
#include "sql.h"


int main (int argc, char *argv[])
{
  initialize_program (argc, argv);
  
  
  // Gather all the arguments passed to this program.
  vector <string> arguments;
  for (int i = 1; i < argc; i++) {
    arguments.push_back (argv[i]);
  }
  
  
  bool healthy = true;


  // Check on correct number of arguments.
  if (healthy) {
    if ((arguments.size() != 4) && (arguments.size() != 5)) {
      to_tty ({"Usage: ./buy exchange market coin quantity [rate]"});
      healthy = false;
    }
  }
  
  
  // Check that the exchange is known.
  string exchange;
  if (healthy) {
    exchange = arguments [0];
    vector <string> exchanges = exchange_get_names ();
    if (in_array (exchange, exchanges)) {
      to_tty ({"Exchange:", exchange});
    } else {
      to_tty ({"Please pass one of the known exchanges:"});
      to_tty (exchanges);
      healthy = false;
    }
  }
  
    
  // Check that the market is known at the exchange.
  string market;
  if (healthy) {
    market = arguments [1];
    vector <string> markets = exchange_get_supported_markets (exchange);
    if (in_array (market, markets)) {
      to_tty ({"Market:", market});
    } else {
      to_tty ({"Please pass one of the known markets at this exchange:"});
      to_tty (markets);
      healthy = false;
    }
  }
  
    
  // Check that the coin is known at the exchange.
  string coin;
  if (healthy) {
    coin = arguments [2];
    vector <string> coins = exchange_get_coins_per_market_cached (exchange, market);
    if (in_array (coin, coins)) {
      string coin_abbreviation = exchange_get_coin_abbrev (exchange, coin);
      string coin_name = exchange_get_coin_name (exchange, coin);
      to_tty ({"Coin:", coin, coin_abbreviation, coin_name});
    } else {
      to_tty ({"Please pass one of the known coins at this exchange:"});
      to_tty (coins);
    }
  }

  
  // Read the amount to buy.
  float quantity = 0;
  if (healthy) {
    quantity = str2float (arguments [3]);
    to_tty ({"Quantity:", float2string (quantity)});
    if (quantity == 0) healthy = false;
  }
  
  
  // The rate.
  float rate = 0;
  if (healthy) {
    if (arguments.size () > 4) {

      // Read the rate from the command line argument.
      rate = str2float (arguments [4]);
    
    } else {
    
      // Determine the rate based on the order book at the exchange.
      vector <float> quantities;
      vector <float> rates;
      exchange_get_sellers_via_satellite (exchange, market, coin, quantities, rates, true, nullptr);
      fix_dust_trades (market, quantities, rates);
      fix_rate_for_quantity (quantity, quantities, rates);
      if (order_book_is_good (quantities, rates, false)) {
        rate = rates [0];
      } else {
        to_tty ({"The order book at the exchange is too small"});
        healthy = false;
      }
      
    }
    to_tty ({"Rate:", float2string (rate)});
    if (rate == 0) healthy = false;
  }
  
  
  // Place the order.
  string order_id;
  if (healthy) {
    string error, json;
    {
      to_output output ("");
      exchange_limit_buy (exchange, market, coin, quantity, rate, error, json, order_id, &output);
    }
    to_tty ({"Order id", order_id});
    if (order_id.empty ()) {
      to_tty ({"The order could not be placed"});
    }
  }
  
  
  finalize_program ();
  return EXIT_SUCCESS;
}
